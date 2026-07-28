#include <SPI.h>
#include <SD.h>

#define SD_CS 15

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD failed!");
    return;
  }

  Serial.println("SD OK");

  File testFile = SD.open("/test.txt", FILE_WRITE);

  if (testFile) {
    Serial.println("File opened!");
    testFile.println("Hello from ESP32");
    testFile.close();
    Serial.println("File written!");
  } 
  else {
    Serial.println("File open failed!");
  }
}

void loop() {

}