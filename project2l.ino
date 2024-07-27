#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h> 
#include <SoftwareSerial.h>
#include <Keypad.h>
//#include <Servo.h> 
#include <Wire.h> 
#include <EEPROM.h>
#include <MFRC522.h>
#include <Wire.h>
#include <WiFi.h>
#include <time.h>
#include <ESP32Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define MODEM_RX 16
#define MODEM_TX 17
#define mySerial Serial2 // use for ESP32

Servo s1;

#define RST_PIN 15
#define SS_PIN  5

#define STATE_STARTUP       0
#define STATE_STARTING      1
#define STATE_WAITING       2
#define STATE_SCAN_INVALID  3
#define STATE_SCAN_VALID    4
#define STATE_SCAN_MASTER   5
#define STATE_ADDED_CARD    6
#define STATE_REMOVED_CARD  7
#define STATE_UNLOCKED      8

const int cardArrSize = 10;
const int cardSize    = 4;
const int eepromSize = cardArrSize * cardSize + cardSize;

const int eepromMasterCardStart = 10;
const int eepromCardsStart = eepromMasterCardStart + cardSize;

byte cardArr[cardArrSize][cardSize];
byte masterCard[cardSize];
byte readCard[cardSize];
byte cardsStored = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);

byte currentState = STATE_STARTUP;
unsigned long LastStateChangeTime;
unsigned long StateWaitTime;


const byte numRows= 4; 
const byte numCols= 4; 
char keypressed;
char code[4]; 
char c[4]; 
int ij;


int k = 0;
char keymap[numRows][numCols]=
{
  {'1', '2', '3', 'A'}, 
  {'4', '5', '6', 'B'}, 
  {'7', '8', '9', 'C'}, 
  {'*', '0', '#', 'D'}
};
byte rowPins[numRows] = {33,25,26,14}; 
byte colPins[numCols]={27,13,22,4};

Keypad myKeypad= Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
uint8_t id;
bool displayUpdated = false;

const char* ssid     = "T4";
const char* password = "77778888";


const char* host = "192.168.0.149";
const uint16_t port = 9000;
WiFiClient client;

#define TASK1_DELAY 500 
#define TASK2_DELAY 1000 


TaskHandle_t task1_handle = NULL;
TaskHandle_t task2_handle = NULL;


void task1(void *pvParameters) {
    while (1) {
        while(1){
          Serial.println("Task 1 is running");
          vTaskDelay(TASK1_DELAY / portTICK_PERIOD_MS); 
          //lcd.print("LOCK ....");
        }
        
        
    }
}

void task2(void *pvParameters) {
    while (1) {
        while(1){
          Serial.println("Task 2 is running");
          control();
        }
        
        vTaskDelay(TASK2_DELAY / portTICK_PERIOD_MS); 
    }
}



