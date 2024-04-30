#include <U8glib.h>
#include <MFRC522.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <SoftReset.h>

#define MS_PER_SECOND 1000
#define kpin 18
#define SS_PIN 10
#define RST_PIN 9
#define QuitBus 8
#define StopBus 7
#define LED 6
#define Buzzer 1
const int xpin = A0;
const int ypin = A1;
const int fuelpin = A2;

const float FUEL_CONSUMPTION_RATE = 0.5;
int Tones = 261;

unsigned long timeVal; //이전시간
unsigned long timeVal2;
unsigned long readTime; //현재타이머시간

int speed = 0; // 속도
// 기어 관련 변수
int gear = 0;
int plusgear = 0;
int minusgear = 0;
const int MAX_GEAR = 6; // 최대 기어 단수
const int MAX_SPEED = 80; // 최대 속도 상수

float distance = 0.0; // 버스 이동 거리
int stop = 1;
float busstop = 1;

int count = 0;
float cur = 0;
float fuel;
long randNumber;
bool Stopbell = false;

int door_state = 0; // 전역 변수로 door_state 선언 (0: 닫힘, 1: 열림)

String stopname [6] = {"A", "B", "C", "D", "E", "Fin"}; // 버스 정거장 이름
bool BusStop[5] = {false, false, false, false, false}; // 버스 승차 예약의 유무
String buslocation;  // 버스가 다음에 도착할 정류장
int demerit[5] = {0, 0, 0, 0, 0};  // 버스 기사의 벌점(각 정거장)
const int BusStopCnt = 6; // 버스 정류장의 개수

unsigned long lastDecelerationTime = 0; // 마지막 감속 시간
const unsigned long decelerationInterval = 1000; // 자연 감속 간격

// OLED 관련 변수 
int X = 127;
int Y = 63;
int R = 20; //대시보드 바늘 길이
int minDegree = 0;
int maxDegree = 180;
int degree;
int endX;
int endY;
int door_count = 0;

// 아두이노 2에 정보를 보내기 위한 시간 변수
unsigned long interval = 5000;
unsigned long previousSendMillis = 0;

// 평균속도를 구하기 위한 시간 변수
unsigned long previousMillis = 0;
unsigned long timeInterval = 2000; // 시간 간격 (ms)

String receivedBalance;
unsigned long income;

// 버스 기사 구조체
struct BusDriver {
  char ID[9];                  // 카드 ID
  int Demerit;                 // 벌점
  int Incentive;               // 상점
  unsigned long Money;
  float gas;
};

String tag_id;   // 버스 태그 아이디
BusDriver busdriver; // 현재 운전하고 있는 버스 기사 정보

// 객체
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
LiquidCrystal_I2C lcd(0x23, 16, 2);
MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial mySerial(4, 5);  // 버스 정류장 통신
SoftwareSerial mySerial2(2, 3);  // 버스 요금 결제기 통신

