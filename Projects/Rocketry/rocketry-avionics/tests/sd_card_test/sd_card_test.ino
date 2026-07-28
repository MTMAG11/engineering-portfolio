#include <SPI.h>
#include <SD.h>

#define SD_CS 15

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting SD diagnostic...");

  if (!SD.begin(SD_CS)) {
    Serial.println("SD begin failed!");

    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
      Serial.println("No SD card detected.");
    }

    return;
  }

  Serial.println("SD card detected!");

  uint64_t size = SD.cardSize() / (1024 * 1024);
  Serial.print("Card size: ");
  Serial.print(size);
  Serial.println(" MB");
}

void loop() {}