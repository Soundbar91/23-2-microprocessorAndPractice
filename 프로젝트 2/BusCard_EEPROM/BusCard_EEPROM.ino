#include <EEPROM.h>                   // EEPROM 라이브러리

// 버스카드 구조체
struct CardInfo {
  char ID[9];                  // 카드 ID
  unsigned long Balance;                 // 잔액
  int Age;
};

void initializeCardDataBase(char* ID, unsigned long pay, int age) {
  CardInfo card;
  strcpy(card.ID, ID);
  card.Balance = pay;
  card.Age = age;
  
  saveCardInfo(card);
}

void saveCardInfo(CardInfo cardinfo) {
  for (int i = 0; i < EEPROM.length(); i += sizeof(CardInfo)) {
    CardInfo existingCard;
    EEPROM.get(i, existingCard);
    if (existingCard.ID[0] == '\0') {
      EEPROM.put(i, cardinfo);
      return ;
    }
  }
}

void setup() {
  Serial.begin(9600);

  //initializeCardDataBase("93c6b6f7", 90000, 23);

  int numCard = EEPROM.length() / sizeof(CardInfo);
  for (int i = 0; i < numCard; i++) {
    CardInfo card;
    EEPROM.get(i * sizeof(CardInfo), card);

    Serial.print("Card ID: ");
    Serial.println(card.ID);
    Serial.print("Balance: ");
    Serial.println(card.Balance);
    Serial.print("Age: ");
    Serial.println(card.Age);
    Serial.println();
  }
}

void loop() {
}