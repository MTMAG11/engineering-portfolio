#include <Wire.h>
#include <Adafruit_BMP3XX.h>

Adafruit_BMP3XX bmp;

#define SEALEVELPRESSURE_HPA (1013.25)

void setup() {

  Serial.begin(115200);

  Wire.begin(21, 22);

  Serial.println("BMP388 Starting...");

  if (!bmp.begin_I2C(0x77)) {
    Serial.println("BMP388 not found!");
    while (1);
  }

  Serial.println("BMP388 Connected!");

  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
}

void loop() {

  if (!bmp.performReading()) {
    Serial.println("Failed reading");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(bmp.temperature);
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(bmp.pressure / 100.0);
  Serial.println(" hPa");

  Serial.print("Altitude: ");
  Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.println("----------------");

  delay(500);
}