/*
  SD Card Diagnostic — ESP32 Avionics

  This is NOT just a pass/fail test. It isolates the four most common
  causes of "SD begin failed" / intermittent detection on a breadboard
  SPI setup, one at a time, so the serial output tells you WHICH thing
  is broken instead of just THAT something is broken.

  It tests, in order:
    1. Card detection at a conservative (slow) SPI clock
    2. Card type + size reporting
    3. Write + read-back verification (catches silent write failures)
    4. Timing of the write operation (catches blocking behavior that
       will matter once this runs alongside sensor sampling)
    5. Re-test at a faster clock, so you can see if failures are
       clock-speed / signal-integrity related

  HOW TO USE:
  - Set SD_CS below to whatever pin you're currently wired to.
  - If you're unsure about wiring, breadboards are one of the least
    reliable ways to run SPI at high clock speeds — keep wires as
    short as possible and make sure CS, MOSI, MISO, SCK, 3.3V, and GND
    all have solid, unwiggly contact before assuming it's a code or
    pin-number problem.
*/

#include <SPI.h>
#include <SD.h>

// ---- CONFIG: change this to match your current wiring ----
#define SD_CS 15

// Two clock speeds to compare — if SLOW works but FAST fails,
// that's a strong signal it's a signal-integrity / wiring-length
// problem, not a pin or library problem.
const uint32_t SPI_CLOCK_SLOW = 4000000;   // 4 MHz — conservative
const uint32_t SPI_CLOCK_FAST = 25000000;  // 25 MHz — SD library default-ish

SPIClass spi = SPIClass(VSPI);

// ---------------------------------------------------------------

void printCardType(uint8_t cardType) {
  Serial.print("Card type: ");
  switch (cardType) {
    case CARD_NONE:  Serial.println("NONE (not detected)"); break;
    case CARD_MMC:   Serial.println("MMC"); break;
    case CARD_SD:    Serial.println("SDSC"); break;
    case CARD_SDHC:  Serial.println("SDHC/SDXC"); break;
    default:         Serial.println("UNKNOWN"); break;
  }
}

// Runs the full detect -> write -> read-back -> verify sequence
// at a given clock speed. Returns true if everything passed.
bool runDiagnosticAtClock(uint32_t clockHz) {
  Serial.println();
  Serial.print("=== Testing at ");
  Serial.print(clockHz / 1000000.0, 1);
  Serial.println(" MHz ===");

  SD.end();  // make sure we're starting clean

  unsigned long beginStart = millis();
  bool began = SD.begin(SD_CS, spi, clockHz);
  unsigned long beginTime = millis() - beginStart;

  if (!began) {
    Serial.print("FAIL: SD.begin() failed after ");
    Serial.print(beginTime);
    Serial.println(" ms.");

    uint8_t cardType = SD.cardType();
    printCardType(cardType);

    if (cardType == CARD_NONE) {
      Serial.println("-> No card detected at all at this clock speed.");
      Serial.println("   Check: card seated fully, 3.3V + GND solid,");
      Serial.println("   CS/MOSI/MISO/SCK wires making real contact");
      Serial.println("   (wiggle test them while this is running).");
    }
    return false;
  }

  Serial.print("SD.begin() succeeded in ");
  Serial.print(beginTime);
  Serial.println(" ms.");

  uint8_t cardType = SD.cardType();
  printCardType(cardType);

  uint64_t cardSizeMB = SD.cardSize() / (1024 * 1024);
  Serial.print("Card size: ");
  Serial.print(cardSizeMB);
  Serial.println(" MB");

  // ---- Write + read-back verification ----
  const char* testPath = "/diag_test.txt";
  const char* testString = "avionics_diag_check_12345";

  if (SD.exists(testPath)) {
    SD.remove(testPath);
  }

  unsigned long writeStart = micros();
  File writeFile = SD.open(testPath, FILE_WRITE);
  bool writeOk = false;

  if (writeFile) {
    writeFile.print(testString);
    writeFile.close();
    writeOk = true;
  }
  unsigned long writeTimeUs = micros() - writeStart;

  if (!writeOk) {
    Serial.println("FAIL: File opened for write but write/close failed.");
    return false;
  }

  Serial.print("Write completed in ");
  Serial.print(writeTimeUs);
  Serial.println(" us.");

  if (writeTimeUs > 20000) {
    Serial.println("-> NOTE: write took over 20ms. At 50Hz IMU sampling");
    Serial.println("   (20ms per sample), a blocking write this long WILL");
    Serial.println("   cause dropped sensor samples once SD logging is");
    Serial.println("   merged into sensor_integration_test. Flag this now.");
  }

  // Read back and verify content actually matches
  File readFile = SD.open(testPath, FILE_READ);
  if (!readFile) {
    Serial.println("FAIL: Could not reopen file for read-back.");
    return false;
  }

  String readBack = readFile.readString();
  readFile.close();

  if (readBack != String(testString)) {
    Serial.println("FAIL: Read-back does not match what was written.");
    Serial.print("  Expected: "); Serial.println(testString);
    Serial.print("  Got:      "); Serial.println(readBack);
    return false;
  }

  Serial.println("PASS: Write + read-back verified correct.");
  SD.remove(testPath);

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("############################################");
  Serial.println("  SD Card Diagnostic");
  Serial.print("  CS pin: "); Serial.println(SD_CS);
  Serial.println("############################################");

  bool slowPassed = runDiagnosticAtClock(SPI_CLOCK_SLOW);
  bool fastPassed = runDiagnosticAtClock(SPI_CLOCK_FAST);

  Serial.println();
  Serial.println("=== SUMMARY ===");
  Serial.print("Slow clock (4 MHz):  ");
  Serial.println(slowPassed ? "PASS" : "FAIL");
  Serial.print("Fast clock (25 MHz): ");
  Serial.println(fastPassed ? "PASS" : "FAIL");
  Serial.println();

  if (!slowPassed && !fastPassed) {
    Serial.println("Both failed -> likely wiring, power, card format,");
    Serial.println("or CS pin issue. Not a clock-speed problem. Check");
    Serial.println("3.3V rail with a multimeter WHILE this test runs");
    Serial.println("(look for sag during SD.begin()), and confirm the");
    Serial.println("card is formatted FAT32.");
  } else if (slowPassed && !fastPassed) {
    Serial.println("Slow passed, fast failed -> classic signal integrity");
    Serial.println("problem. Your wiring (breadboard + jumper length) can't");
    Serial.println("reliably handle the faster clock. Either shorten wires /");
    Serial.println("move to soldered connections, or just run the logger at");
    Serial.println("the slower clock permanently.");
  } else if (slowPassed && fastPassed) {
    Serial.println("Both passed. SD hardware and wiring are solid at this");
    Serial.println("moment. If it's been failing intermittently, the next");
    Serial.println("suspect is a loose physical connection (breadboard SPI");
    Serial.println("is notoriously flaky) — try the wiggle test, and treat");
    Serial.println("this as unverified until it survives being bumped.");
  }
}

void loop() {
  // nothing — this is a one-shot diagnostic, check Serial Monitor
}
