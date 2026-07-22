#include <Wire.h>
#include <Adafruit_BMP3XX.h>
#include "ICM42688.h"
#include <math.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define BMP388_ADDRESS 0x77
#define ICM42688_ADDRESS 0x68

#define SEA_LEVEL_PRESSURE_HPA 1013.25

Adafruit_BMP3XX bmp;
ICM42688 IMU(Wire, ICM42688_ADDRESS);



float groundAltitude = 0.0;
float filteredAltitude = 0.0;

const float ALTITUDE_FILTER_ALPHA = 0.20;


// --------------------
// IMU orientation variables
// --------------------

float roll = 0.0;
float pitch = 0.0;
float yaw = 0.0;

float gyroBiasX = 0.0;
float gyroBiasY = 0.0;
float gyroBiasZ = 0.0;

const float ORIENTATION_FILTER_STRENGTH = 0.98;


unsigned long previousImuUpdateTime = 0;
unsigned long previousBmpUpdateTime = 0;
unsigned long previousPrintTime = 0;

const unsigned long IMU_INTERVAL_US = 20000;  // 50 Hz
const unsigned long BMP_INTERVAL_MS = 40;     // 25 Hz
const unsigned long PRINT_INTERVAL_MS = 200;  // 5 Hz


float accelX = 0.0;
float accelY = 0.0;
float accelZ = 0.0;

float gyroX = 0.0;
float gyroY = 0.0;
float gyroZ = 0.0;

float pressureHpa = 0.0;
float temperatureC = 0.0;



bool readImu() {

  for (int attempt = 0; attempt < 5; attempt++) {

    if (IMU.getAGT() > 0) {
      return true;
    }

    delay(2);
  }

  return false;
}


void calibrateGyroBias() {

  Serial.println("Calibrating remaining gyro bias...");
  Serial.println("DO NOT MOVE THE SENSOR.");

  const int samples = 500;

  float sumX = 0.0;
  float sumY = 0.0;
  float sumZ = 0.0;

  int successfulSamples = 0;

  while (successfulSamples < samples) {

    if (readImu()) {

      sumX += IMU.gyrX();
      sumY += IMU.gyrY();
      sumZ += IMU.gyrZ();

      successfulSamples++;
    }

    delay(5);
  }

  gyroBiasX = sumX / samples;
  gyroBiasY = sumY / samples;
  gyroBiasZ = sumZ / samples;

  Serial.print("Gyro bias X: ");
  Serial.println(gyroBiasX, 6);

  Serial.print("Gyro bias Y: ");
  Serial.println(gyroBiasY, 6);

  Serial.print("Gyro bias Z: ");
  Serial.println(gyroBiasZ, 6);
}


void calibrateGroundAltitude() {

  Serial.println("Calibrating ground altitude...");
  Serial.println("DO NOT MOVE THE SYSTEM.");

  float altitudeSum = 0.0;
  int successfulSamples = 0;

  const int samples = 300;

  while (successfulSamples < samples) {

    if (bmp.performReading()) {

      altitudeSum +=
        bmp.readAltitude(SEA_LEVEL_PRESSURE_HPA);

      successfulSamples++;
    }

    delay(20);
  }

  groundAltitude =
    altitudeSum / successfulSamples;

  filteredAltitude = 0.0;

  Serial.print("Ground altitude: ");
  Serial.print(groundAltitude, 2);
  Serial.println(" m");
}


void setup() {

  Serial.begin(115200);

  delay(2000);

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println();
  Serial.println("ESP32 Avionics Sensor Integration Test");


  // Initialize BMP388

  if (!bmp.begin_I2C(BMP388_ADDRESS)) {

    Serial.println("BMP388 initialization failed!");

    while (1) {
      delay(100);
    }
  }

  bmp.setTemperatureOversampling(
    BMP3_OVERSAMPLING_8X
  );

  bmp.setPressureOversampling(
    BMP3_OVERSAMPLING_8X
  );

  bmp.setIIRFilterCoeff(
    BMP3_IIR_FILTER_COEFF_3
  );

  Serial.println("BMP388 connected!");



  Serial.println("Starting ICM42688-P...");

  int imuStatus = IMU.begin();

  if (imuStatus < 0) {

    Serial.println("ICM42688-P initialization failed!");

    Serial.print("Status: ");
    Serial.println(imuStatus);

    while (1) {
      delay(100);
    }
  }

  Serial.println("ICM42688-P connected!");



  Serial.println("Warming sensors for 10 seconds...");

  unsigned long warmupStart = millis();

  while (millis() - warmupStart < 10000) {

    readImu();
    bmp.performReading();

    delay(10);
  }


  calibrateGyroBias();
  calibrateGroundAltitude();


  if (!readImu()) {

    Serial.println("Initial IMU reading failed!");

    while (1) {
      delay(100);
    }
  }

  accelX = IMU.accX();
  accelY = IMU.accY();
  accelZ = IMU.accZ();

  roll =
    atan2(accelY, accelZ) *
    180.0 / PI;

  pitch =
    atan2(
      -accelX,
      sqrt(
        (accelY * accelY) +
        (accelZ * accelZ)
      )
    ) *
    180.0 / PI;

  yaw = 0.0;


  previousImuUpdateTime = micros();
  previousBmpUpdateTime = millis();
  previousPrintTime = millis();


  Serial.println("SYSTEM READY");

  Serial.println(
    "time_ms,altitude_m,pressure_hpa,"
    "temp_c,ax_g,ay_g,az_g,"
    "gx_dps,gy_dps,gz_dps,"
    "roll_deg,pitch_deg,yaw_deg"
  );
}