void loop() {
    if (client.available()) {
        char command = client.read();

        if (command == '2') {
            
            vTaskSuspend(task2_handle);
            vTaskResume(task1_handle);
        } else if (command == '1') {
            vTaskSuspend(task1_handle);
            vTaskResume(task2_handle);
        }else if(command == '3'){
        Serial.print("Phát hiện khuôn mặt. Mở cửa ....... ");
        unlockDoor();
        }
    }

    delay(100);
}
void setup()
{
  EEPROM.begin(512);
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();

  LastStateChangeTime = millis();
  updateState(STATE_STARTING);

  readMasterCardFromEEPROM(masterCard);

  for (int i = 0; i < cardArrSize; i++) {
    byte storedCard[cardSize];
    readCardFromEEPROM(i, storedCard);
    if (storedCard[0] != 0xFF) { 
      cardsStored++;
    }
  }


  s1.attach(12);
  s1.write(0);
  finger.begin(57600);
  lcd.init();
  lcd.backlight();
  lcd.print("HE THONG KHOA!");
  delay(2000);
  lcd.setCursor(0,1);
  lcd.print("Khoi dong...");
  initialpassword();
  delay(2000);
  displayUpdated = false;

  Serial.println();
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  configTime(3600*7, 0, "pool.ntp.org");
  client.connect(host, port);

   xTaskCreate(task2, "Task2", 2048, NULL, 1, &task2_handle);
   xTaskCreate(task1, "Task1", 2048, NULL, 1, &task1_handle);
   vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void control() {

  Serial.print("Connecting to ");
  Serial.print(host);
  Serial.print(':');
  Serial.println(port);
  checkThe();
  getFingerprintIDez(); 
  keypressed = myKeypad.getKey(); 

  if (keypressed == 'A') { 
    ij = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("THEM NGUOI MOI");
    lcd.setCursor(0, 1);
    //lcd.print("Doc van tay...");
    delay(4000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Nhap mat khau");

    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

    String data =String(timeStr);
    data = data + " Yêu cầu thêm vân tay.";
  
    getPassword(); 
    if (ij == 4) {
      Enrolling(); 
      delay(2000);
      lcd.clear();
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Mat khau sai");
      data = data +" Trạng thái: Không thành công";
      client.print(data);
      delay(2000);
      lcd.clear();
    }
    displayUpdated = false;
  }

  if (keypressed == '*') {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MO KHOA BANG MK");
    lcd.blink();
    lcd.setCursor(0, 1);
    //lcd.print("Mo cua ngay!");
    lcd.blink();
    delay(4000);
    ij = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Nhap mat khau");

    time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

  String data =String(timeStr);
  data = data + " Mở khóa bằng mật khẩu.";
  
    getPassword();
    if (ij == 4) { 
      s1.write(180);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MO KHOA ");
      lcd.blink();
      lcd.setCursor(0, 1);
      lcd.print("OPEN .....");
      lcd.blink();
      delay(5000);
      s1.write(0);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MO KHOA");
      lcd.blink();
      lcd.setCursor(0, 1);
      lcd.print("CLOSE .....");
      
      data = data +" Trạng thái: Thành công";
      client.print(data);
      delay(2000);
    } else { 
      lcd.setCursor(0, 0);
      lcd.print("Mat khau sai");
      
      data = data +" Trạng thái: Không thành công";
      client.print(data);
      delay(2000);
      lcd.clear();
    }
    displayUpdated = false;
  }

  if (keypressed == '#') { 
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Doi MAT KHAU");
    delay(2000);
    lcd.blink();
    change();
    displayUpdated = false;
  }
   if (keypressed == 'B') { 

    String c = getTimeAndMessage();
    c += " Yêu cầu xóa vân tay.";
    ij = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("XOA VAN TAY");
    lcd.setCursor(0, 1);
    lcd.print("Nhap mat khau: ");
    getPassword(); 
    if (ij == 4) { 
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Dat ngon tay...");
      deleteFingerprint(); 
      delay(2000);
    } else {
      c+= " Trạng thái: Không thành công.";
      client.print(c);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mat khau sai");
      delay(2000);
    }
    displayUpdated = false;
  }

  if (keypressed == 'C') {
    ij = 0;
    byte cardState;
    String c = getTimeAndMessage();
    c+= " Yêu cầu thêm thẻ.";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Them the rfid ");
    lcd.setCursor(0, 1);
    lcd.print("Nhap mat khau: ");
    getPassword(); 
    if (ij == 4) {
      lcd.setCursor(0, 1);
      lcd.print("quet the ...");
      delay(2000);
      Serial.println("Press 'C' and scan card to add.");

       while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        }
      cardState = readCardState();
      if (cardState == STATE_SCAN_INVALID) {
        addReadCard();
        c+= " Trạng thái: Thành công.";
        client.print(c);
      } else {
        c+= " Trạng thái: Không thành công.";
        client.print(c);
        lcd.setCursor(0, 1);
        lcd.print("Card already added!");
        delay(3000);
      }

    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mat khau sai");
      c+= " Trạng thái: Không thành công.";
      client.print(c);
        
      delay(2000);
    }
    displayUpdated = false;
  }

  if (keypressed == 'D') { 
    ij = 0;
    byte cardState;

    String c = getTimeAndMessage();
    c+= " Yêu cầu xóa thẻ.";
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Xoa the rfid ");
    lcd.setCursor(0, 1);
    lcd.print("Nhap mat khau: ");
    getPassword(); 
    if (ij == 4) { 
        Serial.println("Press 'D' and scan card to remove.");
        lcd.print("quet the ...");
        delay(2000);
        while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        }
      cardState = readCardState();
      if (cardState == STATE_SCAN_VALID) {
        removeReadCard();
        c+= " Trạng thái: Thành công.";
        client.print(c);
      } else {
        lcd.setCursor(0, 1);
        lcd.print("Card not found!");
        c+= " Trạng thái: Không thành công.";
        client.print(c);
        delay(1000);
      }


    } else { 
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mat khau sai");
      c+= " Trạng thái: Không thành công.";
      client.print(c);
      delay(2000);
    }
    displayUpdated = false;
  }

  
  
  if (!displayUpdated) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("He thong khoa ");
    lcd.setCursor(0, 1);
    //lcd.print("Doc van tay...");
    displayUpdated = true;
  }

  delay(200);
}
void getPassword(){
  for (int i=0 ; i<4 ; i++){
    c[i]= myKeypad.waitForKey();
    lcd.setCursor(i,1);
    lcd.print("*");
  }
  lcd.clear();
  for(int j=0;j<4;j++){
    code[j] == EEPROM.read(j);
  }
  for (int j=0 ; j<4 ; j++){ 
    if(c[j]==code[j])
      ij++; 
  } 
}

