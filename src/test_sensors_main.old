#include <Arduino.h>

#define MOISTURE_PIN A6

int dryValue = 0;
int wetValue = 1023;
bool calibrated = false;

void printInstructions() {
  Serial.println("\n--- Capacitive Moisture Sensor Calibration ---");
  Serial.println("1. Insert sensor into dry soil, then send 'd' over Serial.");
  Serial.println("2. Insert sensor into wet soil, then send 'w' over Serial.");
  Serial.println("3. Sensor readings will be printed every second.");
  Serial.println("---------------------------------------------\n");
}

void setup() {
  Serial.begin(9600);
  while (!Serial) ; // Wait for Serial to be ready
  printInstructions();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'd') {
      dryValue = analogRead(MOISTURE_PIN);
      Serial.print("Dry value set to: ");
      Serial.println(dryValue);
    } else if (cmd == 'w') {
      wetValue = analogRead(MOISTURE_PIN);
      Serial.print("Wet value set to: ");
      Serial.println(wetValue);
      calibrated = true;
    }
  }

  int sensorValue = analogRead(MOISTURE_PIN);
  Serial.print("Raw value: ");
  Serial.print(sensorValue);
  if (calibrated && (wetValue != dryValue)) {
    int percent = map(sensorValue, dryValue, wetValue, 0, 100);
    percent = constrain(percent, 0, 100);
    Serial.print(" | Moisture: ");
    Serial.print(percent);
    Serial.print("%");
  }
  Serial.println();
  delay(1000);
}