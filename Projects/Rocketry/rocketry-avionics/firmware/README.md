Firmware

I2C_Scanner:
Used to detect I2C devices connected to the ESP32

BMP388_Test:
First avionics sensor added. Reads altitude, pressure, and temperature from the BMP388

BMP388 Calibration Test:
Added a 5 second startup window, for the chip to calibrate
Added altitude calibration relative to start point
Achieved relatively stable altitude readings (~±0.5 m)