// 버스 계기판 맵
const unsigned char dashboard [] PROGMEM = {
   0x00, 0x01, 0xc0, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
   0x00, 0x07, 0x00, 0x1f, 0xf8, 0x00, 0xc0, 0x00, 0x00, 0x47, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 
   0x00, 0x1e, 0x03, 0xff, 0xff, 0xc0, 0x70, 0x00, 0x00, 0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 
   0x00, 0x38, 0x0f, 0xe1, 0x87, 0xf0, 0x18, 0x00, 0x00, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 
   0x00, 0x60, 0x7c, 0x01, 0x80, 0x3e, 0x06, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x00, 0xc0, 0xf0, 0x01, 0x80, 0x0f, 0x03, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x01, 0x83, 0x80, 0x01, 0x80, 0x01, 0xc1, 0x80, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x03, 0x07, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xc0, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x06, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x30, 0x20, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x30, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x18, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x10, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x18, 0x70, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x08, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x30, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x0c, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x61, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x39, 0x84, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x63, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x70, 0xc4, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0xc3, 0x06, 0x00, 0x00, 0x00, 0x00, 0x60, 0xc2, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe1, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x80, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x80, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x40, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x40, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x40, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x40, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x20, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x20, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x20, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x20, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x20, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x06, 0x20, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x7e, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x7e, 0x20, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x7e, 0x00, 0x00, 0x03, 0xc0, 0x00, 0x00, 0x7e, 0x20, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x06, 0x60, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x60, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0xf0, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xc6, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xe3, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x31, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x18, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x06, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x18, 0x02, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x30, 0x03, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x30, 0x01, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x30, 0x01, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x60, 0x01, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0xc0, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x21, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x8e, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x20, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x03, 0x04, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x10, 0x60, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x08, 0x30, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x06, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x01, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x03, 0x07, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x01, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x01, 0x83, 0x80, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x03, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x00, 0xc0, 0xf0, 0x00, 0x00, 0x0f, 0xe0, 0x00, 0x06, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x00, 0x30, 0x7c, 0x00, 0x00, 0x3e, 0xf0, 0x00, 0x0c, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 
   0x00, 0x08, 0x0f, 0xe0, 0x07, 0xf0, 0xd8, 0x00, 0x19, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 
   0x00, 0x06, 0x03, 0xff, 0xff, 0xc0, 0xcc, 0x00, 0x33, 0x8f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 
   0x00, 0x01, 0x80, 0x1f, 0xf8, 0x01, 0xe7, 0x80, 0xe7, 0xc7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 
   0x00, 0x00, 0x70, 0x00, 0x00, 0x03, 0xf1, 0xff, 0x9f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

// 버스 시동 거는 함수
void startengine() {

  // RFID 인증 대기 메시지 OLED에 출력
  u8g.firstPage(); 
  do {
    u8g.setFont(u8g_font_5x8);
    u8g.setPrintPos(25, 20);
    u8g.print("[BUS SIMULATION]");
    u8g.drawStr(25, 20, "[BUS SIMULATION]");
    u8g.setPrintPos(10, 40);
    u8g.print("Please Enter Your Key");
  } while(u8g.nextPage());

  while (true) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      for (int i = 0; i < 4; i++) {
        tag_id += String(rfid.uid.uidByte[i], HEX);
      }
      char CardID[9];
      tag_id.toCharArray(CardID, 9);
      busdriver = readBusDriverInfo(CardID);
      if (strcmp(busdriver.ID, "None") == 0) {
        // 인증 실패 메시지 OLED에 출력
        displayOLEDMessage("Wrong Key! Try Again.", 0, 20);
        delay(1000);
        tag_id = "";
        continue;
      } else {
        // 인증 성공 메시지 OLED에 출력
        displayOLEDMessage("Access Granted! Starting...", 0, 20);
        delay(1000);
        fuel = busdriver.gas;
        filecur();
        break;
      }
    }
  }
}

void filecur(){
  while(1){
    
    // 대시보드 업데이트
    u8g.firstPage(); 
    do {
      displayDashboardAndBitmap();
      displayDistance();
      displayBoardingNotification();
      displayFuelGraph();
    } while(u8g.nextPage());

    if(digitalRead(fuelpin)==0){
      if (fuel <= 19)fuel += 1;
      else fuel = 20;
    }

    if(digitalRead(StopBus) == 0){
      break;
    }

  }
}

// 버스 기사 정보 읽어오는 함수
BusDriver readBusDriverInfo(char* ID) {
  BusDriver existingCard;
  for (int i = 0; i < EEPROM.length(); i += sizeof(BusDriver)) {
    EEPROM.get(i, existingCard);
    if (strcmp(existingCard.ID, ID) == 0) {
      return existingCard;
    }
  }

  strcpy(existingCard.ID, "None");
  return existingCard;
}

