#include <SoftwareSerial.h>
#include <Keypad.h>
#include <SPI.h>                      // SPI 통신용 라이브러리
#include <Wire.h>
#include <LiquidCrystal_I2C.h>        // LCD 1602 I2C용 라이브러리
#include <MFRC522.h>                  // MFRC522 모듈 라이브러리
#include <EEPROM.h>

/*
  키패드 
  C1, C2, C3, C4 : D3, D2, D1, D0
  R1, R2, R3, R4 : D7, D6, D5, D4

  RFID
  SDA : D10
  SCK : D13
  MOSI : 11
  MISO : 12
  RST : 9

  LCD (AREF 위 PC4, PC5 / lcd 주소 납땜 안했으면 0x27로 해야함)
  SCL : PC5
  SDA : PC4
  국가유공자, 노인 추가
*/

// RC522과 아두이노의 연결
#define SS_PIN 10                // SDA 핀
#define RST_PIN 9                // RST 핀

// 버스카드 구조체
struct CardInfo {
  char ID[9];                  // 카드 ID
  unsigned long Balance;                 // 잔액
  int Age;
};

const byte ROWS = 4; 
const byte COLS = 4;
 
char keys[ROWS][COLS] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'#','0','*', 'D'}
};
 
byte rowPins[ROWS] = { 8, 7, 0, 1 };
byte colPins[COLS] = { 16, 15, 14, 17};

SoftwareSerial mySerial1(2, 3);  // 버스 정류장
SoftwareSerial mySerial2(5, 6);  // 버스
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
LiquidCrystal_I2C lcd(0x26,16,2);
MFRC522 rfid(SS_PIN, RST_PIN);

String receivedData;

CardInfo readCardInfo(char* ID) {
  CardInfo existingCard;
  for (int i = 0; i < EEPROM.length(); i += sizeof(CardInfo)) {
    EEPROM.get(i, existingCard);
    if (strcmp(existingCard.ID, ID) == 0) {
      return existingCard;
    }
  }

  strcpy(existingCard.ID, "None");
  return existingCard;
}

void updateCardInfo(CardInfo updatedCard) {
  for (int i = 0; i < EEPROM.length(); i += sizeof(CardInfo)) {
    CardInfo existingCard;
    EEPROM.get(i, existingCard);
    if (strcmp(existingCard.ID, updatedCard.ID) == 0) {
      EEPROM.put(i, updatedCard);
    }
  }
}

void PaymentForMultiple(){
  lcd.clear();
  int adult = 0;
  int student = 0;
  int child = 0;

  lcd.print("Adult : ");
  lcd.print(adult);

  while(1){

    char key = kpd.getKey();

    if (key){
      if(key == '1') {
        lcd.clear();
        adult += 1;
        lcd.print("Adult : ");
        lcd.print(adult);
      }
      else if(key == '2'){
        lcd.clear();
        adult -= 1;
        lcd.print("Adult : ");
        lcd.print(adult);
      }
      else if(key == 'D'){
        break;
      }
    }

  }

  lcd.clear();
  lcd.print("Student : ");
  lcd.print(student);

  while(1){
    
    char key = kpd.getKey();

    if (key){
      if(key == '1') {
        lcd.clear();
        student += 1;
        lcd.print("Student : ");
        lcd.print(student);
      }
      else if(key == '2'){
        lcd.clear();
        student -= 1;
        lcd.print("Student : ");
        lcd.print(student);
      }
      else if(key == 'D'){
        break;
      }
    }

  }

  lcd.clear();
  lcd.print("Child : ");
  lcd.print(child);

  while(1){
    
    char key = kpd.getKey();

    if (key){
      if(key == '1') {
        lcd.clear();
        child += 1;
        lcd.print("Child : ");
        lcd.print(child);
      }
      else if(key == '2'){
        lcd.clear();
        child -= 1;
        lcd.print("Child : ");
        lcd.print(child);
      }
      else if(key == 'D'){
        break;
      }
    }

  }

  lcd.clear();
  lcd.print("Tag Card");
  while(1){
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String Tag_id;
      for (int i = 0; i < 4; i++) {
        Tag_id += String(rfid.uid.uidByte[i], HEX);
      }

      char CardID[9];
      Tag_id.toCharArray(CardID, 9);
      CardInfo Card = readCardInfo(CardID);

      if (strcmp(Card.ID , "None") == 0) {
        lcd.clear();
        lcd.print("Unidentified ID");
        lcd.setCursor(0, 1);
        lcd.print(" Please Save ID ");
        delay(2000);
        lcd.clear();
        lcd.print("Tag Card");
      }
      else {
        unsigned long fare = adult * 1400 + student * 1100 + child * 650;

        if (Card.Balance >= fare) {
          Card.Balance -= fare;
          updateCardInfo(Card);

          lcd.clear();
          lcd.print("Payment : ");
          lcd.print(fare);
          lcd.setCursor(0, 1);
          lcd.print("Balance : ");
          lcd.print(Card.Balance);
          delay(1500);
          lcd.clear();
          lcd.print("Mode: Payment");

          char buffer[20];
          ultoa(fare, buffer, 10);
          mySerial2.write(buffer);

          return ;
        } 
        else {
          lcd.clear();
          lcd.print("Your balance is");
          lcd.setCursor(0, 1);
          lcd.print("  insufficient");
          delay(1500);
          lcd.clear();
          lcd.print("Mode: Payment");
        }

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
    }
  }
}

