
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>

#include <Adafruit_BMP3XX.h>
#include "ICM42688.h"

// =====================================
// SENSOR LOGGER V1.2
// DUAL I2C + SD CARD
// 60-SECOND BENCH TEST
// =====================================

// =====================================
// PIN CONFIGURATION
// =====================================

// ICM42688 - I2C Bus 0
#define IMU_SDA 21
#define IMU_SCL 22
#define IMU_ADDRESS 0x69

// BMP388 - I2C Bus 1
#define BMP_SDA 25
#define BMP_SCL 26
#define BMP_ADDRESS 0x77

// SD Card - SPI
#define SD_CS   5
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23

// =====================================
// TEST CONFIGURATION
// =====================================

const uint32_t TEST_DURATION_MS = 60000;
const uint32_t LOG_INTERVAL_US = 20000;

// A gap larger than 30 ms is counted
// as a significant timing delay.
const uint32_t GAP_THRESHOLD_US = 30000;

// Flush every 250 samples (~5 seconds).
const uint32_t FLUSH_INTERVAL = 250;

const uint32_t I2C_SPEED = 100000;

const float SEA_LEVEL_HPA = 1013.25f;

// =====================================
// SENSOR OBJECTS
// =====================================

TwoWire &IMUBus = Wire;
TwoWire &BMPBus = Wire1;

ICM42688 IMU(IMUBus, IMU_ADDRESS);
Adafruit_BMP3XX bmp;

File logFile;

char logFilename[20] = {0};

// =====================================
// GLOBAL VARIABLES
// =====================================

uint32_t testStartMs = 0;
uint32_t previousSampleUs = 0;
uint32_t previousReportMs = 0;

uint32_t sampleCount = 0;

uint32_t imuFailures = 0;
uint32_t bmpFailures = 0;
uint32_t timingGaps = 0;

uint32_t maxIntervalUs = 0;

bool testComplete = false;

// =====================================
// HALT ON INITIALIZATION FAILURE
// =====================================

void haltSystem(const char* message) {

  Serial.println();
  Serial.println(message);
  Serial.println("SYSTEM HALTED");

  while (true) {
    delay(1000);
  }
}

// =====================================
// CSV HELPER
// =====================================

void writeValue(
  bool valid,
  float value,
  uint8_t precision
) {

  if (valid) {
    logFile.print(value, precision);
  }
}

// =====================================
// CREATE UNIQUE LOG FILE
// =====================================

bool createLogFile() {

  // Create LOG000.CSV, LOG001.CSV, etc.
  // Never overwrite an existing run.

  for (int i = 0; i < 1000; i++) {

    snprintf(
      logFilename,
      sizeof(logFilename),
      "/LOG%03d.CSV",
      i
    );

    if (!SD.exists(logFilename)) {

      logFile = SD.open(
        logFilename,
        FILE_WRITE
      );

      if (logFile) {
        return true;
      }

      return false;
    }
  }

  return false;
}

// =====================================
// FINAL REPORT
// =====================================

void finishTest() {

  testComplete = true;

  uint32_t duration =
    millis() - testStartMs;

  logFile.flush();
  logFile.close();

  float actualHz = 0.0f;

  if (duration > 0) {

    actualHz =
      (sampleCount * 1000.0f) / duration;
  }

  float imuErrorRate = 0.0f;
  float bmpErrorRate = 0.0f;

  if (sampleCount > 0) {

    imuErrorRate =
      (imuFailures * 100.0f) /
      sampleCount;

    bmpErrorRate =
      (bmpFailures * 100.0f) /
      sampleCount;
  }

  Serial.println();
  Serial.println("==============================");
  Serial.println("FINAL TEST RESULTS");
  Serial.println("==============================");

  Serial.print("Duration (ms): ");
  Serial.println(duration);

  Serial.print("Total samples: ");
  Serial.println(sampleCount);

  Serial.print("Average rate (Hz): ");
  Serial.println(actualHz, 2);

  Serial.print("IMU failures: ");
  Serial.println(imuFailures);

  Serial.print("IMU error rate (%): ");
  Serial.println(imuErrorRate, 3);

  Serial.print("BMP failures: ");
  Serial.println(bmpFailures);

  Serial.print("BMP error rate (%): ");
  Serial.println(bmpErrorRate, 3);

  Serial.print("Timing gaps >30 ms: ");
  Serial.println(timingGaps);

  Serial.print("Maximum interval (us): ");
  Serial.println(maxIntervalUs);

  Serial.print("Saved CSV: ");
  Serial.println(logFilename);

  Serial.println();
  Serial.println("CSV FLUSHED AND CLOSED");
  Serial.println("TEST COMPLETE");
  Serial.println("==============================");
}

// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("==============================");
  Serial.println("ESP32 SENSOR LOGGER V1.2");
  Serial.println("60-SECOND BENCH TEST");
  Serial.println("==============================");

  // =====================================
  // INITIALIZE TWO I2C BUSES
  // =====================================

  IMUBus.begin(IMU_SDA, IMU_SCL);
  IMUBus.setClock(I2C_SPEED);

  BMPBus.begin(BMP_SDA, BMP_SCL);
  BMPBus.setClock(I2C_SPEED);

  Serial.println("I2C buses configured.");

  // =====================================
  // INITIALIZE IMU
  // =====================================

  Serial.println("Starting ICM42688-P...");

  int imuStatus = IMU.begin();

  Serial.print("IMU status: ");
  Serial.println(imuStatus);

  if (imuStatus != 1) {
    haltSystem("ERROR: IMU initialization failed.");
  }

  Serial.println("ICM42688-P OK");

  // =====================================
  // INITIALIZE BMP388
  // =====================================

  Serial.println("Starting BMP388...");

  if (!bmp.begin_I2C(BMP_ADDRESS, &BMPBus)) {
    haltSystem("ERROR: BMP388 initialization failed.");
  }

  bmp.setTemperatureOversampling(
    BMP3_OVERSAMPLING_2X
  );

  bmp.setPressureOversampling(
    BMP3_OVERSAMPLING_2X
  );

  bmp.setIIRFilterCoeff(
    BMP3_IIR_FILTER_COEFF_3
  );

  Serial.println("BMP388 OK");

  // =====================================
  // BMP388 STARTUP WARM-UP
  // =====================================

  Serial.println("Warming up BMP388...");

  bool bmpStable = false;

  float previousPressure = 0.0f;
  int stableCount = 0;

  for (int i = 0; i < 20; i++) {

    if (bmp.performReading()) {

      float p = bmp.pressure / 100.0f;

      // Reject clearly implausible pressure.
      if (p > 300.0f && p < 1250.0f) {

        if (previousPressure > 0.0f &&
            fabsf(p - previousPressure) < 5.0f) {

          stableCount++;

        } else {
          stableCount = 0;
        }

        previousPressure = p;

        if (stableCount >= 3) {
          bmpStable = true;
          break;
        }
      }
    }

    delay(25);
  }

  if (!bmpStable) {
    haltSystem("ERROR: BMP388 startup readings unstable.");
  }

  Serial.println("BMP388 warm-up complete.");

  // =====================================
  // INITIALIZE SD CARD
  // =====================================

  Serial.println("Initializing SD card...");

  SPI.begin(
    SD_SCK,
    SD_MISO,
    SD_MOSI,
    SD_CS
  );

  if (!SD.begin(SD_CS, SPI, 1000000)) {
    haltSystem("ERROR: SD card initialization failed.");
  }

  Serial.println("SD CARD OK");

  // =====================================
  // CREATE LOG FILE
  // =====================================

  if (!createLogFile()) {
    haltSystem("ERROR: Could not create log file.");
  }

  Serial.print("Logging to: ");
  Serial.println(logFilename);

  // =====================================
  // CSV HEADER
  // =====================================

  logFile.println(
    "time_ms,"
    "sample,"
    "interval_us,"
    "imu_ok,"
    "bmp_ok,"
    "imu_status,"
    "accel_x_g,"
    "accel_y_g,"
    "accel_z_g,"
    "gyro_x_dps,"
    "gyro_y_dps,"
    "gyro_z_dps,"
    "imu_temp_c,"
    "pressure_hpa,"
    "bmp_temp_c,"
    "altitude_m"
  );

  logFile.flush();

  // =====================================
  // START TEST
  // =====================================

  testStartMs = millis();
  previousSampleUs = micros();
  previousReportMs = testStartMs;

  Serial.println();
  Serial.println("==============================");
  Serial.println("SYSTEM READY");
  Serial.println("Target: 50 Hz");
  Serial.println("Duration: 60 seconds");
  Serial.println("Flush interval: 250 samples");
  Serial.println("==============================");
}

// =====================================
// MAIN LOOP
// =====================================

