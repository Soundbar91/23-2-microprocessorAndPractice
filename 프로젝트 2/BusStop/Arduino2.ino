#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <DS1302.h> 
#include <EEPROM.h>
#include <SoftReset.h>

#define SS_PIN 10
#define RST_PIN 9
#define ChangeBusStop_Pin 8
#define Menu_Pin 7
#define Buzzer 6
#define CLOCK 16
#define DATA 15
#define RST 14
#define kpin 2

String BusStop[] = {"A", "B", "C", "D", "E"};  // 버스 정거장 이름
String CurrentBusStop;  // 현재 버스 정거장 이름
int BusStopNum = 5;     // 버스 정거장 개수
int BusStopCnt = 0;     // 현재 버스 정거장
float BusStopLocation[] = {0.1, 0.2, 0.3, 0.4, 0.5};   // 버스 정거장 좌표
int CurrentBusStopLocation;                     // 현재 버스 정거장 좌표
int BusStopTime[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};            // 버스 정거장 도착 예정 시간
bool BusBookNum[] = {false, false, false, false, false};      // 버스 정거장 예약 유무
bool buzzer[5][3] = 
{{false, false, false},
 {false, false, false},
 {false, false, false},
 {false, false, false},
 {false, false, false}};

bool Start = false;  // 버스 출발 여부
unsigned long previousMillis = 0;  // 이전 시간 저장
unsigned long timeInterval = 1000;  // 다음 시간
float distance = 0;  // 버스 이동 거리
long interval = 0;
long interval1 = 0;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;

float averageSpeed;  // 평균 속도
String receivedData;  // 아두이노 1에서 받은 데이터
String receivedBalance;  // 아두이노 3에서 받은 데이터
int cnt = 0;   // 통신용 변수
int Congestion; // 도로 혼잡도

LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(4, 5);  // 버스 통신
SoftwareSerial mySerial1(2, 3);  // 버스 요금 결제기 통신
MFRC522 rfid(SS_PIN, RST_PIN);
DS1302 rtc(RST, DATA, CLOCK);

byte ch1[8] = {
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};