void calcBusBay(){
  mySerial1.end();
  mySerial2.begin(9600);
  
  lcd.clear();
  lcd.print("Mode: Payment");
  while(1){

    char key = kpd.getKey();
    if(key){
      if(key == 'D'){
        lcd.clear();
        mySerial2.end();
        mySerial1.begin(9600);
        return ;
      }
      else if(key == 'B'){
        PaymentForMultiple();
        continue;
      }
    }

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String Tag_id;
      for (int i = 0; i < 4; i++) {
        Tag_id += String(rfid.uid.uidByte[i], HEX);
      }

      char CardID[9];
      Tag_id.toCharArray(CardID, 9);
      CardInfo Card = readCardInfo(CardID);

      if (strcmp(Card.ID , "None") == 0) {
        lcd.clear();
        lcd.print("Unidentified ID");
        lcd.setCursor(0, 1);
        lcd.print(" Please Save ID ");
        delay(2000);
        lcd.clear();
      }
      else {
        unsigned long fare;
        if (Card.Age < 8) {
          fare = 650;
        } 
        else if (Card.Age >= 8 && Card.Age < 19) {
          fare = 1100;
        } 
        else {
          fare = 1400;
        }

        if (Card.Balance >= fare) {
          Card.Balance -= fare;
          updateCardInfo(Card);

          lcd.clear();
          lcd.print("Payment : ");
          lcd.print(fare);
          lcd.setCursor(0, 1);
          lcd.print("Balance : ");
          lcd.print(Card.Balance);
          
          char buffer[20];
          ultoa(fare, buffer, 10);
          mySerial2.write(buffer);

          delay(1500);
          lcd.clear();
          lcd.print("Mode: Payment");
          
        } 
        else {
          lcd.clear();
          lcd.print("Your balance is");
          lcd.setCursor(0, 1);
          lcd.print("  insufficient");
          delay(1500);
          lcd.clear();
          lcd.print("Mode: Payment");
        }

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
    }
  }
}

unsigned long inputAmount() {
  String amountStr = ""; // 입력 받은 금액을 저장할 문자열

  while (1) {
    char key = kpd.getKey(); // 키 입력 받아오기

    if (key >= '0' && key <= '9') {
      amountStr += key; // 0부터 9까지의 키 값인 경우에만 문자열에 추가
      lcd.print(key);
    } 
    else if (key == 'D') {
      break; // 입력 종료
    }
  }

  lcd.clear();
  lcd.print("Tag Card");
  return strtoul(amountStr.c_str(), NULL, 10);
}

void CardRecharge() {
  lcd.clear();
  lcd.print("Charge Amount");
  lcd.setCursor(0, 1);

  unsigned long rechargeAmount = inputAmount();

  while (1) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      String Tag_id;
      for (int i = 0; i < 4; i++) {
        Tag_id += String(rfid.uid.uidByte[i], HEX);
      }

      char CardID[9];
      Tag_id.toCharArray(CardID, 9);
      CardInfo Card = readCardInfo(CardID);

      if (strcmp(Card.ID, "None") == 0) {
        lcd.clear();
        lcd.print("Unidentified ID");
        lcd.setCursor(0, 1);
        lcd.print(" Please Save ID ");
        delay(2000);
        lcd.clear();
        lcd.print("Tag Card");
        continue;
      } 
      else {
        lcd.clear();
        lcd.print("Before:");
        lcd.print(Card.Balance);
        lcd.setCursor(0, 1);
        Card.Balance += rechargeAmount;
        updateCardInfo(Card);
        lcd.print("After:");
        lcd.print(Card.Balance);
        delay(1500);
        lcd.clear();

        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        return;
      }
    }
  }
}

void setup(){
  mySerial1.begin(9600);
  Wire.begin();         // I2C 통신 시작
  SPI.begin();          // SPI 통신 시작
  rfid.PCD_Init();      // MFRC522 초기화
  Serial.end();
  lcd.backlight();      // LCD 백라이트 ON
  lcd.begin(16, 2);     // 사용된 LCD의 글자수

  Wire.begin();
}

void loop(){
  lcd.print("A. Fare Payment");
  lcd.setCursor(0, 1);
  lcd.print("B. Card Recharge");

  if (mySerial1.available() > 0) {
    receivedData += char(mySerial1.read());

    if (receivedData.length() == 8){
      char CardID[9];
      receivedData.toCharArray(CardID, 9);
      CardInfo Card = readCardInfo(CardID);
      char buffer[20];
      if (strcmp(Card.ID, "None") == 0){
        mySerial1.write("None");
      }
      else{
        ultoa(Card.Balance, buffer, 10);
        mySerial1.write(buffer);
      }
      receivedData = "";
    }
  }

  char key = kpd.getKey();
  if(key){
    if(key == 'A'){
      calcBusBay();
      return ;
    }
    else if(key == 'B'){
      CardRecharge();
      return ;
    }
  }
}