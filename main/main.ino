#include <SPI.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include<EEPROM.h>
#include<MFRC522.h>

// Define pins for RFID RC522 abc
#define SS_PIN 9
#define RST_PIN 8

// Create instances
LiquidCrystal_I2C lcd(0x27, 16, 2); // Address might be different for your LCD
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo s;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {22, 23, 24, 25};
byte colPins[COLS] = {28, 29, 30, 31};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int i = 0;
char key;
char tmp_pass[6];
char master_password[6];
byte master_uid[4];
char user_password[6];
byte user_uid[4];
//tu 0->7
//2 byte dau luu tru first_1
//2 byte tiep luu tru first_2
//2 byte tiep luu tru first_3
int first_2;
int first_3;

//part 1: thong tin ve master_password, master_uid
//part 2: luu tru mat khau nguoi dung
const int start_2 = 21;
const int end_2 = 150;
// int first_2 = start_2;
//part 3: luu tru uid cua nguoi dung
const int start_3 = 160;
const int end_3 = 300;
// int first_3 = start_3; 

const int size_pass = sizeof(user_password);
const int size_uid = sizeof(user_uid);

int get_first_2(){
  if(EEPROM.read(2) == 0xFF){
    return start_2;
  }

  byte highByte = EEPROM.read(2);  // Đọc byte cao tại địa chỉ 0
  byte lowByte = EEPROM.read(3);   // Đọc byte thấp tại địa chỉ 1
  int new_value = (highByte << 8) | lowByte; 
  return new_value;
}
int get_first_3(){
  if(EEPROM.read(2) == 0xFF){
    return start_3;
  }

  byte highByte = EEPROM.read(4);  // Đọc byte cao tại địa chỉ 0
  byte lowByte = EEPROM.read(5);   // Đọc byte thấp tại địa chỉ 1
  int new_value = (highByte << 8) | lowByte; 
  return new_value;
}
void set_first_2(int value) {
  //cap nhap gia tri vao eeprom

  //tim first byte co gia tri 0xFF
  int j;
  for(j = value; j <= end_2; j+= size_pass - 1){
    if(EEPROM.read(j) == 0xFF){
      first_2 = j;
      break;
    }
  }
  // Chia giá trị thành 2 byte
  byte highByte = (first_2 >> 8) & 0xFF;  // Lấy byte cao
  byte lowByte = first_2 & 0xFF;          // Lấy byte thấp

  // Ghi các byte vào EEPROM
  EEPROM.write(2, highByte);
  EEPROM.write(3, lowByte);
}
void set_first_3(int value) {
  //cap nhap gia tri vao eeprom
  //tim first byte co gia tri 0xFF
  int j;
  for(j = value; j <= end_3; j+= size_uid){
    if(EEPROM.read(j) == 0xFF){
      first_3 = j;
      break;
    }
  }
  
  // Chia giá trị thành 2 byte
  byte highByte = (first_3 >> 8) & 0xFF;  // Lấy byte cao
  byte lowByte = first_3 & 0xFF;          // Lấy byte thấp

  // Ghi các byte vào EEPROM
  EEPROM.write(4, highByte);
  EEPROM.write(5, lowByte);
}

