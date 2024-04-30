#include <DS1302.h>                   // RTC 모듈 라이브러리
#include <LiquidCrystal.h>            // LCD 1602 라이브러리
#include <Wire.h>                     // I2C 통신용 라이브러리
#include <LiquidCrystal_I2C.h>        // LCD 1602 I2C용 라이브러리
#include <SPI.h>                      // SPI 통신용 라이브러리
#include <MFRC522.h>                  // MFRC522 모듈 라이브러리
#include <EEPROM.h>                   // EEPROM 라이브러리

// 학생 정보 구조체
struct StudentInfo {
  char studentID[9];                  // 학생 ID - 카드의 UID를 저장
  char name[30];                      // 학생 이름
};

// 핀 할당----------------------------------------------------
// RC522과 아두이노의 연결
#define SS_PIN 10                 // SDA 핀
#define RST_PIN 9                // RST 핀
// RTC모듈과 아두이노의 연결
#define RST 14                   // RESET(RST)핀 (Chip Enable)
#define DATA 15                   // DATA핀 (I/O)
#define CLOCK 16                  // Clock 핀 (Serial Clock)
// 시프트 레지스터
#define dataPin 8                 //SR 14
#define latchPin 7                //SR 12출력 레지스터 핀 (모든 데이터 채워지면 출력핀 문 엶; 처음 low, 다 채워지면 high)
#define shiftPin 6                //SR 11
// 부저
#define speakerPin 17
#define masterKey "7913ec60"      // 관리자 키

// 부품모듈객체생성---------------------------------------------------------------------------
DS1302 rtc(RST, DATA, CLOCK);         // DS1302 객체 생성
MFRC522 rfid(SS_PIN, RST_PIN);        // MFRC522 객체 생성
LiquidCrystal_I2C lcd(0x26,16,2);     // I2C 객체 생성, 접근주소: 0x26

//출석 관련 정보 변수 및 함수------------------------------------------------------------------------------
// 총원, 출석 인원 관리
int Total_Student;
byte Attend_Student = 0;

String hourStr;
String minuteStr;
// 수업 시간
int Class_Hour;
int Class_Minute;
// 출석 true, 지각 false
bool State = true;
//long randNumber;
const int EmoticonCount = 2;

// 도트 매트릭스 맵
byte good[8] = {0B00100000, 0B01001100, 0B00101010, 0B00001010, 0B00001010, 0B00101010, 0B01001100, 0B00100000};
byte bad[8] = {0B01010000, 0B00100010, 0B01010010, 0B00000010, 0B00000010, 0B01010010, 0B00100010, 0B01010000};
byte col[8] = {~0B10000000, ~0B01000000, ~0B00100000, ~0B00010000, ~0B00001000, ~0B00000100, ~0B00000010, ~0B00000001};//열
byte Attend_StudentID[25];       // 출석한 학생의 UID 정보 저장

// 부저 음
int tones[] = {262, 294, 330, 349, 392, 440, 494};

// 요일 코드에 알맞는 문자열 출력
String dayAsString(const Time::Day day) {
  switch (day) {
    case Time::kSunday: return "SUN";
    case Time::kMonday: return "MON";
    case Time::kTuesday: return "TUE";
    case Time::kWednesday: return "WED";
    case Time::kThursday: return "THU";
    case Time::kFriday: return "FRI";
    case Time::kSaturday: return "SAT";
  }
  return "";
}

// 학생 정보 관련 함수-----------------------------------------------------------------------------------------------
// EEPROM에 저장된 학생 정보를 탐색하는 함수
StudentInfo readStudentInfo(char* studentID) {
  StudentInfo student;  // 아무 정보가 저장되지 않은 객체 변수 student
  for (int i = 0; i < EEPROM.length(); i += sizeof(StudentInfo)) {
    EEPROM.get(i, student);  // EEPROM에서 정보를 받아 객체 변수 student에 저장
    
    // 매개변수로 받은 학생 ID와 EEPROM에 저장된 학생 ID가 같으면 해당 학생 정보 반환
    if (strcmp(student.studentID, studentID) == 0) {
      // 이미 중복 출석한 학생인지 확인
      for (int j = 0; j < Attend_Student + 1; j++) {
        if (Attend_StudentID[j] == i / sizeof(StudentInfo)) {
          // 이미 중복 출석한 학생이므로 "Too"로 표시하고 로그 출력
          strcpy(student.studentID, "Too");
          return student;
        }
      }
      
      // 중복 출석이 아닌 경우, 출석 목록에 추가
      Attend_StudentID[Attend_Student] = i / sizeof(StudentInfo);
      return student;
    }
  }
  
  // 탐색에 실패하면 빈 구조체 반환
  strcpy(student.studentID, "None");
  return student;
}

