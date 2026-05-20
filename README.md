# Multi-Modal Automated Attendance System

An Arduino-based attendance system that authenticates individuals using **dual biometric verification** — RFID card scanning and fingerprint recognition — combined with **real-time timestamping** via a DS3231 RTC module. Designed for classrooms, labs, or any controlled-access environment requiring tamper-resistant, time-stamped attendance records.

> **Patent Notice**: The design, logic, and integration architecture of this system are the intellectual property of the author. Unauthorized reproduction or commercial use is prohibited.

---

## Features

- Dual-mode authentication: RFID card (MFRC522) **and** fingerprint sensor (R307)
- Real-time timestamps on every attendance entry (DS3231 RTC)
- Three-state LED + buzzer feedback: success, already-present, unauthorized
- Serial interface for querying live attendance data
- Absent/present tracking with dynamic array management
- Supports up to 7 registered individuals (expandable)

---

## Hardware

| Component | Interface | Arduino Pin(s) |
|---|---|---|
| MFRC522 RFID module | SPI | SS → 10, RST → 9 |
| R307 fingerprint sensor | UART (SoftwareSerial) | RX → 2, TX → 3 |
| DS3231 RTC module | I²C | SDA, SCL |
| Green LED (success) | Digital | 7 |
| Yellow LED (duplicate) | Digital | 6 |
| Red LED (unauthorized) | Digital | 5 |
| Buzzer | Digital | 8 |

### Wiring Notes
- The MFRC522 operates at **3.3V** — do not connect VCC to the 5V rail.
- The R307 fingerprint sensor requires a **5V** supply and communicates at 57600 baud.
- The DS3231 RTC requires **pull-up resistors** (4.7kΩ) on the SDA and SCL lines if not already present on your module.

---

## Software Dependencies

Install the following libraries via the Arduino Library Manager:

| Library | Purpose |
|---|---|
| `MFRC522` | RFID card read/write |
| `Adafruit_Fingerprint` | R307 fingerprint sensor |
| `ds3231` | DS3231 RTC communication |
| `Wire` | I²C (RTC) |
| `SPI` | SPI (RFID) |

---

## Setup

### 1. Register fingerprints
Use Adafruit's fingerprint enrollment sketch to enroll fingerprints **before** deploying this system. Each fingerprint ID must map to the corresponding index in the `record[]` array (ID 1 → `record[0]`, etc.).

### 2. Write student IDs to RFID cards
Each MIFARE Classic card must have the student's roll number written to **block 1** of sector 0. The system reads 16 bytes and trims trailing characters automatically.

### 3. Register roll numbers
Edit the `record[]` array in the sketch to match your class roster:

```cpp
String record[7] = {
  "21BCE9489", "21BCE9970", "21BCE9961",
  "21BCE9942", "21BCE9977", "21BCE9966", "70478"
};
```

Increment the array size constant `a` to match the number of registered students.

### 4. Set the RTC time (first run only)
Uncomment the following line in `setup()`, upload, then comment it back out and re-upload:

```cpp
parse_cmd("TssmmhhWDDMMYYYY", 16);
```

Replace the placeholder with actual values, e.g. `T304512319052025` = 30s 45m 12h Wed 19 May 2025.

---

## How It Works

On each loop iteration, the system checks **both** authentication channels simultaneously.

**RFID path:**
1. Detects a new card via `PICC_IsNewCardPresent()`
2. Authenticates and reads block 1 (student roll number)
3. Cross-references with the absent list

**Fingerprint path:**
1. Captures an image and converts it to a template
2. Searches the internal database for a match
3. Maps the fingerprint ID to the corresponding roll number in `record[]`

**On successful authentication (either path):**
- Student is moved from `absentees[]` to `presentees[]`
- Current timestamp is saved to `timing[]`
- Green LED + buzzer pulse for 1 second

**On duplicate scan (already present):**
- Yellow LED + buzzer double-pulse

**On unrecognized ID:**
- Red LED + buzzer triple-pulse

---

## Serial Commands

Connect via Serial Monitor at **9600 baud**. Send a single character:

| Command | Response |
|---|---|
| `P` | List of students currently present |
| `A` | List of students currently absent |
| `T` | Timestamps for each present student (in order of check-in) |

---

## LED & Buzzer Reference

| Event | LED | Buzzer pattern |
|---|---|---|
| Successful check-in | Green (pin 7) | 1× long (1s) |
| Already marked present | Yellow (pin 6) | 2× medium (350ms) |
| Unrecognized / not found | Red (pin 5) | 3× short (200ms) |

---

## Limitations & Future Work

- In-memory storage only — attendance resets on power cycle. Planned: SD card logging.
- Hardcoded roll numbers — planned: EEPROM-based dynamic registration.
- No duplicate check between RFID and fingerprint paths in edge cases.
- Expanding beyond 7 students requires increasing array sizes and re-uploading.

---

## Author

Chinni krishna Bhukya — B.Tech CSE with specialisation in AI and ML, VIT-AP University | MS CS, San Jose State University
*Patent pending. All rights reserved.*