void Enrolling() {
  keypressed = NULL;
  lcd.clear();
  lcd.print("THEM NGUOI MOI");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Nhap ID moi");
  id = readnumber(); 
  String c = getTimeAndMessage();
  c += "Yêu cầu thêm vân tay."; 
  if (id == 0) {
    c+= "Trạng thái: Không thành công.";
    client.print(c);
    return;
  }
  if (finger.loadModel(id) == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ID da ton tai");
    lcd.setCursor(0, 1);
    lcd.print("Nhap ID moi");
    c+= "Trạng thái: Không thành công.";
    client.print(c);
    delay(3000);
    return;
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dat ngon tay");
  uint8_t p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {

    c+= "Trạng thái: Không thành công.";
    client.print(c);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Loi chup van");
    lcd.setCursor(0, 1);
    lcd.print("tay");
    delay(2000);
    return;
  }

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    c+= " Trạng thái: Không thành công.";
    client.print(c);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Van tay da ton");
    lcd.setCursor(0, 1);
    lcd.print("tai");
    delay(3000);
    return;
  }

  while (!getFingerprintEnroll());

  if(k == 0){
    c+= "Trạng thái: Thành công.";
    client.print(c);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Luu ID: ");
    lcd.setCursor(8, 0);
    lcd.print(id);
    delay(3000);
  }
  k = 0;
  
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  lcd.clear();
  lcd.print("Enroll ID:"); 
  lcd.setCursor(10,0);
  lcd.print(id);
  lcd.setCursor(0,1);
  lcd.print("Place finger"); 
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      break;
    case FINGERPRINT_IMAGEMESS:
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      return p;
    case FINGERPRINT_FEATUREFAIL:
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      return p;
    default:
      return p;
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Remove finger"); 
  lcd.setCursor(0,1);
  lcd.print("please !");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  p = -1;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Place same"); 
  lcd.setCursor(0,1);
  lcd.print("finger please");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
  }
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      break;
    case FINGERPRINT_IMAGEMESS:
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      return p;
    case FINGERPRINT_FEATUREFAIL:
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      return p;
    default:
      return p;
  }
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Khong thanh cong !");
    delay(3000);
    k = 1;
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Khong thanh cong !");
    delay(3000);
    k = 1;
    return p;
  } else {
    return p;
  }
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear(); 
    lcd.setCursor(0,0);
    lcd.print("Stored in"); 
    lcd.setCursor(0,1);
    lcd.print("ID: ");
    lcd.setCursor(5,1);
    lcd.print(id);
    delay(3000);
    lcd.clear();
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
      return p;
  } else if (p == FINGERPRINT_FLASHERR) {
      return p;
  } else {
      return p;
  }
}

uint8_t readnumber(void) {
  uint8_t num = 0;
  while (num == 0) {
    char keey = myKeypad.waitForKey();
    int num1 = keey-48;
    lcd.setCursor(0,1);
    lcd.print(num1);
    keey = myKeypad.waitForKey();
    int num2 = keey-48;
    lcd.setCursor(1,1);
    lcd.print(num2);
    keey = myKeypad.waitForKey();
    int num3 = keey-48;
    lcd.setCursor(2,1);
    lcd.print(num3);
    delay(1000);
    num=(num1*100)+(num2*10)+num3;
    keey=NO_KEY;
  }
  return num;
}