// EEPROM에 학생 정보를 저장하는 함수
void saveStudentInfo(char* studentID, char* studentName) {
  StudentInfo student;                      // StudentInfo 객체 변수 student 선언
  strcpy(student.studentID, studentID);     // 객채 변수 student의 studentID에 매개변수로 받은 카드 UID 저장
  strcpy(student.name, studentName);        // 객체 변수 student의 name에 매개변수로 받은 학생 이름 저장

  // 반복문을 통해 EEPROM에 저장할 공간이 있는지 검사
  for (int i = 0; i < EEPROM.length(); i += sizeof(StudentInfo)) {
    StudentInfo existingStudent;
    EEPROM.get(i, existingStudent);
    if (existingStudent.studentID[0] == '\0') {
      // 빈 슬롯을 찾았으므로 정보를 저장하고 함수 종료
      EEPROM.put(i, student);
      return ;
    }
  }
}

// 날짜, 시간 출력
void printTime() {
  // 칩에 저장된 시간을 읽어옵니다. 
  Time t = rtc.time();
 
  // 날짜 출력 (필요하면 주석 제거)
  char buf[20];
 
  // 시간 출력
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hr, t.min, t.sec);
  lcd.setCursor(0, 1);
  lcd.print(buf);
}

void ShowEmoticon(){
  if (State){
    for(int i = 0; i < 4000; i++){
      digitalWrite(latchPin, LOW);
      shiftOut(dataPin, shiftPin, LSBFIRST, col[i%8]);
      shiftOut(dataPin, shiftPin, LSBFIRST, good[i%8]);
      digitalWrite(latchPin, HIGH);
      digitalWrite(latchPin, LOW);
      shiftOut(dataPin, shiftPin, LSBFIRST, 0B00000000);
      shiftOut(dataPin, shiftPin, LSBFIRST, 0B00000000);
      digitalWrite(latchPin, HIGH);
    }
  }
  else {
    for(int i = 0; i < 4000; i++){
      digitalWrite(latchPin, LOW);
      shiftOut(dataPin, shiftPin, LSBFIRST, col[i%8]);
      shiftOut(dataPin, shiftPin, LSBFIRST, bad[i%8]);
      digitalWrite(latchPin, HIGH);
      digitalWrite(latchPin, LOW);
      shiftOut(dataPin, shiftPin, LSBFIRST, 0B00000000);
      shiftOut(dataPin, shiftPin, LSBFIRST, 0B00000000);
      digitalWrite(latchPin, HIGH);
    }
  }
}

void TimePrint(){
  // 날짜 저장 변수
  char buf[20];
  // 칩에 저장된 시간을 읽기 
  Time t = rtc.time();
  // 요일 코드에 알맞는 문자열 가져오기
  const String day = dayAsString(t.day);
  
  lcd.clear();
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %s", t.yr, t.mon, t.date, day.c_str());
  lcd.print(buf);

  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hr, t.min, t.sec);
  lcd.setCursor(0, 1);
  lcd.print(buf);
  
  delay(1000);
}

void SetSubjectTime(){
  while(1) {
    Serial.println("수업 시작 시와 분을 공백으로 구분하여 입력하세요 (예: 12 13): ");
    String input = "";
    while (1) {
      if (Serial.available() > 0) { // 시리얼 버퍼에 데이터가 있는지 확인
        char received = Serial.read(); // 문자를 읽음
        if (received == '\n') {
          // 줄 바꿈 문자(엔터 키)를 받으면 입력 완료로 처리
          break;
          } 
          else {
            input += received; // 개행 문자가 아닌 경우 문자를 문자열에 추가
          }
        }
      }

    // 공백으로 구분하여 시간과 분을 추출
    int spacePos = input.indexOf(' ');
    if (spacePos >= 0) {
      hourStr = input.substring(0, spacePos);  // input 문자열의 처음부터 공백 문자 앞까지의 부분을 hourStr 문자열에 저장
      minuteStr = input.substring(spacePos + 1); //백 문자 다음부터 문자열 끝까지의 부분을 minuteStr 문자열에 저장

      Class_Hour = hourStr.toInt();  //  hourStr 문자열을 정수로 변환하여 Class_Hour 변수에 저장
      Class_Minute = minuteStr.toInt();  //minuteStr 문자열을 정수로 변환하여 Class_Minute 변수에 저장
      if (Class_Hour > 23 || Class_Hour < 0 || Class_Minute < 0 || Class_Minute > 59){
        Serial.println("올바른 시간을 입력하세요.");
      }
      else {
        Serial.println("수업 시간 설정이 완료됐습니다.");
        break;
      }
    }  
    else {
      Serial.println("올바른 형식으로 입력하세요 (예: 12 13).");
    }
  }
}

