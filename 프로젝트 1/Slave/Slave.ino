#include <Wire.h>         // I2C 통신 라이브러리

// 시프트 레지스터
#define dataPin 8                 //SR 14
#define latchPin 7                //SR 12출력 레지스터 핀(모든 데이터 채워지면 출력핀 문 엶; 처음 low, 다 채워지면 high)
#define shiftPin 6                //SR 11 CLOCK
//7세그먼트 선택 핀
int digit_select_pin[] = {2, 3, 4, 5};

// 총원, 출석 인원 관리
int Total_Student = 0;
int Attend_Student = 0;

int count = 0;        // I2C 통신을 위한 카운터 변수 (학생 수만 읽기 위해)

// FND 출력 함수
void show_digit(int pos, int number){
 int a[10] = {252, 96, 218, 242, 102, 182, 190, 224, 254, 246}; //0~9 10진수로 표현

  for(int i = 0; i < 4; i++)
  {
    if(i == pos)
      //해당 자릿수의 선택 핀만 LOW로 설정
      digitalWrite(digit_select_pin[i], LOW);
    else
      //나머지 자리는 HIGH로 설정
      digitalWrite(digit_select_pin[i], HIGH);
  }
  if(pos == 0){
    if(number < 10) shiftOut(dataPin, shiftPin, LSBFIRST, 0); 
    else shiftOut(dataPin, shiftPin, LSBFIRST, a[number / 10]);
  }
  else if(pos == 1) {
    shiftOut(dataPin, shiftPin, LSBFIRST, a[number % 10]);
  }
  else if(pos == 2) {
    shiftOut(dataPin, shiftPin, LSBFIRST, a[Total_Student / 10]);
  }  
  else if(pos == 3) {
    shiftOut(dataPin, shiftPin, LSBFIRST, a[Total_Student % 10]);
  }
}

//전송 데이터 읽기
void receiveEvent(int howMany) {
  // 전송 받은 데이터를 검사
  while (Wire.available()>=1) {
    // 데이터가 있으면 읽어어고 시리얼 모니터에 출력
    char ch = Wire.read();
    if (count == 2) {
      Attend_Student = int(ch);
      count = 0;
      return ;   
    }
    else if(ch == '&'){
      char ch1 = Wire.read();
      Serial.println("Data,-,Subject Change,-");
      Total_Student = int(ch1);
      Attend_Student = 0;
      return ;
    }
    else {
      Serial.print(ch);
    }
  }
  count += 1;
  if(count == 2) Serial.println();
}

void print(){
  for(int i=0; i < 4; i++){
    digitalWrite(latchPin, LOW);
    show_digit(i, Attend_Student);//4자리 출력
    digitalWrite(latchPin, HIGH);
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, shiftPin, LSBFIRST, 0b00000000);
    digitalWrite(latchPin, HIGH);
    delay(1);
  }
}

void setup() {
  Wire.begin(1);                    //슬레이브 주소                
  Wire.onReceive(receiveEvent);     //데이터 전송 받을 때 receiveEvent함수 호출
  
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(shiftPin, OUTPUT);
  

  for(int i = 0; i < 4; i++)
  {
    pinMode(digit_select_pin[i], OUTPUT);
  }
  
  Serial.begin(9600);         
  Serial.println("CLEARDATA");            // 엑셀 연동을 위한 문구 출력
  Serial.println("LABEL,Name,Time,State");   // 엑셀에 연동할 데이터 이름 출력, 쉼표는 엑셀에서 행을 구분           
}

void loop() {
  print(); 
}