int getFingerprintIDez() {
  uint8_t p = finger.getImage(); 
  if (p != FINGERPRINT_OK) return -1;
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;
  p = finger.fingerFastSearch(); 

  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

  String data =String(timeStr);
  data = data + " Mở khóa bằng vân tay.";
  
  if (p != FINGERPRINT_OK){ 
    lcd.clear(); 
    lcd.print("Khong cho phep");
    delay(2000);
    lcd.clear();
    lcd.print("He thong khoa ");
    data = data +" Trạng thái: Không thành công";
    client.print(data);
      
    return -1;
  }
 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CHAO MUNG");
  lcd.setCursor(0, 1);
  lcd.print("ID: #");
  lcd.setCursor(8, 1);
  lcd.print(finger.fingerID, DEC);
  delay(5000);
  s1.write(180);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MO KHOA");
  lcd.blink();
  lcd.setCursor(0, 1);
  lcd.print("OPEN .....");
  lcd.blink();
  delay(5000); 
  s1.write(0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MO KHOA");
  lcd.blink();
  lcd.setCursor(0, 1);
  lcd.print("CLOSE ...");
  delay(2000);
  lcd.clear();
  lcd.print("He thong khoa ");
  data = data +" Trạng thái: Thành công";
  client.print(data);
  return finger.fingerID;
}

void change(){
  int j=0;
  lcd.clear();
  lcd.print("Mat khau cu");
  lcd.setCursor(0,1);
  while(j<4){
    char key=myKeypad.getKey();
    if(key)
    {
      c[j++]=key;
      lcd.print(key);
    }
    key=0;}
  delay(500);

  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

  String data =String(timeStr);
  data = data + " Thay đổi mật khẩu.";
  //client.print(data);
  
  if((strncmp(c, code, 4))){
    lcd.clear();
    lcd.print("Mat khau sai");
    lcd.setCursor(0,1);
    lcd.print("Thu lai");
    delay(1000);

    data = data +" Trạng thái: Không thành công";
     client.print(data);
      
  } else {
    j=0;
    lcd.clear();
    lcd.print("Mat khau moi:");
    lcd.setCursor(0,1);

    while(j<4){
      char key=myKeypad.getKey();
      if(key)
      {
        code[j]=key;
        lcd.print(key);
        EEPROM.write(j,key);
        j++;
      }
    }
    lcd.print("Mat khau moi");

    data = data +" Trạng thái: Thành công";
     client.print(data);
      
    delay(1000);
  }
 
  lcd.clear();
  lcd.print("Nhap mat khau");
  lcd.setCursor(0,1);
  keypressed=0;
}
void deleteFingerprint() {
  uint8_t p = FINGERPRINT_NOFINGER;

  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dat ngon tay");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    delay(200); 
  }
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Loi chup van tay");
    delay(2000);
    return;
  }
  String c = getTimeAndMessage();
  c+= " Yêu cầu xóa vân tay.";
  
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Khong co van tay");

    c+= " Trạng thái: Không thành công.";
    client.print(c);
    delay(2000);
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Xoa ID: ");
  lcd.setCursor(8, 0);
  lcd.print(finger.fingerID);
  delay(2000);

  p = finger.deleteModel(finger.fingerID);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Da xoa ID : ");
    lcd.setCursor(10, 0);
    lcd.print(finger.fingerID);
    c += " Trạng thái: Thành công.";
    client.print(c);
    
    delay(3000);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Loi xoa vân tay");
    delay(2000);
  } else if (p == FINGERPRINT_BADLOCATION) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Vi tri loi");
    delay(2000);
  } else if (p == FINGERPRINT_FLASHERR) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Loi flash");
    delay(2000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Loi khong xac dinh");
    delay(2000);
  }
}

void initialpassword(){
  int j;
  for(j=0;j<4;j++)
    EEPROM.write(j,j+49);
  for(j=0;j<4;j++)
    code[j]=EEPROM.read(j);
}

void saveCardToEEPROM(int index, byte* card) {
  int startAddr = eepromCardsStart + index * cardSize;
  for (int i = 0; i < cardSize; i++) {
    EEPROM.write(startAddr + i, card[i]);
  }
  EEPROM.commit();
}

void saveMasterCardToEEPROM(byte* card) {
  for (int i = 0; i < cardSize; i++) {
    EEPROM.write(eepromMasterCardStart + i, card[i]);
  }
  EEPROM.commit();
}

void readCardFromEEPROM(int index, byte* card) {
  int startAddr = eepromCardsStart + index * cardSize;
  for (int i = 0; i < cardSize; i++) {
    card[i] = EEPROM.read(startAddr + i);
  }
}

