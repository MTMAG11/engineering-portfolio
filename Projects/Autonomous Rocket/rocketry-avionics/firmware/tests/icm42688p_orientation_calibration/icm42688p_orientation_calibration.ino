#include <Wire.h>
#include "ICM42688.h"
#include <math.h>

#define SDA_PIN 21
#define SCL_PIN 22

ICM42688 IMU(Wire, 0x68);

float roll = 0.0;
float pitch = 0.0;
float yaw = 0.0;

float gyroBiasX = 0.0;
float gyroBiasY = 0.0;
float gyroBiasZ = 0.0;

unsigned long previousUpdateTime = 0;
unsigned long previousPrintTime = 0;

const unsigned long UPDATE_INTERVAL_US = 20000;
const unsigned long PRINT_INTERVAL_MS = 200;

const float FILTER_STRENGTH = 0.98;

const int CALIBRATION_SAMPLES = 1000;
const int MAX_READ_ATTEMPTS = 5;

const float BIAS_LEARNING_RATE = 0.002;

const float MAX_STATIONARY_GYRO_DPS = 0.20;
const float MIN_STATIONARY_ACCEL_G = 0.97;
const float MAX_STATIONARY_ACCEL_G = 1.03;


bool readImuWithRetry() {

  for (int attempt = 0; attempt < MAX_READ_ATTEMPTS; attempt++) {

    if (IMU.getAGT() > 0) {
      return true;
    }

    delay(5);
  }

  return false;
}


void calibrateRemainingGyroBias() {

  Serial.println("Final gyro calibration...");
  Serial.println("DO NOT MOVE THE SENSOR.");

  float sumX = 0.0;
  float sumY = 0.0;
  float sumZ = 0.0;

  int successfulSamples = 0;
  int failedSamples = 0;

  while (successfulSamples < CALIBRATION_SAMPLES) {

    if (readImuWithRetry()) {

      sumX += IMU.gyrX();
      sumY += IMU.gyrY();
      sumZ += IMU.gyrZ();

      successfulSamples++;

    } else {

      failedSamples++;

      Serial.print("Calibration read failure: ");
      Serial.println(failedSamples);

      if (failedSamples >= 20) {

        Serial.println("Too many IMU communication failures.");
        Serial.println("Check SDA, SCL, power, ground, CS, and ADO.");

        while (1) {
          delay(100);
        }
      }
    }

    delay(5);
  }

  gyroBiasX = sumX / successfulSamples;
  gyroBiasY = sumY / successfulSamples;
  gyroBiasZ = sumZ / successfulSamples;

  Serial.println("Remaining gyro bias:");

  Serial.print("X: ");
  Serial.print(gyroBiasX, 6);
  Serial.println(" dps");

  Serial.print("Y: ");
  Serial.print(gyroBiasY, 6);
  Serial.println(" dps");

  Serial.print("Z: ");
  Serial.print(gyroBiasZ, 6);
  Serial.println(" dps");
}


void setup() {

  Serial.begin(115200);

  delay(2000);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  Serial.println("Starting stabilized orientation test...");
  Serial.println("Keep the sensor completely still.");

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

  Serial.println("Warming up IMU for 15 seconds...");

  unsigned long warmupStart = millis();
  int warmupFailures = 0;

  while (millis() - warmupStart < 15000) {

    if (!readImuWithRetry()) {
      warmupFailures++;
    }

    delay(10);
  }

  Serial.print("Warm-up read failures: ");
  Serial.println(warmupFailures);

  calibrateRemainingGyroBias();

  Serial.println("Taking initial orientation reading...");

  if (!readImuWithRetry()) {

    Serial.println("Could not get initial orientation reading.");
    Serial.println("Check the IMU wiring and I2C bus.");

    while (1) {
      delay(100);
    }
  }

  float ax = IMU.accX();
  float ay = IMU.accY();
  float az = IMU.accZ();

  roll =
    atan2(ay, az) *
    180.0 / PI;

  pitch =
    atan2(
      -ax,
      sqrt((ay * ay) + (az * az))
    ) *
    180.0 / PI;

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
    (currentUpdateTime - previousUpdateTime) /
    1000000.0;

  previousUpdateTime = currentUpdateTime;

  if (deltaTime <= 0.0 || deltaTime > 0.1) {
    return;
  }

  if (!readImuWithRetry()) {

    Serial.println("IMU read failed after retries");
    return;
  }

  float ax = IMU.accX();
  float ay = IMU.accY();
  float az = IMU.accZ();

  float rawGx = IMU.gyrX();
  float rawGy = IMU.gyrY();
  float rawGz = IMU.gyrZ();

  float accelerationMagnitude =
    sqrt(
      (ax * ax) +
      (ay * ay) +
      (az * az)
    );

  float gyroMagnitude =
    sqrt(
      (rawGx * rawGx) +
      (rawGy * rawGy) +
      (rawGz * rawGz)
    );

  bool sensorIsStationary =
    gyroMagnitude < MAX_STATIONARY_GYRO_DPS &&
    accelerationMagnitude > MIN_STATIONARY_ACCEL_G &&
    accelerationMagnitude < MAX_STATIONARY_ACCEL_G;

  if (sensorIsStationary) {

    gyroBiasX =
      ((1.0 - BIAS_LEARNING_RATE) * gyroBiasX) +
      (BIAS_LEARNING_RATE * rawGx);

    gyroBiasY =
      ((1.0 - BIAS_LEARNING_RATE) * gyroBiasY) +
      (BIAS_LEARNING_RATE * rawGy);

    gyroBiasZ =
      ((1.0 - BIAS_LEARNING_RATE) * gyroBiasZ) +
      (BIAS_LEARNING_RATE * rawGz);
  }

  float gx = rawGx - gyroBiasX;
  float gy = rawGy - gyroBiasY;
  float gz = rawGz - gyroBiasZ;

  float accelerometerRoll =
    atan2(ay, az) *
    180.0 / PI;

  float accelerometerPitch =
    atan2(
      -ax,
      sqrt((ay * ay) + (az * az))
    ) *
    180.0 / PI;

  float gyroRoll =
    roll + (gx * deltaTime);

  float gyroPitch =
    pitch + (gy * deltaTime);

  roll =
    (FILTER_STRENGTH * gyroRoll) +
    ((1.0 - FILTER_STRENGTH) * accelerometerRoll);

  pitch =
    (FILTER_STRENGTH * gyroPitch) +
    ((1.0 - FILTER_STRENGTH) * accelerometerPitch);

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