bool validate_master_pass(char* pass_need_validate){
  if(strncmp(pass_need_validate, master_password, 5) == 0){
    Serial.println("validate_master_pass: right");
    return true;
  }
  Serial.println("validate_master_pass: wrong");
  return false;
}
void add_user_pass(char* new_user_pass){
  if(first_2 + size_pass - 1 >= end_2){
    //lcd print: not enough space
    Serial.println("not enough space");
    lcd.clear();
    lcd.print("not enough space");
    delay(1000);
    return;
  }
  char used_pass[6];

  //kiem tra xem new_user_pass da ton tai trong he thong hay chua 
  int j = start_2;
  bool is_exist_used_pass = false;
  while(j + size_pass - 1 < end_2){
    if(EEPROM.read(j) == 0xFF){
      j += size_pass - 1;
      continue;
    }
    for(int k = 0; k < 5; k++){
      used_pass[k] = EEPROM.read(j + k);
    }
    used_pass[5] = '\0';
    if(strncmp(used_pass, new_user_pass, 5) == 0){
      is_exist_used_pass = true;
      break;
    }
    j += size_pass - 1;
  }
  if(is_exist_used_pass || validate_master_pass(new_user_pass)){
    //man hinh hien thi pass_da ton tai
    Serial.println("Pass exist");
    lcd.clear();
    lcd.print("pass exist");
    delay(1000);
    return;
  }
  //neu chua ton tai, them vao index eeprom cua first_2
  for(int k = 0; k < 5; k++){
    EEPROM.write(first_2 + k, new_user_pass[k]);
  }
  //cap nhat first_2
  set_first_2(first_2 + size_pass - 1);
  Serial.println("pass added");
  lcd.clear();
  lcd.print("pass added");
  delay(1000);
}
bool validate_master_uid(byte* uid_need_validate) {
  int count_equal;

      count_equal = 0;
      for (int k = 0; k < 4; k++) {
        if (master_uid[k] == uid_need_validate[k]) {
          count_equal++;
        }
      }
      if (count_equal == 4) {
        Serial.println("validate_user_uid: right");
        return true;
      }

  Serial.println("validate_user_uid: wrong");
  return false;
}
void add_user_uid(byte* new_user_uid) {
  if (first_3 + size_uid >= end_3) {
    // lcd print: not enough space
    Serial.println("Not enough space");
    lcd.clear();
    lcd.print("not enough space");
    delay(1000);
    return;
  }
  
  //kiem tra uid da ton tai hay chua
  int j = start_3;
  bool is_exist_used_uid = false;
  int count_equal;
  while (j + size_uid <= end_3) {
    if (EEPROM.read(j) == 0xFF) {
      j += size_uid;
      continue;
    }

    byte used_uid[4];
    for (int k = 0; k < 4; k++) {
      used_uid[k] = EEPROM.read(j + k);
    }

    count_equal = 0;
    for (int k = 0; k < 4; k++) {
      if (used_uid[k] == new_user_uid[k]) {
        count_equal++;
      }
    }

    if (count_equal == 4) {
      is_exist_used_uid = true;
      break;
    }

    j += size_uid;
  }

  if (is_exist_used_uid || validate_master_uid(new_user_uid)) {
    // màn hình hiển thị UID đã tồn tại
    Serial.println("UID already exists");
    lcd.clear();
    lcd.print("UID exist");
    delay(1000);
    return;
  }

  // nếu chưa tồn tại, thêm vào index EEPROM của first_3
  for (int k = 0; k < 4; k++) {
    EEPROM.write(first_3 + k, new_user_uid[k]);
  }

  // cập nhật first_3
  set_first_3(first_3 + size_uid);
  lcd.clear();
  lcd.print("uid added");
  delay(1000);
}


void del_user_pass(char* pass_to_delete) {

  //kiem tra su ton tai
  int j = start_2;
  bool pass_found = false;
  while (j + size_pass - 1 < end_2) {
    char stored_pass[6];
    for (int k = 0; k < 5; k++) {
      stored_pass[k] = EEPROM.read(j + k);
    }
    stored_pass[5] = '\0';

    if (strncmp(stored_pass, pass_to_delete, 5) == 0) {
      pass_found = true;
      break;
    }
    j += size_pass - 1;
  }
  //chua ton tai -> thong bao la pass khong ton tai
  if(pass_found == false){
    //lcd print pass not exist
    Serial.println("pass not exist");
    lcd.clear();
    lcd.print("pass not exist");
    delay(1000);
  } else {
    //dat het phan byte nay la 0xFF (do da xoa)
    for(int k = 0; k < 5; k++){
      EEPROM.write(k+j, 0xFF);
    }
    //neu nhu phan bi xoa dung truoc first_2 thi cap nhat first_2 = j
    //neu phan bi xoa dung sau thi khong can 
    if(j < first_2){
      set_first_2(j);
    }
    Serial.println("pass deleted");
    lcd.clear();
    lcd.print("pass deleted");
    delay(1000);
  }
}