void readMasterCardFromEEPROM(byte* card) {
  for (int i = 0; i < cardSize; i++) {
    card[i] = EEPROM.read(eepromMasterCardStart + i);
  }
}

int readCardState() {
  int index;

  Serial.print("Card Data - ");
  for (index = 0; index < cardSize; index++) {
    readCard[index] = mfrc522.uid.uidByte[index];
    Serial.print(readCard[index]);
    if (index < cardSize - 1) {
      Serial.print(",");
    }
  }
  Serial.println(" ");

  if ((memcmp(readCard, masterCard, cardSize)) == 0) {
    return STATE_SCAN_MASTER;
  }

  if (cardsStored == 0) {
    return STATE_SCAN_INVALID;
  }

  byte storedCard[cardSize];
  for (index = 0; index < cardsStored; index++) {
    readCardFromEEPROM(index, storedCard);
    if ((memcmp(readCard, storedCard, cardSize)) == 0) {
      return STATE_SCAN_VALID;
    }
  }

  return STATE_SCAN_INVALID;
}

void addReadCard() {
  if (cardsStored < cardArrSize) {
    saveCardToEEPROM(cardsStored, readCard);
    cardsStored++;
    lcd.setCursor(0, 1);
    lcd.print("Card added!");
    delay(3000);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("No space to add new card!");
    delay(3000);
  }
}

void removeReadCard() {
  int cardIndex;
  byte storedCard[cardSize];
  bool found = false;

  for (cardIndex = 0; cardIndex < cardsStored; cardIndex++) {
    readCardFromEEPROM(cardIndex, storedCard);
    if ((memcmp(readCard, storedCard, cardSize)) == 0) {
      found = true;
    }
    if (found && cardIndex < cardsStored - 1) {
      readCardFromEEPROM(cardIndex + 1, storedCard);
      saveCardToEEPROM(cardIndex, storedCard);
    }
  }

  if (found) {
    cardsStored--;
    lcd.setCursor(0, 1);
    lcd.print("Card removed!");
    delay(3000);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Card not found!");
    delay(3000);
  }
}

void unlockDoor() {
  lcd.setCursor(0, 1);
  lcd.print("Unlocking door!");
  s1.write(180);
  delay(5000);
  s1.write(0);
}

void updateState(byte aState) {
  if (aState == currentState) {
    return;
  }

  switch (aState) {
    case STATE_STARTING:
      StateWaitTime = 1000;
      break;
    case STATE_WAITING:
      StateWaitTime = 0;
      break;
    case STATE_SCAN_INVALID:
      if (currentState == STATE_SCAN_MASTER) {
        addReadCard();
        aState = STATE_ADDED_CARD;
        StateWaitTime = 1000;
      } else if (currentState == STATE_REMOVED_CARD) {
        return;
      } else {
        StateWaitTime = 1000;
      }
      break;
    case STATE_SCAN_VALID:
      if (currentState == STATE_SCAN_MASTER) {
        removeReadCard();
        aState = STATE_REMOVED_CARD;
        StateWaitTime = 1000;
      } else if (currentState == STATE_ADDED_CARD) {
        return;
      } else {
        StateWaitTime = 1000;
      }
      break;
    case STATE_SCAN_MASTER:
      StateWaitTime = 3000;
      break;
    case STATE_UNLOCKED:
      unlockDoor();
      StateWaitTime = 1000;
      break;
  }

  currentState = aState;
  LastStateChangeTime = millis();
}
void checkThe(){
  byte cardState;

   if ((currentState != STATE_WAITING) &&
      (StateWaitTime > 0) &&
      (LastStateChangeTime + StateWaitTime < millis())) {
    updateState(STATE_WAITING);
  }

  
  
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.println("Mo khoa bang the ..");

  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
  String data =String(timeStr);
  data = data + " Mở khóa bằng thẻ.";
  //client.print(data);
  
    cardState = readCardState();
    if (cardState == STATE_SCAN_VALID) {
      updateState(STATE_UNLOCKED);
      data = data +" Trạng thái: Thành công";
      client.print(data);
    } else if (cardState == STATE_SCAN_INVALID) {
      data = data +" Trạng thái: Không thành công";
      client.print(data);
      Serial.println("Khong cho phep!");
      lcd.print("Khong cho phep!");
      delay(2000);
    }
  }
  delay(50);
  displayUpdated = false;

}

String getTimeAndMessage() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
    String data = String(timeStr);
    data += " ";
    return data;
}
