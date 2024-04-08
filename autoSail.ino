#include <Servo.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Wire.h> //maybe not needed if compass doesn't work
#include <HMC5883L_Simple.h> //maybe not needed if compass doesn't work

static const int windSensorPin = A5;
static const int TXPin = 4, RXPin = 3;
static const uint32_t GPSBaud = 9600;

Servo rudder;
Servo mainsheet;
HMC5883L_Simple Compass; //maybe not needed if compass doesn't work
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);


int windDirection;
int windSensorOutput;

void setup() {
  Serial.begin(115200);
  Wire.begin(); //maybe not needed if compass doesn't work
  ss.begin(GPSBaud);

  rudder.attach(9);
  rudder.write(90);

  mainsheet.attach(11);
}

void loop() {
  windSensorOutput = analogRead(windSensorPin);
  windDirection = map(windSensorOutput, 0,1006, 0, 359);

}

float calulateHeadingToDestination(float latitude, float longitude, float destLatitude, float destLongitude) {
  int headingToDestination;

  return headingToDestination;
}

int calculateTargetHeading(int windDirection, float heading, float headingToDestination) {

}