void del_user_uid(byte* uid_to_delete) {
  int j = start_3;
  bool uid_found = false;
  int count_equal;
  while (j + size_uid < end_3) {
    // if (EEPROM.read(j) == 0xFF) {
    //   break;
    // }
    byte stored_uid[4];
    for (int k = 0; k < 4; k++) {
      stored_uid[k] = EEPROM.read(j + k);
    }
    //so sanh hai uid
    count_equal = 0;
    for(int k = 0; k < 4; k++){
      if(stored_uid[k] == uid_to_delete[k]){
        count_equal++;
      }
    }
    if(count_equal == 4){
      uid_found = true;
      break;
    }
    j += size_uid;
  }
  if (uid_found == false) {
    // LCD print: UID not found
    Serial.print("uid not exist");
    lcd.clear();
    lcd.print("uid not exist");
    delay(1000);
  } else {
    Serial.println("uid deleted");
    lcd.clear();
    lcd.print("uid deleted");
    delay(1000);
    //dat het phan byte nay la 0xFF (do da xoa)
    for(int k = 0; k < 4; k++){
      EEPROM.write(k+j, 0xFF);
    }
    //neu nhu phan bi xoa dung truoc first_3 thi cap nhat first_3 = j
    //neu phan bi xoa dung sau thi khong can 
    if(j < first_3){
      set_first_3(j);
    }
  }
}

//bit 6: chcek xem eeprom da duoc reset 0xff chua
//bit7: check xem master_password da duoc khoi tao chua 
//bit8: check xem master_uid da duoc khoi tao chua 
//from 9 -> 13
void set_master_password(){
  //neu chua khoi tao thi khoi tao mac dinh la 1 2 3 4 5
  if(EEPROM.read(7) != 49){
    for(int k = 0; k < 5; k++){
      master_password[k] = k+49;
      EEPROM.write(k+9, k+49);
    }
    EEPROM.write(7, 49);
    return;
  }
  for(int k = 0; k < 5; k++){
    master_password[k] = EEPROM.read(k+9);
    Serial.println(master_password[k]);
  }
}
//from 15->18
void set_master_uid(){
  if(EEPROM.read(8) != 49){
    //set up ban dau cho master card
    Serial.println("Your must input your master card");
    while(!(mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())){
      read_card_to_serial(mfrc522.uid.uidByte);
    }
    for (byte k = 0; k < 4; k++) {
      master_uid[k] = mfrc522.uid.uidByte[k];
      EEPROM.write(15 + k, master_uid[k]);
    }
    Serial.println("Master UID has been set.");
    EEPROM.write(8, 49);
  } else {
    // Load master UID from EEPROM
    for (byte k = 0; k < 4; k++) {
      master_uid[k] = EEPROM.read(15 + k);
    }
    Serial.println("Master UID is already set.");
  }
}
void set_eeprom(){
  if(EEPROM.read(6) != 49){
    for(int k = 0; k < EEPROM.length(); k++){
      EEPROM.write(k, 0xFF);
    }
    Serial.println("set_eeprom: all element set to 0xFF");
    //danh dau o index 6 trong eeprom la 1 -> danh dau la da dua toan bo phan tu trong eeprom ve gia tri 0xFF
    EEPROM.write(6, 49);
  } else {
    Serial.println("set_eeprom: eeprom has been set");
  }
}
bool validate_user_pass(char* pass_need_validate){
  char pass_to_check[5];
  for(int j = start_2; j < end_2; j += size_pass - 1){
    if(EEPROM.read(j) != 0xFF){
      for(int k = 0; k < 5; k++){
        pass_to_check[k] = EEPROM.read(k+j);
      }
      if(strncmp(pass_to_check, pass_need_validate, 5)==0){
        Serial.println("validate_user_pass: right");
        return true;
      }
    } 
  }
  Serial.println("validate_user_pass: wrong");
  return false;
}
bool validate_user_uid(byte* uid_need_validate) {
  byte uid_to_check[4];
  int count_equal;

  for (int j = start_3; j < end_3; j += size_uid) {
    if (EEPROM.read(j) != 0xFF) {
      for (int k = 0; k < 4; k++) {
        uid_to_check[k] = EEPROM.read(j + k);
      }

      count_equal = 0;
      for (int k = 0; k < 4; k++) {
        if (uid_to_check[k] == uid_need_validate[k]) {
          count_equal++;
        }
      }
      if (count_equal == 4) {
        Serial.println("validate_user_uid: right");
        return true;
      }
    }
  }
  Serial.println("validate_user_uid: wrong");
  return false;
}


