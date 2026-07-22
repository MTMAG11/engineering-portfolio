#include <Wire.h>
#include <Adafruit_BMP3XX.h>

Adafruit_BMP3XX bmp;

#define SEALEVELPRESSURE_HPA 1013.25

float groundAltitude = 0;


void setup() {

  Serial.begin(115200);

  Wire.begin(21, 22);

  Serial.println("BMP388 Calibration");


  if (!bmp.begin_I2C(0x77)) {
    Serial.println("BMP388 not found!");
    while (1);
  }


  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);


  Serial.println("Stabilizing sensor...");
  delay(10000);


  Serial.println("Discarding readings...");

  for (int i = 0; i < 100; i++) {

    bmp.performReading();

    delay(20);
  }


  Serial.println("Calibrating...");


  float sum = 0;

  int samples = 200;


  for (int i = 0; i < samples; i++) {

    if (bmp.performReading()) {

      float altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);

      sum += altitude;
    }

    delay(20);
  }


  groundAltitude = sum / samples;


  Serial.print("Ground altitude: ");
  Serial.println(groundAltitude);


  Serial.println("READY");
}



void loop() {

  if (!bmp.performReading()) {
    return;
  }


  float currentAltitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);

  float relativeAltitude = currentAltitude - groundAltitude;


  Serial.print("Current: ");
  Serial.print(currentAltitude);

  Serial.print("  Ground: ");
  Serial.print(groundAltitude);

  Serial.print("  Relative: ");
  Serial.println(relativeAltitude);


  delay(500);
}