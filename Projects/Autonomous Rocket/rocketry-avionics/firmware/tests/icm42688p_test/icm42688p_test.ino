
#include <Arduino.h>
#include <Wire.h>
#include "ICM42688.h"

ICM42688 IMU(Wire, 0x69);

unsigned long startTime = 0;
unsigned long totalReads = 0;
unsigned long failedReads = 0;
bool imuReady = false;
bool finished = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22);
  Wire.setClock(100000);

  Serial.println();
  Serial.println("=== ICM-42688 60-Second Reliability Test ===");

  int status = IMU.begin();

  Serial.print("IMU begin status: ");
  Serial.println(status);

  if (status != 1) {
    Serial.println("FAIL: Initialization unsuccessful.");
    return;
  }

  imuReady = true;
  startTime = millis();

  Serial.println("Testing... keep the board stationary.");
}

void loop() {
  if (!imuReady || finished) {
    delay(100);
    return;
  }

  int result = IMU.getAGT();
  totalReads++;

  if (result < 0) {
    failedReads++;
  }

  unsigned long elapsed = millis() - startTime;

  if (elapsed >= 60000) {
    finished = true;

    Serial.println();
    Serial.println("=== TEST RESULTS ===");

    Serial.print("Duration (ms): ");
    Serial.println(elapsed);

    Serial.print("Total reads: ");
    Serial.println(totalReads);

    Serial.print("Failed reads: ");
    Serial.println(failedReads);

    Serial.print("Successful reads: ");
    Serial.println(totalReads - failedReads);

    float errorPercent = totalReads > 0
      ? 100.0f * failedReads / totalReads
      : 0.0f;

    Serial.print("Error rate (%): ");
    Serial.println(errorPercent, 2);

    Serial.println("Test complete.");
    return;
  }

  delay(20);
}