void setup() {
  //truoc do co the cap nhap cac start va end 1 2 3
  // first_1 = get_first_1();
  set_eeprom();
  set_master_password();
  first_2 = get_first_2();
  first_3 = get_first_3();

  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.init();                    
  lcd.backlight();
  lcd.clear();
  lcd.print("Enter Password:");
  lcd.setCursor(0,1);
  s.attach(10);
  s.write(90);
  set_master_uid();
  Serial.println("Done 1");
}
void loop() {
  // kiem tra the 
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    handle_card();
    mfrc522.PICC_HaltA(); // Stop reading
  } else {
    handle_pass();
    delay(50);  // Add a short delay to avoid too rapid polling
  }
}
void handle_pass(){

  char key = keypad.getKey();
  
  if (key) {
    if (key == 'A') {
      //khoi tao lai
      re_input();
      return;
    } else if (key == 'B') {
      //xoa ky tu
      del_char();
    } else if (key == 'C') {
      //thay doi master_pass va master_uid
      change_master();
      return;
    } else if (key == 'D') {
      //them xoa mat khau cho user
      handle_user();
      //
    } else if (key == '#') {
      // Handle '#' key (if required)
    } else if (key == '*') {
      // Handle '*' key (if required)
    } else {
      tmp_pass[i++] = key;
      lcd.print(key);
      lcd.setCursor(i - 1, 1);
      delay(300);
      lcd.print("*");
    }
  }

  if (i == 5) {
    tmp_pass[i] = '\0';
    delay(300);
    if (validate_user_pass(tmp_pass)) {
      do_when_granted();
    } else {
      do_when_denied();
    }
    re_input();
  }
}
void handle_user(){
  //truoc tien la phai co phan nhap mat khau cua master, hoac la the rfid cua master (kiem tra tinh dung sai )
  byte readUID[4];
  bool valid_master = false;
  lcd.clear();
  lcd.print("Master Pass:");
  lcd.setCursor(0, 1);
  // char master_input[6];
  int input_pos = 0;

  while(0 == 0){
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      for (int k = 0; k < 4; k++) {
        readUID[k] = mfrc522.uid.uidByte[k];
      }
      if(validate_master_uid(readUID)){
        valid_master = true;
        break;
      } else {
        Serial.println("Wrong UID");
        lcd.clear();
        lcd.print("Wrong UID");
        delay(1000);
        lcd.clear();
        lcd.print("Master Pass:");
        lcd.setCursor(0, 1);
      }
      mfrc522.PICC_HaltA(); // Stop reading
    } else {
      key = keypad.getKey();
      if (key) {
        if (key == 'A') {
          //khoi tao lai
          re_input();
          return;
        } else if (key == 'B') {
          //xoa ky tu
          // del_char();
          if(input_pos > 0){
            lcd.setCursor(input_pos-1, 1);
            lcd.print(" ");
            input_pos--;
          }
          if(input_pos == 0){
            lcd.setCursor(0, 1);
          }
        } else if (key == 'C') {
          //thay doi master_pass va master_uid
          // change_master();
          // return;
        } else if (key == 'D') {
          //them xoa mat khau cho user
          // handle_user();
          //
        } else if (key == '#') {
          // Handle '#' key (if required)
        } else if (key == '*') {
          // Handle '*' key (if required)
        } else {
          tmp_pass[input_pos++] = key;
          lcd.print(key);
          lcd.setCursor(input_pos - 1, 1);
          delay(300);
          lcd.print("*");
        }
      }

      if (input_pos == 5) {
        tmp_pass[input_pos] = '\0';
        delay(300);
        if (validate_master_pass(tmp_pass)) {
          valid_master = true;
          Serial.println("valid_naster: true");
        } else {
          Serial.println("valid_master: false");
        }
        break;
      }
      delay(50); 
    }
  }
  if(valid_master){
    lcd.clear();
    lcd.print("A.Add ID");
    lcd.print("B.Del ID");
    lcd.setCursor(0, 1);
    lcd.print("C.Add pw");
    lcd.print("D.Del pw");
    delay(1000);
    // lcd.clear();
    // lcd.print("#: exit");
    // delay(500);
    while (true) {
      char key = keypad.getKey();
      if (key == '#') {
        // Exit the user handling menu
        Serial.println("Exit");
        re_input();
        return;
      }
      
      if (key == 'A') {
        // Add UID
        lcd.clear();
        lcd.print("Scan new UID");
        while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
          //wait to scan
          read_card_to_serial(mfrc522.uid.uidByte);
        }
        add_user_uid(mfrc522.uid.uidByte);
        mfrc522.PICC_HaltA(); // Stop reading
        re_input();
        return;
      } else if (key == 'B') {
        // Delete UID
        lcd.clear();
        lcd.print("Scan UID to Del");
        while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
          // Wait for card to be scanned
          read_card_to_serial(mfrc522.uid.uidByte);
        }
        del_user_uid(mfrc522.uid.uidByte);
        mfrc522.PICC_HaltA(); // Stop reading
        re_input();
        return;
      } else if (key == 'C') {
        // Add Password
        lcd.clear();
        lcd.print("New User Pass:");
        lcd.setCursor(0, 1);
        
        char new_pass[6];
        int pass_pos = 0;
        while (pass_pos < 5) {
          key = keypad.getKey();
          if (key) {
            if(key == 'A'){
              re_input();
              return;
            } else if(key == 'B'){
              if(pass_pos > 0){
                lcd.setCursor(pass_pos-1, 1);
                lcd.print(" ");
                pass_pos--;
              }
              if(pass_pos == 0){
                lcd.setCursor(0, 1);
              }
            } else if(key == 'C' || key == 'D' || key == '*' || key == '#'){

            } else {
              new_pass[pass_pos] = key;
              lcd.print(key);
              lcd.setCursor(pass_pos, 1);
              delay(300);
              lcd.print("*");
              pass_pos++;
            }
          }
        }
        new_pass[5] = '\0';
        
        add_user_pass(new_pass);
        re_input();
        return;
      } else if (key == 'D') {
        // Delete Password
        lcd.clear();
        lcd.print("User Pass to Del:");
        lcd.setCursor(0, 1);
        
        char del_pass[6];
        int pass_pos = 0;
        while (pass_pos < 5) {
          key = keypad.getKey();
          if (key) {
            if(key == 'A'){
              re_input();
              return;
            } else if(key == 'B'){
              if(pass_pos > 0){
                lcd.setCursor(pass_pos-1, 1);
                lcd.print(" ");
                pass_pos--;
              }
              if(pass_pos == 0){
                lcd.setCursor(0, 1);
              }
            } else if(key == 'C' || key == 'D' || key == '*' || key == '#'){

            } else {
              del_pass[pass_pos] = key;
              lcd.print(key);
              lcd.setCursor(pass_pos, 1);
              delay(300);
              lcd.print("*");
              pass_pos++;
            }
          }
        }
        del_pass[5] = '\0';
        
        del_user_pass(del_pass);
        re_input();
        return;
      }
    }
  } else {
    read_card_to_serial(readUID);
    lcd.clear();
    lcd.print("Wrong Pass");
    delay(1000);
    re_input();
    return;
  }
  //sau do 
  //hien ra menu: 
  //A: add_uid, B: del_uid, C: add_pass, D: del_pass, #: exit
  //cho phep, them, xoa mat khau, uid (mat khau va uid nhap truoc)
  
}
void read_card_to_serial(byte* readUID){

  Serial.print("Card UID: ");
  for (byte i = 0; i < 4; i++) {
    Serial.print(readUID[i] < 0x10 ? " 0" : " ");
    Serial.print(readUID[i], HEX);
  }
  Serial.println();
}
void handle_card(){
  //luu card nay vao trong user_uid
  // Lấy UID từ thẻ được quét
  byte readUID[4];
  for (byte i = 0; i < 4; i++) {
    readUID[i] = mfrc522.uid.uidByte[i];
  }

  // Hiển thị UID của thẻ trên Serial Monitor
  // Serial.print("Card UID: ");
  // for (byte i = 0; i < 4; i++) {
  //   Serial.print(readUID[i] < 0x10 ? " 0" : " ");
  //   Serial.print(readUID[i], HEX);
  // }
  // Serial.println();
  read_card_to_serial(readUID);
  // Kiểm tra xem UID này có trong hệ thống hay không
  if (validate_user_uid(readUID)) {
    do_when_granted();
  } else {
    do_when_denied();
  }
}
void do_when_granted(){
  Serial.println("Access Granted");
  lcd.clear();
  lcd.print("Access Granted");
  s.write(180); // Điều khiển servo để mở khóa
  delay(1000); // Giữ khóa mở trong 5 giây
  s.write(90); // Đóng khóa lại
  re_input();
}
void do_when_denied(){
  Serial.println("Access Denied");
  lcd.clear();
  lcd.print("Access Denied");
  delay(1000); // Hiển thị "Access Denied" trong 3 giây
  re_input();
}

