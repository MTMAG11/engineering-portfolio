#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <Adafruit_BMP3XX.h>
#include "ICM42688.h"


// --------------------
// Pin configuration
// --------------------

#define SDA_PIN 21
#define SCL_PIN 22

#define SD_CS   5
#define SD_SCK  18
#define SD_MISO 19
#define SD_MOSI 23

#define BMP388_ADDRESS 0x77
#define ICM42688_ADDRESS 0x68


// --------------------
// Sensor objects
// --------------------

Adafruit_BMP3XX bmp;
ICM42688 IMU(Wire, ICM42688_ADDRESS);

File logFile;


// --------------------
// Logging configuration
// --------------------

const unsigned long LOG_INTERVAL_MS = 20;  // 50 Hz

unsigned long previousLogTime = 0;

unsigned long sampleCount = 0;


// --------------------
// Sensor data
// --------------------

float accelX = 0.0;
float accelY = 0.0;
float accelZ = 0.0;

float gyroX = 0.0;
float gyroY = 0.0;
float gyroZ = 0.0;

float pressureHpa = 0.0;
float temperatureC = 0.0;
float altitudeM = 0.0;


// --------------------
// IMU reading
// --------------------

bool readImu() {

  for (int attempt = 0; attempt < 5; attempt++) {

    if (IMU.getAGT() > 0) {
      return true;
    }

    delay(2);
  }

  return false;
}


// --------------------
// Setup
// --------------------

void setup() {

  Serial.begin(115200);

  delay(2000);

  Serial.println();
  Serial.println("Autonomous Rocket - Sensor Logger V1");
  Serial.println();


  // --------------------
  // Initialize I2C
  // --------------------

  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Initializing sensors...");


  // --------------------
  // Initialize BMP388
  // --------------------

  if (!bmp.begin_I2C(BMP388_ADDRESS)) {

    Serial.println("ERROR: BMP388 initialization failed.");

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

  Serial.println("BMP388 OK");


  // --------------------
  // Initialize ICM-42688P
  // --------------------

  int imuStatus = IMU.begin();

  if (imuStatus < 0) {

    Serial.println("ERROR: ICM42688-P initialization failed.");
    Serial.print("Status: ");
    Serial.println(imuStatus);

    while (1) {
      delay(100);
    }
  }

  Serial.println("ICM42688-P OK");


  // --------------------
  // Initialize SD card
  // --------------------

  Serial.println("Initializing SD card...");

  SPI.begin(
    SD_SCK,
    SD_MISO,
    SD_MOSI,
    SD_CS
  );

  if (!SD.begin(SD_CS, SPI, 1000000)) {

    Serial.println("ERROR: SD card initialization failed.");

    while (1) {
      delay(100);
    }
  }

  Serial.println("SD card OK");

  Serial.print("Card size: ");
  Serial.print(
    SD.cardSize() / (1024 * 1024)
  );
  Serial.println(" MB");


  // --------------------
  // Create log file
  // --------------------

  logFile = SD.open(
    "/flight_log.csv",
    FILE_WRITE
  );

  if (!logFile) {

    Serial.println("ERROR: Could not create log file.");

    while (1) {
      delay(100);
    }
  }


  // --------------------
  // Write CSV header
  // --------------------

  logFile.println(
    "time_ms,"
    "sample,"
    "accel_x_g,"
    "accel_y_g,"
    "accel_z_g,"
    "gyro_x_dps,"
    "gyro_y_dps,"
    "gyro_z_dps,"
    "pressure_hpa,"
    "temperature_c,"
    "altitude_m"
  );

  logFile.flush();


  // --------------------
  // Ready
  // --------------------

  previousLogTime = millis();

  Serial.println();
  Serial.println("SYSTEM READY");
  Serial.println(
    "Logging sensor data at 50 Hz..."
  );
}


// --------------------
// Main loop
// --------------------

void loop() {

  unsigned long currentTime = millis();


  if (
    currentTime - previousLogTime
    < LOG_INTERVAL_MS
  ) {
    return;
  }

  previousLogTime += LOG_INTERVAL_MS;


  // --------------------
  // Read IMU
  // --------------------

  if (!readImu()) {

    Serial.println("WARNING: IMU read failed.");

    return;
  }

  accelX = IMU.accX();
  accelY = IMU.accY();
  accelZ = IMU.accZ();

  gyroX = IMU.gyrX();
  gyroY = IMU.gyrY();
  gyroZ = IMU.gyrZ();


  // --------------------
  // Read BMP388
  // --------------------

  if (!bmp.performReading()) {

    Serial.println("WARNING: BMP388 read failed.");

    return;
  }

  pressureHpa =
    bmp.pressure / 100.0;

  temperatureC =
    bmp.temperature;

  altitudeM =
    bmp.readAltitude(1013.25);


  // --------------------
  // Timestamp
  // --------------------

  unsigned long timestamp =
    millis();


  // --------------------
  // Write CSV row
  // --------------------

  logFile.print(timestamp);
  logFile.print(",");

  logFile.print(sampleCount);
  logFile.print(",");

  logFile.print(accelX, 4);
  logFile.print(",");

  logFile.print(accelY, 4);
  logFile.print(",");

  logFile.print(accelZ, 4);
  logFile.print(",");

  logFile.print(gyroX, 3);
  logFile.print(",");

  logFile.print(gyroY, 3);
  logFile.print(",");

  logFile.print(gyroZ, 3);
  logFile.print(",");

  logFile.print(pressureHpa, 2);
  logFile.print(",");

  logFile.print(temperatureC, 2);
  logFile.print(",");

  logFile.println(altitudeM, 2);


  sampleCount++;


  // --------------------
  // Periodically flush
  // --------------------

  if (sampleCount % 50 == 0) {

    logFile.flush();

    Serial.print("Samples logged: ");
    Serial.println(sampleCount);
  }
}