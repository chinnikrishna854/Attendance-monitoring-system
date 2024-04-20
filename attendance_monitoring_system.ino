#include <SPI.h>
#include <MFRC522.h>

#include <Wire.h>
#include "ds3231.h"

#include <Adafruit_Fingerprint.h>

//fingerprint sensor (r307)
#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial mySerial(2, 3);
#else
#define mySerial Serial1
#endif
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

//RTC module (ds3231)
#define BUFF_MAX 128

uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;
unsigned long prev, interval = 1000;

void parse_cmd(char *cmd, int cmdsize);

String tim;

//RFID module
#define RST_PIN         9
#define SS_PIN          10

MFRC522 mfrc522(SS_PIN, RST_PIN);

String str;

char c;

//
int a = 7, p = 0;
String record[7] = {"21BCE9489", "21BCE9970", "21BCE9961", "21BCE9942", "21BCE9977", "21BCE9966", "70478"};
String absentees[7] = {"", "", "", "", "", ""};
String presentees[7] = {"", "", "", "", "", ""};
String timing[7] = {"", "", "", "", "", ""};


void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Wire.begin();
  DS3231_init(DS3231_CONTROL_INTCN);
  memset(recv, 0, BUFF_MAX);
  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  finger.begin(57600);
  for(int i = 0; i<a; i++) {
    absentees[i] = record[i];
  }
  //parse_cmd("TssmmhhWDDMMYYYY", 16);
}


void loop() {
  bool temp = run();
  if (temp&&check1(str)) {
    bool temp = false;
    for(int i = 0; i<a-1; i++) {
      if(absentees[i] == str)
        temp = true;
      if(temp)
        absentees[i] = absentees[i+1];
    }
    absentees[--a] = "";
    presentees[p] = str;
    timing[p++] = tim;
    digitalWrite(7, HIGH);
    digitalWrite(8, HIGH);
    delay(1000);
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
  }
  else if(temp&&check2(str)) {
    digitalWrite(6, HIGH);
    digitalWrite(8, HIGH);
    delay(350);
    digitalWrite(8, LOW);
    delay(200);
    digitalWrite(8, HIGH);
    delay(350);
    digitalWrite(6, LOW);
    digitalWrite(8, LOW);
  }
  else if(temp) {
    digitalWrite(5, HIGH);
    digitalWrite(8, HIGH);
    delay(200);
    digitalWrite(8, LOW);
    delay(200);
    digitalWrite(8, HIGH);
    delay(200);
    digitalWrite(8, LOW);
    delay(200);
    digitalWrite(8, HIGH);
    delay(200);
    digitalWrite(5, LOW);
    digitalWrite(8, LOW);
  }
  else {
    if(Serial.available()>0) {
      char ch = Serial.read();
      int in = Serial.read();
      switch(ch) {
        case 'P':
          for(int i=0; i<p; i++) {
            Serial.println(presentees[i]);
          }
          break;
        case 'A':
          for(int i=0; i<a; i++) {
            Serial.println(absentees[i]);
          }
          break;
        case 'T':
          for(int i=0; i<p; i++) {
            Serial.println(timing[i]);
          }
          break;
        default:
          Serial.println("Please give valid input...");
      }
    }
  }
}

bool run() {
  //fingerprint sensor
  getFingerprintID();
  delay(50);

  //RTC
  char in;
  char buff[BUFF_MAX];
  unsigned long now = millis();
  struct ts t;

  // show time once in a while
  if ((now - prev > interval) && (Serial.available() <= 0)) {
      DS3231_get(&t);
      
      snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,
            t.mon, t.mday, t.hour, t.min, t.sec);

      tim = buff;
      prev = now;
    }
  
  //Serial.println(tim);
  //delay(1000);

  //RFID
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  byte block;
  byte len;
  MFRC522::StatusCode status;

  if ( !mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }

  if ( !mfrc522.PICC_ReadCardSerial()) {
    return false;
  }


  mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid));

  len = 18;

  byte buffer2[18];
  block = 1;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid)); //line 834
  if (status != MFRC522::STATUS_OK) {
    return false;
  }

  status = mfrc522.MIFARE_Read(block, buffer2, &len);

  str = "";
  for (uint8_t i = 0; i < 16; i++) {
    c = buffer2[i];
    str += c;
  }
  str.remove(9, 7);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  return true;
}

bool check1(String str) {
  for(int i = 0; i<a; i++) {
    if(str == absentees[i])
      return true;
  }
  return false;
}

bool check2(String str) {
  for(int i = 0; i<a; i++) {
    if(str == presentees[i])
      return true;
  }
  return false;
}