//phan thay doi mat khau hoac la uid thi phai check xem mat khau co trung voi mat khau cu khong va uid co trung voi uid cu khong, nue khong thi khong doi 
void change_master() {
  for(int i = 0; i < 5; i++){
    Serial.print(master_password[i]);
    Serial.print(" ");
  }
  Serial.println();

  //tao bien check dung sai 
  byte readUID[4];
  bool valid_master = false;
  lcd.clear();
  lcd.print("Master Pass:");
  lcd.setCursor(0, 1);
  // char master_input[6];
  int input_pos = 0;

  while(0 == 0){
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      for (byte i = 0; i < 4; i++) {
        readUID[i] = mfrc522.uid.uidByte[i];
      }
      if(validate_master_uid(readUID)){
        valid_master = true;
        lcd.clear();
        lcd.print("Grandted MasUID");
        delay(1000);
        break;
      } else {
        lcd.clear();
        lcd.print("Denied MasUID");
        delay(1000);
        lcd.clear();
        lcd.print("Master Password:");
        lcd.setCursor(0, 1);
        input_pos = 0;
      }
      mfrc522.PICC_HaltA(); // Stop reading
    } else {
      key = keypad.getKey();
      if (key) {
        if (key == 'A') {
          //khoi tao lai
          re_input();
          return;
        } else if (key == 'B') {
          //xoa ky tu
          // del_char();
          if(input_pos > 0){
            lcd.setCursor(input_pos-1, 1);
            lcd.print(" ");
            input_pos--;
          }
          if(input_pos == 0){
            lcd.setCursor(0, 1);
          }
        } else if (key == 'C') {
          //thay doi master_pass va master_uid
          // change_master();
          // return;
        } else if (key == 'D') {
          //them xoa mat khau cho user
          // handle_user();
          //
        } else if (key == '#') {
          // Handle '#' key (if required)
        } else if (key == '*') {
          // Handle '*' key (if required)
        } else {
          tmp_pass[input_pos++] = key;
          lcd.print(key);
          lcd.setCursor(input_pos - 1, 1);
          delay(300);
          lcd.print("*");
        }
      }

      if (input_pos == 5) {
        tmp_pass[input_pos] = '\0';
        delay(300);
        if (validate_master_pass(tmp_pass)) {
          valid_master = true;
          Serial.println("valid_naster: true");
        } else {
          Serial.println("valid_master: false");
        }
        break;
      }
      delay(50); 
    }
  }
  if(valid_master){
    lcd.clear();
    lcd.print("1. Change MasPass");
    lcd.setCursor(0, 1);
    lcd.print("2. Chang MasUID");
    while (true) {
      char key = keypad.getKey();
      if (key == '1') {
        // Change master password
        lcd.clear();
        lcd.print("New Master Pass:");
        lcd.setCursor(0, 1);

        char new_master_pass[6];
        int input_pos = 0;
        while (input_pos < 5) {
          key = keypad.getKey();
          if (key) {
            if(key == 'A'){
              re_input();
              return;
            } else if(key == 'B'){
              if(input_pos > 0){
                lcd.setCursor(input_pos-1, 1);
                lcd.print(" ");
                input_pos--;
              }
              if(input_pos == 0){
                lcd.setCursor(0, 1);
              }
            } else if(key == 'C'){

            } else if(key == 'D'){

            } else if(key == '*'){

            } else if(key == '#'){

            } else {
              new_master_pass[input_pos] = key;
              lcd.print(key);
              lcd.setCursor(input_pos, 1);
              delay(300);
              lcd.print("*");
              input_pos++;
            }
          }
        }
        new_master_pass[5] = '\0';

        // Update EEPROM with new master password
        if(strncmp(new_master_pass, master_password, 5) == 0){
          lcd.clear();
          lcd.print("Old master pas!");
          delay(1000);
          re_input();
          return;
        }
        for (int i = 0; i < 5; i++) {
          EEPROM.write(9 + i, new_master_pass[i]);
          master_password[i] = new_master_pass[i];
        }
        lcd.clear();
        lcd.print("Pass Updated");
        delay(1000);
        re_input();
        return;
      } else if (key == '2') {
        // Change master UID
        lcd.clear();
        lcd.print("Scan New UID");

        bool uid_received = false;
        while (!uid_received) {
          if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
            uid_received = true;
          }
        }
        if(validate_master_uid(mfrc522.uid.uidByte)){
          lcd.clear();
          lcd.print("Old master uid!");
          delay(1000);
          re_input();
          return;
        }
        // Update EEPROM with new master UID
        for (byte i = 0; i < 4; i++) {
          master_uid[i] = mfrc522.uid.uidByte[i];
          EEPROM.write(15 + i, master_uid[i]);
        }

        lcd.clear();
        lcd.print("UID Updated");
        delay(1000);
        re_input();
        return;
      } else if(key == 'A'){
        re_input();
        return;
      }
    }
  } else {
    lcd.clear();
    lcd.print("Wrong pass");
    delay(1000);
    re_input();
  }
}

void del_char(){
  if(i > 0){
    lcd.setCursor(i-1, 1);
    lcd.print(" ");
    i--;
  }
  if(i == 0){
    lcd.setCursor(0, 1);
  }
}
void re_input(){
  i = 0;
  lcd.clear();
  lcd.print("Enter Password:");
  lcd.setCursor(0,1);
}

       
