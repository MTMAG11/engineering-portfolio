#include <Wire.h>
#include <Adafruit_BMP3XX.h>

Adafruit_BMP3XX bmp;

#define SEALEVELPRESSURE_HPA 1013.25

float groundAltitude = 0;

float filteredAltitude = 0;

float alpha = 0.2;  // filtering strength


void setup() {

  Serial.begin(115200);

  Wire.begin(21, 22);

  Serial.println("BMP388 Filtering Test");


  if (!bmp.begin_I2C(0x77)) {

    Serial.println("BMP388 not found!");

    while (1);
  }


  Serial.println("BMP388 connected");


  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);


  Serial.println("Stabilizing sensor...");
  delay(10000);


  Serial.println("Discarding startup readings...");

  for (int i = 0; i < 100; i++) {

    bmp.performReading();

    delay(20);
  }


  Serial.println("Calibrating ground altitude...");


  float sum = 0;

  int samples = 500;


  for (int i = 0; i < samples; i++) {

    if (bmp.performReading()) {

      sum += bmp.readAltitude(SEALEVELPRESSURE_HPA);

    }

    delay(20);
  }


  groundAltitude = sum / samples;


  // Initialize filter with first reading
  if (bmp.performReading()) {

    filteredAltitude =
      bmp.readAltitude(SEALEVELPRESSURE_HPA) - groundAltitude;
  }


  Serial.print("Ground altitude: ");
  Serial.println(groundAltitude);


  Serial.println("READY");

}



void loop() {


  if (!bmp.performReading()) {

    return;
  }


  float currentAltitude =
    bmp.readAltitude(SEALEVELPRESSURE_HPA);


  float rawAltitude =
    currentAltitude - groundAltitude;



  filteredAltitude =
    (alpha * rawAltitude) +
    ((1 - alpha) * filteredAltitude);



  Serial.print("Raw: ");

  Serial.print(rawAltitude);


  Serial.print(" m   Filtered: ");

  Serial.print(filteredAltitude);


  Serial.println(" m");


  delay(100);

}