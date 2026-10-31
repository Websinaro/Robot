#include <Arduino.h>

//Robot V! firmware
// BOOT

#define ROBOT_NAME "WEBY"
#define SERIAL_BAUD 115200

unsigned long previousMillis = 0;
const unsigned long heartbeatInterval = 3000;

void setup() {
  Serial.begin(SERIAL_BAUD);

  delay(1500);

  Serial.println(" ROBOT V! BOOT");  
}

void loop() {
  unsigned long currentMillis = millis();

  if(currentMillis-previousMillis >=heartbeatInterval) {
    previousMillis = currentMillis;
    Serial.println("Robot booted successfully");
  }
}
