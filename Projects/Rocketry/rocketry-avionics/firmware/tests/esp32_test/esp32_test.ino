#define POT_PIN 32

void setup() {
  Serial.begin(115200);
}

void loop() {
  int value = analogRead(POT_PIN);
  Serial.println(value);
  delay(100);
}