#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"
#include <EEPROM.h>
#include "Stepper.h"

fauxmoESP fauxmo; // Declare fauxmoESP object
const char* deviceZero = "D1";
#define WIFI_SSID "SSID Here" // Wifi SSID
#define WIFI_PASS "Password Here"   // Wifi Password

int data;              // EEPROM data
int edata;             // EEPROM value
int state;             // EEPROM value
int alexaValue;
int alexaState = 2;
int countflag = 0;
int stopflag = 0;
int blindDir = 0;       // Blind direction, set to 0 for reverse if needed
int blindSpeed = 1;
int blindLength = 4200; // Number of steps for full blind length
int blindPosition = 0;  // Initial blind position
int blindPositionB = 0;
boolean resetB = false;
Stepper myStepper = Stepper(blindLength, 2, 4, 0, 5);

void setup() {
  myStepper.setSpeed(5);
  Serial.begin(9600);
  EEPROM.begin(512);
  edata = EEPROM.get(0, data);
  state = EEPROM.get(1, data);

  Serial.println("Setting up WiFi!");
  wifiSetup(); // Set up WiFi

  // Setup fauxmo
  fauxmo.createServer(true);
  fauxmo.setPort(80);
  fauxmo.enable(true);

  // Add virtual devices
  fauxmo.addDevice(deviceZero); // Add deviceZero
  fauxmo.onSetState(callback);

  if (state == 1) {
    // If the last state was on, drive the motor to the off position
    moveMotor(0, blindLength);
    blindPosition = 0;
    EEPROM.put(0, blindPosition);
    EEPROM.put(1, 0); // Set the state to 0 (off)
    EEPROM.commit();
    delay(1000); // Delay to allow the motor to reach the off position
  }

  stepperOff();
}

void loop() {
  if (resetB == false) {
    resetB = true;
    if (edata > 0 && state == 1) {
      moveMotor(0, blindPosition); // Drive motor to off position on reboot
      blindPosition = 0;
      EEPROM.put(0, blindPosition);
      EEPROM.put(1, 0); // Set the state to 0 (off)
      EEPROM.commit();
    }
  }
  fauxmo.handle();
  delay(100);
  if (alexaValue > 0) {
    alexaAction();
  }
  stopflag = 0;
  alexaState = 2;
  delay(100);
  blindPositionB = blindPosition;
}

void callback(unsigned char device_id, const char* device_name, bool state, unsigned char value) {
  Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
  Serial.print(state);
  alexaValue = value;
  alexaState = state;
}

void alexaAction() {
  alexaValue = blindLength; //alexaValue * (blindLength/255); // Divide the numbers of total steps with 255 to get percentage number
  if ((blindPosition > alexaValue) && (stopflag == 0) && (alexaState == 1)) {
    countflag = blindPosition - alexaValue;
    moveMotor(0, countflag);
    blindPosition = alexaValue;
    stopflag = 1;
  }

  if ((blindPosition < alexaValue) && (stopflag == 0) && (alexaState == 1)) {
    countflag = alexaValue - blindPosition;
    moveMotor(1, countflag);
    blindPosition = alexaValue;
    EEPROM.put(0, blindPosition);
    EEPROM.put(1, 1); // Set the state to 1 (on)
    EEPROM.commit();
    stopflag = 1;
  }
  if ((blindPosition == alexaValue) && (stopflag == 0) && (alexaState == 1)) {
    stopflag = 1;
  }
  if (alexaState == 0) {
    moveMotor(0, blindPositionB);
    blindPosition = 0;
    EEPROM.put(0, blindPosition);
    EEPROM.put(1, 0); // Set the state to 0 (off)
    EEPROM.commit();
    alexaState = 2;
  }
  alexaValue = 0;
}

void moveMotor(int moveDir, int noSteps) {
  if (moveDir == blindDir) {
    for (int i = 0; i <= noSteps; i++) {
      myStepper.step(1);
      delay(blindSpeed);
    }
  } else {
    for (int i = 0; i <= noSteps; i++) {
      myStepper.step(-1);
      delay(blindSpeed);
    }
  }
  stepperOff();
}

void stepperOff() {
  digitalWrite(2, LOW);
  digitalWrite(4, LOW);
  digitalWrite(0, LOW);
  digitalWrite(5, LOW);
}

// Wifi Setup
void wifiSetup() {
  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(LIGHT_SLEEP_T);

  // Connect
  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Connected!
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  Serial.printf("Device Name: %s ", deviceZero);
  Serial.println(".........................................");
}

