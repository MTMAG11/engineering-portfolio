#include <SPI.h>
#include <SD.h>

#define SD_CS   15
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println("SD Test");

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if(!SD.begin(SD_CS)) {

    Serial.println("SD FAILED");
    return;

  }

  Serial.println("SD OK");


  File file = SD.open("/test.txt", FILE_WRITE);

  if(file){

    file.println("hello");
    file.close();

    Serial.println("WRITE OK");

  }
  else {

    Serial.println("FILE FAILED");

  }

}

void loop(){}