void getFingerprintID() {
  uint8_t p = finger.getImage();
  // OK success!
  if(p == FINGERPRINT_OK) {
    p = finger.image2Tz();
    // OK converted!

    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK)
      fig(finger.fingerID);

    if (p == FINGERPRINT_NOTFOUND) {
      digitalWrite(5, HIGH);
      digitalWrite(8, HIGH);
      delay(200);
      digitalWrite(8, LOW);
      delay(200);
      digitalWrite(8, HIGH);
      delay(200);
      digitalWrite(8, LOW);
      delay(200);
      digitalWrite(8, HIGH);
      delay(200);
      digitalWrite(5, LOW);
      digitalWrite(8, LOW);
    }
  }
}

uint8_t fig(uint8_t id) {
  str = record[id-1];
  //Serial.println(str);
  if(check2(str)) {
    digitalWrite(6, HIGH);
    digitalWrite(8, HIGH);
    delay(350);
    digitalWrite(8, LOW);
    delay(200);
    digitalWrite(8, HIGH);
    delay(350);
    digitalWrite(6, LOW);
    digitalWrite(8, LOW);
  }
  if (check1(str)) {
    bool temp = false;
    for(int i = 0; i<a-1; i++) {
      if(absentees[i] == str)
        temp = true;
      if(temp)
        absentees[i] = absentees[i+1];
    }
    absentees[--a] = "";
    presentees[p] = str;
    timing[p++] = tim;
    digitalWrite(7, HIGH);
    digitalWrite(8, HIGH);
    delay(1000);
    digitalWrite(7, LOW);
    digitalWrite(8, LOW);
  }
}

void parse_cmd(char *cmd, int cmdsize)
{
    uint8_t i;
    uint8_t reg_val;
    char buff[BUFF_MAX];
    struct ts t;

    //snprintf(buff, BUFF_MAX, "cmd was '%s' %d\n", cmd, cmdsize);
    //Serial.print(buff);

    // TssmmhhWDDMMYYYY aka set time
    if (cmd[0] == 84 && cmdsize == 16) {
        //T355720619112011
        t.sec = inp2toi(cmd, 1);
        t.min = inp2toi(cmd, 3);
        t.hour = inp2toi(cmd, 5);
        t.wday = cmd[7] - 48;
        t.mday = inp2toi(cmd, 8);
        t.mon = inp2toi(cmd, 10);
        t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
        DS3231_set(t);
        Serial.println("OK");
    } else if (cmd[0] == 49 && cmdsize == 1) {  // "1" get alarm 1
        DS3231_get_a1(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 50 && cmdsize == 1) {  // "2" get alarm 1
        DS3231_get_a2(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 51 && cmdsize == 1) {  // "3" get aging register
        Serial.print("aging reg is ");
        Serial.println(DS3231_get_aging(), DEC);
    } else if (cmd[0] == 65 && cmdsize == 9) {  // "A" set alarm 1
        DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A1IE);
        //ASSMMHHDD
        for (i = 0; i < 4; i++) {
            time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // ss, mm, hh, dd
        }
        uint8_t flags[5] = { 0, 0, 0, 0, 0 };
        DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
        DS3231_get_a1(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 66 && cmdsize == 7) {  // "B" Set Alarm 2
        DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A2IE);
        //BMMHHDD
        for (i = 0; i < 4; i++) {
            time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // mm, hh, dd
        }
        uint8_t flags[5] = { 0, 0, 0, 0 };
        DS3231_set_a2(time[0], time[1], time[2], flags);
        DS3231_get_a2(&buff[0], 59);
        Serial.println(buff);
    } else if (cmd[0] == 67 && cmdsize == 1) {  // "C" - get temperature register
        Serial.print("temperature reg is ");
        Serial.println(DS3231_get_treg(), DEC);
    } else if (cmd[0] == 68 && cmdsize == 1) {  // "D" - reset status register alarm flags
        reg_val = DS3231_get_sreg();
        reg_val &= B11111100;
        DS3231_set_sreg(reg_val);
    } else if (cmd[0] == 70 && cmdsize == 1) {  // "F" - custom fct
        reg_val = DS3231_get_addr(0x5);
        Serial.print("orig ");
        Serial.print(reg_val,DEC);
        Serial.print("month is ");
        Serial.println(bcdtodec(reg_val & 0x1F),DEC);
    } else if (cmd[0] == 71 && cmdsize == 1) {  // "G" - set aging status register
        DS3231_set_aging(0);
    } else if (cmd[0] == 83 && cmdsize == 1) {  // "S" - get status register
        Serial.print("status reg is ");
        Serial.println(DS3231_get_sreg(), DEC);
    } else {
        Serial.print("unknown command prefix ");
        Serial.println(cmd[0]);
        Serial.println(cmd[0], DEC);
    }
}