void SaveStudentID(){
  char studentID[9];
  char studentName[30];
  String str = "";
  String sava_id;

  Serial.println("학생 정보를 추가합니다.");
  Serial.println("학생증을 태그해주세요.");
  delay(3000);  // 딜레이를 안주면 마스터 키를 인식함

  // 학생증이 태그 될때까지 태그를 받음
  // 태그가 되면 반목문 탈출 (이게 마스터 키도 인식을 해서 고쳐야함) => if문 안에 마스터키가 태그되었는지 확인하는 if문을 추가하면 가능하지 않을까요..?
  while(1){
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      for (int i = 0; i < 4; i++) {
        sava_id += String(rfid.uid.uidByte[i], HEX);
      }
      sava_id.toCharArray(studentID, 9);

      StudentInfo student = readStudentInfo(studentID);
      if (strcmp(student.studentID, "None") != 0){
        Serial.println("이미 저장된 학생입니다.");
        delay(100);
        continue;
      }
      else {
        break;
      }
    }
  }

  Serial.println("학생 이름을 영어로 입력하세요.");
  delay(3000);

  // 시리얼 모니터에 입력할 때까지 기달림
  // 엔터키가 입력되면 종료
  while (1) {
    if (Serial.available() > 0) { // 시리얼 버퍼에 데이터가 있는지 확인
      char received = Serial.read(); // 문자를 읽음
      // 입력된 문자가 알파벳인지 확인 (대문자 또는 소문자)
      if (received == '\n') {
        // 줄 바꿈 문자(엔터 키)를 받으면 입력 완료로 처리
        break;
      } 
      else {
        str += received; // 영어 알파벳을 문자열에 추가
      }  
    }
  }
  str.toCharArray(studentName, 30);       // String -> char 형변환

  // 학생 정보 저장
  saveStudentInfo(studentID, studentName);
  Serial.println("저장했습니다.");
  lcd.clear();
  lcd.print(" StudentID  Save ");
  lcd.setCursor(0, 1);
  lcd.print("    Complete    ");

  // 저장되면 부저 울림
  tone(speakerPin, tones[0]);
  delay(2000);
  noTone(speakerPin);
  lcd.clear();

  // 처음으로 돌아감
  return;
}

void SetSubjectStudent(){
  while (1){
    Serial.println("수업 정원을 입력하시오. (1 ~ 25): ");
    String input = "";

    while (1) {
      if (Serial.available() > 0) { // 시리얼 버퍼에 데이터가 있는지 확인
        char received = Serial.read(); // 문자를 읽음
        if (received == '\n') {
          // 줄 바꿈 문자(엔터 키)를 받으면 입력 완료로 처리
          break;
        } 
        else {
          input += received; // 개행 문자가 아닌 경우 문자를 문자열에 추가
        }
      }
    }

    int inputValue = input.toInt(); // 문자열을 정수로 변환
    if (inputValue != 0) {
      if(inputValue < 1 || inputValue > 25) {
          Serial.println("범위을 벗어난 숫자입니다! ");  
      }
      else {
        Serial.println("정원 설정이 완료됐습니다.");
        for (int i = 0; i < Total_Student; i++) {
          Attend_StudentID[i] = "";
        }
        Total_Student = inputValue;
        Attend_Student = 0;
        Wire.beginTransmission(1);
        Wire.write("&");
        Wire.write(byte(Total_Student));
        Wire.endTransmission();
        break;
      }
    } 
    else {
      // 변환된 값이 0이면, 입력된 문자열이 숫자가 아님
      Serial.println("입력된 값은 숫자가 아닙니다.");
    }
  }
}
void setting(){
   // 저장할 학생의 이름과 아이디 저장할 변수
      // char은 함수로 넘기는 변수, String는 읽은 정보를 저장
      // String로 읽고 char로 변환 후 함수로 넘겨서 EPPROM에 저장
      String num = "";

      Serial.println("1. 학생 정보 추가 2. 수업 변경 3. 종료");
      lcd.clear();
      lcd.print("<Manager    Mode>");
      lcd.setCursor(0, 1);
      lcd.print("- Waiting  Plz --");

      while(1){
        while (1) {
          if (Serial.available() > 0) { // 시리얼 버퍼에 데이터가 있는지 확인
            char received = Serial.read(); // 문자를 읽음
            if (received == '\n') {
              // 줄 바꿈 문자(엔터 키)를 받으면 입력 완료로 처리
              break;
            } 
            else {
              num += received; // 개행 문자가 아닌 경우 문자를 문자열에 추가
            }
          }
        }

        if (num == "1"){
          lcd.clear();
          lcd.print("<Save   Student>");
          lcd.setCursor(0, 1);
          lcd.print("- Waiting  Plz -");
          SaveStudentID();
          return ;
        }
        else if(num == "2"){
          lcd.clear();
          lcd.print("<Subject Change>");
          lcd.setCursor(0, 1);
          lcd.print("- Waiting  Plz -");
          
          SetSubjectTime();
          SetSubjectStudent();
          return ;
        }
        else if(num == "3"){
          return ;
        }
        else {
          Serial.println("잘못 입력하셨습니다.");
          num = "";
        }
      }
}
// setup() ---------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);   // 시리얼 모니터 시작
  Wire.begin();         // I2C 통신 시작

  SPI.begin();          // SPI 통신 시작
  rfid.PCD_Init();      // MFRC522 초기화

  lcd.backlight();      // LCD 백라이트 ON
  lcd.begin(16, 2);     // 사용된 LCD의 글자수

  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(shiftPin, OUTPUT);
  for(int i = 0; i < 8; i++){
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, shiftPin, LSBFIRST, 0B00000000);
    shiftOut(dataPin, shiftPin, LSBFIRST, col[8]);
    digitalWrite(latchPin, HIGH);
  }

  lcd.print("Subject Settings");
  lcd.setCursor(0, 1);
  lcd.print("- Waiting  Plz -");
  
  Serial.println("관리자 키를 태그하세요.");
  while (1){
    while(1){
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
          delay(500);
          break;
      }
    }

    String tag_id;
    for (int i = 0; i < 4; i++) {
      tag_id += String(rfid.uid.uidByte[i], HEX);
    }
    
    if(strcmp(tag_id.c_str(), masterKey) != 0) {
      Serial.println("관리자 키가 아닙니다.");
      delay(500);
      continue;
    }
    else {
      SetSubjectTime();
      SetSubjectStudent();
    }
    break;    
  }
}

