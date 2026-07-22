#include <Wire.h>
#include "ICM42688.h"
#include <math.h>

#define SDA_PIN 21
#define SCL_PIN 22

ICM42688 IMU(Wire, 0x68);

float roll = 0.0;
float pitch = 0.0;
float yaw = 0.0;

unsigned long previousUpdateTime = 0;
unsigned long previousPrintTime = 0;

const float filterStrength = 0.98;

const unsigned long UPDATE_INTERVAL_US = 20000; // 20 ms = 50 Hz
const unsigned long PRINT_INTERVAL_MS = 200;    // 200 ms = 5 Hz


void setup() {

  Serial.begin(115200);

  delay(2000);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Starting ICM42688 orientation test...");
  Serial.println("Keep the sensor completely still during startup.");

  int status = IMU.begin();

  if (status < 0) {

    Serial.println("IMU initialization unsuccessful");

    Serial.print("Status: ");
    Serial.println(status);

    while (1) {
      delay(100);
    }
  }

  Serial.println("IMU connected!");

  if (IMU.getAGT() < 0) {

    Serial.println("Initial sensor reading failed");

    while (1) {
      delay(100);
    }
  }

  float ax = IMU.accX();
  float ay = IMU.accY();
  float az = IMU.accZ();

  roll = atan2(ay, az) * 180.0 / PI;

  pitch = atan2(
    -ax,
    sqrt((ay * ay) + (az * az))
  ) * 180.0 / PI;

  yaw = 0.0;

  previousUpdateTime = micros();
  previousPrintTime = millis();

  Serial.println("Orientation system ready!");
  Serial.println("roll_deg,pitch_deg,yaw_deg");
}


void loop() {

  unsigned long currentUpdateTime = micros();

  if (currentUpdateTime - previousUpdateTime < UPDATE_INTERVAL_US) {
    return;
  }

  float deltaTime =
    (currentUpdateTime - previousUpdateTime) / 1000000.0;

  previousUpdateTime = currentUpdateTime;

  if (deltaTime <= 0.0 || deltaTime > 0.1) {
    return;
  }

  if (IMU.getAGT() < 0) {

    Serial.println("Sensor reading failed");
    return;
  }

  float ax = IMU.accX();
  float ay = IMU.accY();
  float az = IMU.accZ();

  float gx = IMU.gyrX();
  float gy = IMU.gyrY();
  float gz = IMU.gyrZ();

  float accelerometerRoll =
    atan2(ay, az) * 180.0 / PI;

  float accelerometerPitch =
    atan2(
      -ax,
      sqrt((ay * ay) + (az * az))
    ) * 180.0 / PI;

  float gyroRoll = roll + (gx * deltaTime);
  float gyroPitch = pitch + (gy * deltaTime);

  roll =
    (filterStrength * gyroRoll) +
    ((1.0 - filterStrength) * accelerometerRoll);

  pitch =
    (filterStrength * gyroPitch) +
    ((1.0 - filterStrength) * accelerometerPitch);

  yaw += gz * deltaTime;

  unsigned long currentPrintTime = millis();

  if (currentPrintTime - previousPrintTime >= PRINT_INTERVAL_MS) {

    previousPrintTime = currentPrintTime;

    Serial.print(roll, 2);
    Serial.print(",");

    Serial.print(pitch, 2);
    Serial.print(",");

    Serial.println(yaw, 2);
  }
}