#include <Wire.h>
#include "ICM42688.h"

#define SDA_PIN 21
#define SCL_PIN 22

ICM42688 IMU(Wire, 0x68);


void setup() {

  Serial.begin(115200);

  delay(5000);

  Wire.begin(SDA_PIN, SCL_PIN);


  Serial.println("Starting ICM42688...");


  int status = IMU.begin();


  if (status < 0) {

    Serial.println("IMU initialization unsuccessful");

    Serial.print("Status: ");
    Serial.println(status);

    while (1);

  }


  Serial.println("IMU connected!");

  Serial.println("ax,ay,az,gx,gy,gz,temp_C");

}



void loop() {


  IMU.getAGT();


  Serial.print(IMU.accX(), 6);
  Serial.print("\t");

  Serial.print(IMU.accY(), 6);
  Serial.print("\t");

  Serial.print(IMU.accZ(), 6);
  Serial.print("\t");


  Serial.print(IMU.gyrX(), 6);
  Serial.print("\t");

  Serial.print(IMU.gyrY(), 6);
  Serial.print("\t");

  Serial.print(IMU.gyrZ(), 6);
  Serial.print("\t");


  Serial.println(IMU.temp(), 6);


  delay(100);

}