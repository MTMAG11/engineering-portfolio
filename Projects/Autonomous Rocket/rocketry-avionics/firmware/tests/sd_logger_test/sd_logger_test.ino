#include <SPI.h>
#include <SD.h>

#define SD_CS   5
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting...");

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, SPI, 1000000)) {
    Serial.println("SD init failed");
    return;
  }

  Serial.println("SD OK");

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

void loop() {
}