//loop() -------------------------------------------------------------------------------------
void loop() {
  TimePrint();
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Time t = rtc.time();      // 카드가 인식된 시간 저장
    
    // 문자열 변수에 uid 저장
    String tag_id;
    for (int i = 0; i < 4; i++) {
      tag_id += String(rfid.uid.uidByte[i], HEX);
    }

    // 마스터 키 인식 -> 학생 정보 저장
    if(strcmp(tag_id.c_str(), masterKey) == 0) {
      setting();
      return ;
    }

    // 학생 출석 인원이 다 차면 더이상 x
    if (Attend_Student == Total_Student) {
      lcd.clear();
      lcd.print("Student capacity");
      lcd.setCursor(0, 1);
      lcd.print("    Exceeded    ");
      delay(1000);
      return;
    }
    
    // UID를 사용하여 학생 정보를 찾음
    char studentID[9];
    tag_id.toCharArray(studentID, 9);
    StudentInfo student = readStudentInfo(studentID);

    // 학생 이름 출력
    // 등록되지 않은 학생증인 경우 Unidentifide 출력 후 return
    if (strcmp(student.studentID, "None") == 0){
      lcd.clear();
      lcd.print("Unidentified ID");
      lcd.setCursor(0, 1);
      lcd.print(" Please Save ID ");
      
      delay(2000);
      lcd.clear();
      return;
    }
    else if (strcmp(student.studentID, "Too") == 0){
      return ;
    }
    // 등록된 학생증은 학생 이름 출력 
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(student.name);
    }

    // 카드가 인식된 시각을 LCD에 출력
    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.hr, t.min, t.sec);
    printTime();

    // 출석, 지각을 처리
    if (t.hr * 60 + t.min < Class_Hour * 60 + Class_Minute){
      Attend_Student += 1;

      for (int i = 0; i < 3; i++) {
        tone(speakerPin, tones[i * 2]);
        delay(250);
      }
      noTone(speakerPin);

      lcd.print(" Attend");
      ShowEmoticon();
      
    }
    else {
      State = false;
      lcd.print(" Late");
      ShowEmoticon();
    }

    // 슬레이브 보드에 정보 전달
    Wire.beginTransmission(1);  // 전송 시작을 알리는 함수                           
    // 학생 정보를 슬레이브 보드의 시리얼 모니터에 출력
    // Wire.write() : 괄호한의 정보를 슬레이브 보드로 전송
    Wire.write("Data,");
    Wire.write(student.name);
    Wire.write(",");
    Wire.write(buf);
    Wire.endTransmission();   // 전송 끝을 알리는 함수

    Wire.beginTransmission(1);  // 전송 시작을 알리는 함수  
    Wire.write(",");                         
    if (State){
      Wire.write("Attend");
    }
    else {
      Wire.write("Late");
    }
    Wire.endTransmission();   // 전송 끝을 알리는 함수
    
    Wire.beginTransmission(1);
    Wire.write(Attend_Student);
    Wire.endTransmission();

    // RFID 통신 종료
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    delay(1000);                  // 딜레이를 이용하여 LCD에 출력된 시간을 확인할 수 있도록 한다.
    lcd.clear();                  // LCD의 모든 내용을 지운 다음에 커서의 위치를 0,0으로 옮긴다.
    State = true;
  }
}