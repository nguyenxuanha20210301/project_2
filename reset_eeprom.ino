#include<EEPROM.h>
void setup(){
  Serial.begin(9600);
  for(int i = 0; i < EEPROM.length(); i++){
    EEPROM.write(i, 0xFF);
  }
  Serial.println("Done");
}
void loop(){
}