void displayOLEDMessage(String message, int sizeX, int sizeY) {
  u8g.firstPage(); 
  do {
    u8g.setFont(u8g_font_5x8);
    u8g.setPrintPos(sizeX, sizeY);
    u8g.print(message);
  } while (u8g.nextPage());
}

void displayDashboardAndBitmap() {
  // 계기판 표시
  displayDashboard();

  // 비트맵 이미지의 위치 정의
  int bitmapX = 0; // 비트맵의 X 좌표
  int bitmapY = 0; // 비트맵의 Y 좌표
  int bitmapWidth = 128; // 비트맵의 너비 (픽셀 단위)
  int bitmapHeight = 64; // 비트맵의 높이 (픽셀 단위)

  // 비트맵 이미지 표시
  u8g.drawBitmapP(bitmapX, bitmapY, bitmapWidth / 8, bitmapHeight, dashboard);
}

void displayDashboard() {
  // 속도를 대시보드 바늘의 각도로 매핑
  int degree = map(speed, 0, MAX_SPEED, 180, 0);

  // 대시보드 바늘의 끝점 계산
  int centerX = 32;
  int centerY = 33;
  int R = 20; //대시보드 바늘 길이
  int endX, endY;

  endX = centerX + R * cos(radians(degree));
  endY = centerY - R * sin(radians(degree));

  // 대시보드 바늘 그리기
  u8g.drawLine(centerX, centerY, endX, endY);

  // 속도 표시
  u8g.setFont(u8g_font_6x10);
  u8g.setPrintPos(20, 44); // 속도 표시 위치 조정
  u8g.print(speed);
  u8g.setFont(u8g_font_4x6);
  u8g.print("km/h");

  // 현재 기어 단수 표시
  u8g.setFont(u8g_font_5x8);
  u8g.setPrintPos(32, 54);
  u8g.print(gear); 

  u8g.setFont(u8g_font_4x6);
  u8g.setPrintPos(53, 60);
  u8g.print("F"); 

  u8g.setFont(u8g_font_4x6);
  u8g.setPrintPos(64, 46);
  u8g.print("E"); 
}

void displayDistance() {
  // 이동 거리 표시
  u8g.setFont(u8g_font_5x8);
  u8g.setPrintPos(80, 12); // 이동거리 표시 위치 조정
  u8g.print(distance); 
  u8g.setFont(u8g_font_5x8);
  u8g.print("km"); 
}

//승차 예약 인원 알림 출력 함수
void displayBoardingNotification() {
  String ch;
  for (int i = 0; i < 5; i++) {
    if (BusStop[i]) {
      ch += stopname[i];
    }
  }
  u8g.setFont(u8g_font_5x8);
  u8g.setPrintPos(80, 20);  
  u8g.print(ch);
}

// 남은 연료량에 따라 바늘 그래프 그리기
void displayFuelGraph() {
  // 연료량에 따라 각도 매핑
  int graphAngle = map(fuel, 20, 0, 0, 180);

  // 계기판 아래 중앙에 그래프 그리기
  int centerX = 61;
  int centerY = 51;
  int radius = 6; // 그래프 반지름

  int endX, endY;

  // 바늘 끝점 계산
  if (graphAngle <= 315) {
    endX = centerX - radius * cos(radians(315 - graphAngle));
    endY = centerY - radius * sin(radians(315 - graphAngle));
  } else {
    endX = centerX - radius * cos(radians(graphAngle - 315));
    endY = centerY + radius * sin(radians(graphAngle - 315));
  }

  // 바늘 그리기
  u8g.drawLine(centerX, centerY, endX, endY);
}

// 문 애니메이션 출력 함수
void displayDoorAnimation() {
  for (int frame = 0; frame <= 10; frame++) {
    u8g.firstPage();
    do {
      if (door_state == 0) {
        drawOpeningDoorFrame(frame); // 문 열림 애니메이션
      } else {
        drawClosingDoorFrame(frame); // 문 닫힘 애니메이션
      }
    } while (u8g.nextPage());
    
    delay(100);
  }
  // 애니메이션이 완료된 후 door_state 업데이트
  door_state = 1 - door_state;
}

