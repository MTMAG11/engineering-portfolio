#include <Wire.h>

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  Serial.println("I2C Scanner");
}

void loop() {
  byte error;
  byte address;
  int devices = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {

    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device found at 0x");

      if (address < 16) {
        Serial.print("0");
      }

      Serial.println(address, HEX);
      devices++;
    }
  }

  if (devices == 0) {
    Serial.println("No devices found");
  }

  Serial.println("----------------");
  delay(3000);
}