byte ch2[8] = {
  B11000,
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte ch3[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B01111,
  B00111,
  B00011
};

byte ch4[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};

byte ch5[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11110,
  B11100,
  B11000
};

byte ch6[8] = {
  B10001,
  B11111,
  B11111,
  B00000,
  B11111,
  B00100,
  B10000,
  B11111
};

byte bus[8] = {
  B10001,
  B11111,
  B10001,
  B10001,
  B11111,
  B10101,
  B11111,
  B01010
};

byte stop[8] = {
  B11111,
  B10001,
  B11111,
  B10001,
  B10001,
  B10001,
  B11111,
  B01010
};

void setup() {
  Serial.end();
  mySerial.begin(9600);
  pinMode(ChangeBusStop_Pin, INPUT);
  pinMode(Menu_Pin, INPUT);
  pinMode(Buzzer, OUTPUT);
  CurrentBusStop = BusStop[BusStopCnt];
  CurrentBusStopLocation = BusStopLocation[BusStopCnt];

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();
  lcd.createChar(0, ch1);
  lcd.createChar(1, ch2);
  lcd.createChar(2, ch3);
  lcd.createChar(3, ch4);
  lcd.createChar(4, ch5);
  lcd.createChar(5, ch6);
  lcd.createChar(6, bus);
  lcd.createChar(7, stop);

  rfid.PCD_Init();
  SPI.begin();
  Wire.begin();
}

// 버스 예약 함수
void BookBus(){
  delay(500);
  lcd.clear();
  lcd.print("Book Bus?");
  lcd.setCursor(0, 1);
  lcd.print("A. Yes     B. No");

  while (1){
    int val = digitalRead(ChangeBusStop_Pin);
    int val1 = digitalRead(Menu_Pin);

    // A. Yes
    if (val == HIGH) {
      BusBookNum[BusStopCnt] = true;
      mySerial.write(BusStopCnt);
      delay(500);
      return ;
    }

    // B. No
    if (val1 == HIGH){
      return ;
    }
  }

}

// 버스 노선도 출력 함수
void PrintBusMap(){
  delay(250);
  // 버스 정거장 출력 부분
  lcd.clear();
  lcd.write(7);
  lcd.print("----");
  for(int i = 0; i < 5; i++){
    lcd.print(BusStop[i]);
    lcd.print("----");
  }
  lcd.write(7);

  // 스위치 입력
  while(1){
    // 아두이노 1으로부터 이동거리와 속도를 넘겨받음
    if (mySerial.available() > 0) {
      receivedData = mySerial.read();
      cnt += 1;
      if (cnt == 1){
        distance = receivedData.toInt() * 0.01;
        receivedData = "";
      }
      else if (cnt == 2){
        Congestion = receivedData.toInt();
        receivedData = "";
      }
      else {
        averageSpeed = receivedData.toInt() * 0.1;
        receivedData = "";
        cnt = 0;
      }
    }

    if (Congestion == 0) {
      interval = 250;
      interval1 = 500;
    }
    else if (Congestion == 1) {
      interval = 750;
      interval1 = 1000;
    }
    else {
      interval = 0;
      interval1 = 0;
    }

    unsigned long currentMillis = millis();

    if (interval != 0 && interval1 != 0){
      if (currentMillis - previousMillis1 >= interval) {
        // 버스 출력 부분
        lcd.setCursor(0, 1);
        int n = int(distance * 100) / 10;
        for (int i = 0; i < n; i++){
          lcd.print("     ");
        }

        int u = int(distance * 100) % 10 / 2;
        for (int i = 0; i < u; i++){
          lcd.print(" ");
        }
        lcd.write(6);
        previousMillis1 = currentMillis;
      }

      if (currentMillis - previousMillis2 >= interval1) {
        // 추가로 5초에 한 번 실행되는 부분
        lcd.setCursor(0, 1);
        lcd.print("                        ");  // 두 번째 줄 내용 지우기
        previousMillis2 = currentMillis;  // 두 번째 갱신 주기에만 적용
      }
    }
    
    else {
      lcd.setCursor(0, 1);
      int n = int(distance * 100) / 10;
      for (int i = 0; i < n; i++){
        lcd.print("     ");
      }

      int u = int(distance * 100) % 10 / 2;
      for (int i = 0; i < u; i++){
        lcd.print(" ");
      }
      lcd.write(6);
    }

    int val = digitalRead(ChangeBusStop_Pin);
    int val1 = digitalRead(Menu_Pin);

    if (val == HIGH){
      lcd.scrollDisplayLeft();
      delay(250);
    }

    if (val1 == HIGH){
      lcd.clear();
      delay(250);
      break;
    }
  }
}

// 메뉴 모드
void MenuMode(){
  delay(500);
  lcd.clear();
  lcd.print("A. BusBook");
  lcd.setCursor(0, 1);
  lcd.print("B. BusRouteMap");

  while (1){
    int val = digitalRead(ChangeBusStop_Pin);
    int val1 = digitalRead(Menu_Pin);

    // A. 예약 모드
    if (val == HIGH) {
      BookBus();
      return ;
    }

    // B. 노선도 출력
    if (val1 == HIGH){
      PrintBusMap();
      return ;
    }

  }

}

void MainPrint(){
  Time t = rtc.time();
  char buf[12];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hr, t.min, t.sec);
  lcd.clear();
  lcd.write(0);
  lcd.write(0);
  lcd.write(1);
  lcd.print(" ");
  lcd.write(0);
  lcd.write(0);
  lcd.write(1);
  lcd.print(" ");
  lcd.print("[");
  lcd.print(CurrentBusStop);
  lcd.print("]");
  if (BusStopTime[BusStopCnt] == 0 && Start == false){
    lcd.print(" ---");
  }
  else if (BusStopTime[BusStopCnt] == -1){
    lcd.print(" ---");
  }
  else if(BusStopTime[BusStopCnt] == 0 && Start == true) {
    lcd.print("Soon");
  }
  else {
    lcd.print(" ");
    lcd.print(BusStopTime[BusStopCnt]);
    lcd.write(5);
  }
  lcd.setCursor(0, 1);
  lcd.write(2);
  lcd.write(3);
  lcd.write(3);
  lcd.print(" ");
  lcd.write(3);
  lcd.write(3);
  lcd.write(4);
  lcd.print(" ");
  lcd.print(buf);
}