void loop() {

  if (testComplete) {
    delay(1000);
    return;
  }

  // =====================================
  // AUTOMATIC END
  // =====================================

  if (millis() - testStartMs >=
      TEST_DURATION_MS) {

    finishTest();
    return;
  }

  // =====================================
  // SAMPLING SCHEDULER
  // =====================================

  uint32_t nowUs = micros();

  uint32_t elapsedUs =
    nowUs - previousSampleUs;

  if (elapsedUs < LOG_INTERVAL_US) {
    return;
  }

  previousSampleUs = nowUs;

  // =====================================
  // TIMING ANALYSIS
  // =====================================

  if (sampleCount > 0) {

    if (elapsedUs > GAP_THRESHOLD_US) {
      timingGaps++;
    }

    if (elapsedUs > maxIntervalUs) {
      maxIntervalUs = elapsedUs;
    }
  }

  // =====================================
  // READ IMU
  // =====================================

  int imuStatus = IMU.getAGT();

  bool imuOK = (imuStatus > 0);

  float ax = 0;
  float ay = 0;
  float az = 0;

  float gx = 0;
  float gy = 0;
  float gz = 0;

  float imuTemp = 0;

  if (imuOK) {

    ax = IMU.accX();
    ay = IMU.accY();
    az = IMU.accZ();

    gx = IMU.gyrX();
    gy = IMU.gyrY();
    gz = IMU.gyrZ();

    imuTemp = IMU.temp();

  } else {
    imuFailures++;
  }

  // =====================================
  // READ BMP388
  // =====================================

  bool bmpOK = bmp.performReading();

  float pressureHpa = 0;
  float bmpTemp = 0;
  float altitudeM = 0;

  if (bmpOK) {

    pressureHpa =
      bmp.pressure / 100.0f;

    bmpTemp =
      bmp.temperature;

    // Reject invalid pressure.
    if (pressureHpa < 300.0f ||
        pressureHpa > 1250.0f) {

      bmpOK = false;
    }

    if (bmpOK) {

      altitudeM =
        44330.0f *
        (1.0f - powf(
          pressureHpa / SEA_LEVEL_HPA,
          0.1903f
        ));
    }
  }

  if (!bmpOK) {
    bmpFailures++;
  }

  // =====================================
  // WRITE CSV
  // =====================================

  uint32_t timestamp =
    millis() - testStartMs;

  logFile.print(timestamp);
  logFile.print(",");

  logFile.print(sampleCount);
  logFile.print(",");

  logFile.print(elapsedUs);
  logFile.print(",");

  logFile.print(imuOK ? 1 : 0);
  logFile.print(",");

  logFile.print(bmpOK ? 1 : 0);
  logFile.print(",");

  logFile.print(imuStatus);
  logFile.print(",");

  writeValue(imuOK, ax, 4);
  logFile.print(",");

  writeValue(imuOK, ay, 4);
  logFile.print(",");

  writeValue(imuOK, az, 4);
  logFile.print(",");

  writeValue(imuOK, gx, 3);
  logFile.print(",");

  writeValue(imuOK, gy, 3);
  logFile.print(",");

  writeValue(imuOK, gz, 3);
  logFile.print(",");

  writeValue(imuOK, imuTemp, 2);
  logFile.print(",");

  writeValue(bmpOK, pressureHpa, 2);
  logFile.print(",");

  writeValue(bmpOK, bmpTemp, 2);
  logFile.print(",");

  writeValue(bmpOK, altitudeM, 2);

  logFile.println();

  sampleCount++;

  // =====================================
  // PERIODIC SD FLUSH
  // =====================================

  if (sampleCount % FLUSH_INTERVAL == 0) {
    logFile.flush();
  }

  // =====================================
  // SERIAL STATUS EVERY SECOND
  // =====================================

  if (millis() - previousReportMs >= 1000) {

    previousReportMs = millis();

    Serial.println();
    Serial.println("--- LOGGER STATUS ---");

    Serial.print("Elapsed (s): ");
    Serial.println(
      (millis() - testStartMs) / 1000.0f,
      1
    );

    Serial.print("Samples: ");
    Serial.println(sampleCount);

    Serial.print("IMU failures: ");
    Serial.println(imuFailures);

    Serial.print("BMP failures: ");
    Serial.println(bmpFailures);

    Serial.print("Timing gaps >30 ms: ");
    Serial.println(timingGaps);

    Serial.println("---------------------");
  }
}
