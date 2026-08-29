#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define SD_CS 15

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed");
    return;
  }

  Serial.println("SD OK");

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No card");
    return;
  }

  Serial.print("Card size: ");
  Serial.print(SD.cardSize() / (1024 * 1024));
  Serial.println(" MB");

  File file = SD.open("/test.txt", FILE_WRITE);

  if (!file) {
    Serial.println("Could not open file");
    return;
  }

  file.println("Hello from ESP32");
  file.close();

  Serial.println("Write successful");

}

void loop() {}