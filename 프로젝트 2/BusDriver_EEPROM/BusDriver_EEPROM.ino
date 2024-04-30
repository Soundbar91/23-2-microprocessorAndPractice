#include <EEPROM.h>                   // EEPROM 라이브러리

// 버스 기사 구조체
struct BusDriver {
  char ID[9];                  // 카드 ID
  int Demerit;                 // 벌점
  int Incentive;               // 상점
  unsigned long Money;
  float gas;
};

void initializeCardDataBase(char* ID, int demerit, int incentive, unsigned long money, float gas) {
  BusDriver busdriver;
  strcpy(busdriver.ID, ID);
  busdriver.Demerit = demerit;
  busdriver.Incentive = incentive;
  busdriver.Money = money;
  busdriver.gas = gas;
  
  saveBusDriverInfo(busdriver);
}

void saveBusDriverInfo(BusDriver busdriverinfo) {
  for (int i = 0; i < EEPROM.length(); i += sizeof(BusDriver)) {
    BusDriver existingCard;
    EEPROM.get(i, existingCard);
    if (existingCard.ID[0] == '\0') {
      EEPROM.put(i, busdriverinfo);
      return ;
    }
  }
}

void setup() {
  Serial.begin(9600);

  initializeCardDataBase("93c6b6f7", 0, 0, 0, 0);

  int numCard = EEPROM.length() / sizeof(BusDriver);
  for (int i = 0; i < numCard; i++) {
    BusDriver card;
    EEPROM.get(i * sizeof(BusDriver), card);

    Serial.print("Card ID: ");
    Serial.println(card.ID);
    Serial.print("Demerit: ");
    Serial.println(card.Demerit);
    Serial.print("Incentive: ");
    Serial.println(card.Incentive);
    Serial.print("Money: ");
    Serial.println(card.Money);
    Serial.print("gas: ");
    Serial.println(card.gas);
  }
}

void loop() {
}