// 문이 열리는 애니메이션 함수
void drawOpeningDoorFrame(int frame) {
  int doorWidth = 40; // 전체 문의 너비
  int doorY = 20; // 문의 Y 위치
  int doorHeight = 40; // 문의 높이
  int openingWidth = frame; // 문이 열리는 정도
  int doorX = 64 - doorWidth / 2; // 문의 X 시작 위치

  // 바닥 그리기
  u8g.drawLine(0, doorY + doorHeight, 128, doorY + doorHeight);

  // 문의 왼쪽 및 오른쪽 부분 그리기
  if (openingWidth < doorWidth / 2) {
    u8g.drawBox(doorX, doorY, doorWidth / 2 - openingWidth, doorHeight); // 왼쪽
    u8g.drawBox(doorX + doorWidth / 2 + openingWidth, doorY, doorWidth / 2 - openingWidth, doorHeight); // 오른쪽
  }

  // 문의 윤곽선 그리기
  u8g.drawFrame(doorX, doorY, doorWidth, doorHeight);
}

//문이 닫히는 애니메이션 함수
void drawClosingDoorFrame(int frame) {
  int doorWidth = 40; // 전체 문의 너비
  int doorY = 20; // 문의 Y 위치
  int doorHeight = 40; // 문의 높이
  int doorX = 64 - doorWidth / 2; // 문의 X 시작 위치

  // 문 닫힘 애니메이션에서는 frame 값이 증가함에 따라 openingWidth가 감소
  int openingWidth = 10 - frame; // 문이 닫히는 정도

  // 바닥 그리기
  u8g.drawLine(0, doorY + doorHeight, 128, doorY + doorHeight);

  // 문의 왼쪽 및 오른쪽 부분 그리기
  if (openingWidth >= 0 && openingWidth < doorWidth / 2) {
    u8g.drawBox(doorX, doorY, doorWidth / 2 - openingWidth, doorHeight); // 왼쪽
    u8g.drawBox(doorX + doorWidth / 2 + openingWidth, doorY, doorWidth / 2 - openingWidth, doorHeight); // 오른쪽
  }

  // 문의 윤곽선 그리기
  u8g.drawFrame(doorX, doorY, doorWidth, doorHeight);
}

// 운행 종료 후 운행정보 출력
void displayDriverInfo() {
  const int animationInterval = 30;  // 애니메이션 갱신 간격 (밀리초)
  const int yPosStart = 64;  // 텍스트가 시작되는 초기 위치
  int cnt = 0;
  int yPos = yPosStart;

  do {
    u8g.firstPage();
    do {
      u8g.setFont(u8g_font_4x6);
      u8g.setPrintPos(20, yPos);
      u8g.print("Driver ID: ");
      u8g.print(busdriver.ID);

      u8g.setPrintPos(20, yPos + 15);
      u8g.print("Incentives: ");
      u8g.print(busdriver.Incentive);

      u8g.setPrintPos(20, yPos + 25);
      u8g.print("Demerits: ");
      u8g.print(busdriver.Demerit);

      u8g.setPrintPos(20, yPos + 35);
      u8g.print("Income : ");
      u8g.print(income);

      u8g.setPrintPos(20, yPos + 45);
      u8g.print("Money: ");
      u8g.print(busdriver.Money);

      yPos -= 1;  // yPos 값을 감소하여 텍스트를 위로 이동

      // yPos 값이 음수가 되면 초기화하여 텍스트가 아래로 다시 나오도록 함
      if (yPos == 0) {
        cnt = 1;
      }

    } while (u8g.nextPage());

    delay(animationInterval);

    if (cnt == 1){
      break;
    }

  } while (1);

  while(1){
    if(digitalRead(StopBus)==0){
      break;
    }
  }

}

