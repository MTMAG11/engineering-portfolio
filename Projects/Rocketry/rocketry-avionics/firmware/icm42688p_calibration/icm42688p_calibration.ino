#include <Wire.h>
#include "ICM42688.h"

#define SDA_PIN 21
#define SCL_PIN 22
#define IMU_ADDRESS 0x68

ICM42688 IMU(Wire, IMU_ADDRESS);

const int CALIBRATION_SAMPLES = 500;

float gyroXBias = 0.0;
float gyroYBias = 0.0;
float gyroZBias = 0.0;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setTimeOut(100);

  Serial.println("Starting ICM42688...");

  int status = IMU.begin();

  if (status < 0) {
    Serial.print("IMU initialization failed. Status: ");
    Serial.println(status);

    while (true) {
      delay(1000);
    }
  }

  Serial.println("IMU connected.");
  Serial.println("Keep the sensor completely still.");
  delay(3000);

  Serial.println("Collecting calibration samples...");

  float gyroXSum = 0.0;
  float gyroYSum = 0.0;
  float gyroZSum = 0.0;

  int validSamples = 0;

  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    int readStatus = IMU.getAGT();

    if (readStatus > 0) {
      gyroXSum += IMU.gyrX();
      gyroYSum += IMU.gyrY();
      gyroZSum += IMU.gyrZ();
      validSamples++;
    } else {
      Serial.println("Warning: IMU read failed.");
    }

    delay(10);
  }

  if (validSamples == 0) {
    Serial.println("Calibration failed: no valid samples.");

    while (true) {
      delay(1000);
    }
  }

  gyroXBias = gyroXSum / validSamples;
  gyroYBias = gyroYSum / validSamples;
  gyroZBias = gyroZSum / validSamples;

  Serial.println();
  Serial.println("Calibration complete.");
  Serial.print("Valid samples: ");
  Serial.println(validSamples);

  Serial.print("Gyro X bias: ");
  Serial.print(gyroXBias, 6);
  Serial.println(" dps");

  Serial.print("Gyro Y bias: ");
  Serial.print(gyroYBias, 6);
  Serial.println(" dps");

  Serial.print("Gyro Z bias: ");
  Serial.print(gyroZBias, 6);
  Serial.println(" dps");

  Serial.println();
  Serial.println("Corrected gyro readings:");
}

void loop() {
  int readStatus = IMU.getAGT();

  if (readStatus < 0) {
    Serial.println("IMU read failed.");
    delay(100);
    return;
  }

  float correctedGyroX = IMU.gyrX() - gyroXBias;
  float correctedGyroY = IMU.gyrY() - gyroYBias;
  float correctedGyroZ = IMU.gyrZ() - gyroZBias;

  Serial.print("X: ");
  Serial.print(correctedGyroX, 3);
  Serial.print(" dps   Y: ");
  Serial.print(correctedGyroY, 3);
  Serial.print(" dps   Z: ");
  Serial.print(correctedGyroZ, 3);
  Serial.println(" dps");

  delay(100);
}