#include <EEPROM.h>                   // EEPROM 라이브러리

// 학생 정보 구조체
struct StudentInfo {
  char studentID[9];                  // 학생 ID - RIFD 키체인, 카드의 UID 저장
  char name[30];                      // 학생 이름
};

void initializeStudentDataBase(char* studentID, char* studentName) {
  StudentInfo student;                      // StudentInfo 객체 변수 student 선언
  strcpy(student.studentID, studentID);     // 객채 변수 studented의 studentID에 매개변수로 받은 카드 UID 저장
  strcpy(student.name, studentName);        // 객체 변수 studented의 name에 매개변수로 받은 학생 이름 저장
  
  saveStudentInfo(student);                 // EEPROM에 학생 정보를 저장
}

// EEPROM에 학생 정보를 저장하는 함수
void saveStudentInfo(StudentInfo student) {
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

void setup() {
  Serial.begin(115200);  // 시리얼 통신 초기화

  // EEPROM에 학생 정보 저장, 매개변수 (카드 UID, 학생 이름)
  // 이미 저장한 학생 정보는 주석 처리
  //initializeStudentDataBase("7913ec60", "Sava Information");
  //initializeStudentDataBase("93c6b6f7", "Shin GwanGyu");
  //initializeStudentDataBase("d33c3711", "Jang Kyungsik");


  // EEPROM에 저장된 모든 학생 정보 출력
  int numStudents = EEPROM.length() / sizeof(StudentInfo);
  // 반복문을 통해 EEPROM에 저장된 학생 정보를 출력
  for (int i = 0; i < numStudents; i++) {
    StudentInfo student;
    EEPROM.get(i * sizeof(StudentInfo), student);

    // 저장된 학생 정보 출력
    Serial.print("Student ID: ");
    Serial.println(student.studentID);
    Serial.print("Name: ");
    Serial.println(student.name);
    Serial.println(); // 학생 정보 사이에 빈 줄 삽입
  }
}

void loop() {
}