// 운행 종료 함수
void engineEnd(){
  for (int i = 0; i < 5; i++){
    if (demerit[i] != 0) busdriver.Demerit += 1;
    else busdriver.Incentive += 1;
  }
  busdriver.gas = fuel;
  busdriver.Money += income + (busdriver.Incentive * 1000 - busdriver.Demerit * 500);

  updateCardInfo(busdriver);
  displayDriverInfo();// OLED에 정보 출력
  
  // 운전 재시작
  soft_restart();
}

// 버스 기사 정보 업데이트하는 함수
void updateCardInfo(BusDriver updatedCard) {
  for (int i = 0; i < EEPROM.length(); i += sizeof(BusDriver)) {
    BusDriver existingCard;
    EEPROM.get(i, existingCard);
    if (strcmp(existingCard.ID, updatedCard.ID) == 0) {
      EEPROM.put(i, updatedCard);
    }
  }
}

void setup(){
  SPI.begin();
  rfid.PCD_Init();

  Serial.end();
  mySerial.begin(9600);

  pinMode(kpin, INPUT_PULLUP);
  pinMode(StopBus, INPUT_PULLUP);
  pinMode(QuitBus, INPUT_PULLUP);
  pinMode(fuelpin, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  randomSeed(analogRead(0));

  u8g.setFont(u8g_font_courB10);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lastDecelerationTime = millis();
  startengine();
}

void handleStopMode() {
  if (BusStop[(int)busstop - 1]) {
    BusStop[(int)busstop - 1] = false;
  }

  delay(250);
  speed = 0;
  gear = 0;
  mySerial.end();
  mySerial2.begin(9600);

  displayDoorAnimation();
  digitalWrite(6, LOW);
  stop = -stop;

  while (1) {
    if (digitalRead(StopBus) == 0) {
      stop = -stop;
      mySerial2.end();
      mySerial.begin(9600);
      displayDoorAnimation();
      break;
    }

    if (mySerial2.available() > 0) {
      char ch = mySerial2.read();
      receivedBalance += ch;
    } else {
      income += receivedBalance.toInt();
      receivedBalance = "";
      delay(500);
    }
  }
}

void loop(){
  
  if(millis()-timeVal2 >= MS_PER_SECOND){
    noTone(Buzzer);
  }

  timeVal = millis();

  if(timeVal-readTime>= MS_PER_SECOND){
    distance += speed/3600;
    readTime = timeVal;
    count++;
  }

  if(count == 10){
    randNumber = random(3);
    count = 0;
  }

  if(distance - cur >= 0.02){
    fuel -= FUEL_CONSUMPTION_RATE;
    cur = distance;
  }

  if ((int)busstop == 6){
    lcd.clear();
    lcd.setCursor(0, 0);                            
    lcd.print("This Stop : "+ stopname[(int)busstop - 1]);
  }
  else if  ((int)busstop > 6){
    lcd.clear();
    lcd.setCursor(0, 0);                            
    lcd.print("End");
  }
  else {
    lcd.setCursor(0, 0);                            
    lcd.print("This Stop : "+ stopname[(int)busstop-1]);                
    lcd.setCursor(0, 1);                            
    lcd.print("Next Stop : "+ stopname[(int)busstop]);
  }

  if(analogRead(ypin) >= 900) plusgear = 1;  //기어+
  if((plusgear == 1)  && (analogRead(ypin) < 600)){
    if(gear != 5)gear++;
    plusgear = 0;
  }
  if(analogRead(ypin) <= 20) minusgear = 1;  //기어-
  if((minusgear == 1)  && (analogRead(ypin) > 100)){
    if(speed==0) gear = 0;
    else if(gear!=0) gear--;
    minusgear = 0;
  }
  if(analogRead(xpin) > 500){ //브레이크
    if (speed == 0);
    else speed--;
  }

  if((gear != 0) && (analogRead(xpin) < 400) && (stop > 0) && fuel != 0){  //기어에 따른 스피드, 정차모드가 아니어야 스피드가 올라감
    if(gear == 1){
      if(speed < 10)speed++;
      else if(speed > 10)speed--;
      }
    else if(gear == 2){
      if(speed < 20)speed++;
      else if(speed > 20)speed--;
      }
    else if(gear == 3){
      if(speed < 40)speed++;
      else if(speed > 40)speed--;
      }
    else if(gear == 4){
      if (randNumber == 0){
        if(speed < 40)speed++;
        else if(speed > 40)speed--;
      }
      else{
      if(speed < 60)speed++;
      else if(speed > 60)speed--;
      }
    }
    else if(gear == 5){
      if (randNumber == 0){
        if(speed < 40)speed++;
        else if(speed > 40)speed--;
      }
      else if (randNumber == 1){
        if(speed < 60)speed++;
        else if(speed > 60)speed--;
      }
      else{
      if(speed < 80)speed++;
      else if(speed > 80)speed--;
      }
    }
  }
  else{ //가만히 있으면 속도 감소
    if (speed == 0);
    else speed--;
  }
  
  if(digitalRead(QuitBus)==0 && Stopbell == false){
    digitalWrite(LED, HIGH); //하차벨
    timeVal2 = millis();
    tone(Buzzer, Tones);
    Stopbell = true;
  }

  if((digitalRead(StopBus)==0) && (abs(distance*10-busstop)<=0.1)){  //정차모드
    Stopbell = false;
    handleStopMode();
  }

  if((digitalRead(StopBus)==0) && (stop < 0)){
    stop = 1;
  }

  // 승차 예약이 있는데 그냥 지나 간 경우 벌점 부여
  if ((int)busstop - 1 >= 1) {
    if (BusStop[(int)busstop - 1 - 1] == true && buslocation == stopname[(int)busstop - 1]) {
      if (demerit[(int)busstop - 1] == 0){
        demerit[(int)busstop - 1 - 1] = 1;
      }
    }
    else if(Stopbell == true && buslocation == stopname[(int)busstop - 1]){
      Stopbell == false;
      digitalWrite(6, LOW);
    }
  }

  if((distance*10-busstop) > 0.2){
    if(busstop == 6) {
      engineEnd();
    }
    busstop++;
  }

  // 평균 속도를 위한 시간 계산
  unsigned long currentMillis = millis();
  unsigned long elapsedTime = currentMillis - previousMillis;

  // 이동거리 계산
  float speed_kph = speed * 0.036; // 속도를 km/h로 변환
  float deltaDistance = speed_kph * (elapsedTime / 3600000.0); // 이동거리를 km로 계산
  distance += deltaDistance;

  // 평균속도
  float totalElapsedTime = previousMillis / 3600000.0;
  float averageSpeed = distance / totalElapsedTime;

  // 대시보드 업데이트
  u8g.firstPage(); 
  do {
    displayDashboardAndBitmap();
    displayDistance();
    displayBoardingNotification();
    displayFuelGraph();
  } while(u8g.nextPage());

  if (elapsedTime >= timeInterval) {
    previousMillis = currentMillis; // 다음 간격을 위해 현재 시간 저장
  }

  // 아두이노2에 이동 거리와 평균 속도 데이터 전송
  if (currentMillis - previousSendMillis >= interval) {
    previousSendMillis = currentMillis;
    mySerial.write(int(distance * 100));
    mySerial.write(int(randNumber));
    mySerial.write(int(averageSpeed * 10));
  }

  // 아두이노 1에서 승차 예약 정보를 받아옴
  if (mySerial.available() > 0){
    int Data = mySerial.read();
    BusStop[Data] = true;
  }

  // 다음에 도착할 버스 정류장
  buslocation = stopname[(int)busstop-1];

  delay(1);            
}