void loop() {

  // 아두이노 1으로부터 이동거리와 속도를 넘겨받음
  if (mySerial.available() > 0) {
    receivedData = mySerial.read();
    cnt += 1;
    if (cnt == 1){
      distance = receivedData.toInt() * 0.01;
      receivedData = "";
    }
    else if (cnt == 2){
      Congestion = receivedData.toInt();
      receivedData = "";
    }
    else {
      averageSpeed = receivedData.toInt() * 0.1;
      receivedData = "";
      cnt = 0;
    }
  }

  if (distance > 0.0){
    Start = true;
  }

  // 카드 금액 조회
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    mySerial.end();
    mySerial1.begin(9600);
    String Tag_id;
    for (int i = 0; i < 4; i++) {
        Tag_id += String(rfid.uid.uidByte[i], HEX);
    }
    char CardID[9];
    Tag_id.toCharArray(CardID, 9);
    mySerial1.write(CardID);
    delay(500);

    while(1){
      if (mySerial1.available() > 0){
        char ch = mySerial1.read();
        receivedBalance += ch;
      }
      else {
        if (receivedBalance == "None"){
          lcd.clear();
          lcd.print("Unidentified ID");
          lcd.setCursor(0, 1);
          lcd.print(" Please Save ID ");
          delay(1000);
          lcd.clear();
        }
        else {
          lcd.clear();
          lcd.print("Card Balance");
          lcd.setCursor(0, 1);
          lcd.print(receivedBalance);
          delay(1000);
          lcd.clear();
        }
        receivedBalance = "";
        mySerial1.end();
        mySerial.begin(9600);
        break;
      }
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(100);
    return ;
  }
  
  unsigned long currentMillis = millis();
  unsigned long elapsedTime = currentMillis - previousMillis;

  // 현재 정거장에 버스가 도착할 시간 계산
  float remainingDistance = BusStopLocation[BusStopCnt] - distance;
  if (remainingDistance / averageSpeed * 60 < 0) {
     BusStopTime[BusStopCnt] = -1;
  }
  else if (remainingDistance / averageSpeed * 60 > 0 && remainingDistance / averageSpeed * 60 < 1){
    BusStopTime[BusStopCnt] = 0;
  }
  else {
    BusStopTime[BusStopCnt] = round(remainingDistance / averageSpeed * 60);
  }

  // 3분, 2분, 1분 남았을 때 부저소리
  if (BusStopTime[BusStopCnt] == 3 && buzzer[BusStopCnt][0] == false){
    tone(Buzzer, 254);
    delay(250);
    noTone(Buzzer);
    buzzer[BusStopCnt][0] = true;
  }
  if (BusStopTime[BusStopCnt] == 2 && buzzer[BusStopCnt][1] == false){
    tone(Buzzer, 254);
    delay(250);
    noTone(Buzzer);
    buzzer[BusStopCnt][1] = true;
  }
  if (BusStopTime[BusStopCnt] == 1 && buzzer[BusStopCnt][2] == false){
    tone(Buzzer, 254);
    delay(250);
    noTone(Buzzer);
    buzzer[BusStopCnt][2] = true;
  }

  // 메인화면 출력
  MainPrint();

  // 정거장에 정차했을 때, 승차 예약인원 초기화
  if (distance > BusStopLocation[BusStopCnt] - 10 && distance < BusStopLocation[BusStopCnt] + 10){
    BusBookNum[BusStopCnt] = false;
  }

  // 다음 정거장으로 변경
  int val = digitalRead(ChangeBusStop_Pin);
  if (val == HIGH) {
    BusStopCnt = (BusStopCnt + 1) % BusStopNum;
    CurrentBusStop = BusStop[BusStopCnt];
    CurrentBusStopLocation = BusStopLocation[BusStopCnt];
    delay(100);
  }

  // 메뉴 모드
  int val1 = digitalRead(Menu_Pin);
  if (val1 == HIGH) {
    MenuMode();
  }

  if (elapsedTime >= timeInterval) {
    previousMillis = currentMillis; // 다음 간격을 위해 현재 시간 저장
  }

  if (distance > 0.63){
    soft_restart();
  }

  delay(250);
}