void loop() {

  unsigned long currentMicros = micros();
  unsigned long currentMillis = millis();



  if (
    currentMicros - previousImuUpdateTime
    >= IMU_INTERVAL_US
  ) {

    float deltaTime =
      (currentMicros - previousImuUpdateTime)
      / 1000000.0;

    previousImuUpdateTime = currentMicros;


    if (
      deltaTime > 0.0 &&
      deltaTime < 0.1 &&
      readImu()
    ) {

      accelX = IMU.accX();
      accelY = IMU.accY();
      accelZ = IMU.accZ();

      gyroX = IMU.gyrX() - gyroBiasX;
      gyroY = IMU.gyrY() - gyroBiasY;
      gyroZ = IMU.gyrZ() - gyroBiasZ;


      float accelerometerRoll =
        atan2(accelY, accelZ) *
        180.0 / PI;

      float accelerometerPitch =
        atan2(
          -accelX,
          sqrt(
            (accelY * accelY) +
            (accelZ * accelZ)
          )
        ) *
        180.0 / PI;


      float gyroRoll =
        roll + (gyroX * deltaTime);

      float gyroPitch =
        pitch + (gyroY * deltaTime);


      roll =
        (
          ORIENTATION_FILTER_STRENGTH *
          gyroRoll
        ) +
        (
          (1.0 - ORIENTATION_FILTER_STRENGTH) *
          accelerometerRoll
        );

      pitch =
        (
          ORIENTATION_FILTER_STRENGTH *
          gyroPitch
        ) +
        (
          (1.0 - ORIENTATION_FILTER_STRENGTH) *
          accelerometerPitch
        );

      yaw += gyroZ * deltaTime;
    }
  }


  if (
    currentMillis - previousBmpUpdateTime
    >= BMP_INTERVAL_MS
  ) {

    previousBmpUpdateTime = currentMillis;


    if (bmp.performReading()) {

      float currentAltitude =
        bmp.readAltitude(
          SEA_LEVEL_PRESSURE_HPA
        );

      float rawRelativeAltitude =
        currentAltitude - groundAltitude;


      filteredAltitude =
        (
          ALTITUDE_FILTER_ALPHA *
          rawRelativeAltitude
        ) +
        (
          (1.0 - ALTITUDE_FILTER_ALPHA) *
          filteredAltitude
        );


      pressureHpa =
        bmp.pressure / 100.0;

      temperatureC =
        bmp.temperature;
    }
  }



  if (
    currentMillis - previousPrintTime
    >= PRINT_INTERVAL_MS
  ) {

    previousPrintTime = currentMillis;


    Serial.print(currentMillis);
    Serial.print(",");

    Serial.print(filteredAltitude, 2);
    Serial.print(",");

    Serial.print(pressureHpa, 2);
    Serial.print(",");

    Serial.print(temperatureC, 2);
    Serial.print(",");


    Serial.print(accelX, 4);
    Serial.print(",");

    Serial.print(accelY, 4);
    Serial.print(",");

    Serial.print(accelZ, 4);
    Serial.print(",");


    Serial.print(gyroX, 3);
    Serial.print(",");

    Serial.print(gyroY, 3);
    Serial.print(",");

    Serial.print(gyroZ, 3);
    Serial.print(",");


    Serial.print(roll, 2);
    Serial.print(",");

    Serial.print(pitch, 2);
    Serial.print(",");

    Serial.println(yaw, 2);
  }
}