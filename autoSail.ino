#include <math.h>
#include <Servo.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

using namespace std;

static const int windSensorPin = A5;
static const int TXPin = 4, RXPin = 5;
static const uint32_t GPSBaud = 9600;

Servo rudder;
Servo mainsheet;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);

float currentLat, currentLon;
float destLat, destLon;

int windDirection, windSensorOutput;

int mainsheetTrim;
int rudderPosition;
float destHeading;
float currentHeading;
int targHeading;
float velocity;

float waypointList[10][2] = {
  { 0.0, 0.0},
  { 0.0, 0.0},
  { 0.0, 0.0},
  { 0.0, 0.0},
  { 0.0, 0.0}
};

bool manualControl;

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);
  mainsheet.attach(11);
}

void loop() {
  while (ss.available() > 0)
    if (gps.encode(ss.read()))
      processGPS();
  windSensorOutput = analogRead(windSensorPin);
  windDirection = map(windSensorOutput, 0,1006, 0, 359);
  mainsheetTrim = calculateSailAngle(windDirection);
  mainsheet.writeMicroseconds(mainsheetTrim);
}


int calculateSailAngle(int windDirection) {
  int sailAngle;
  windDirection -= 180;
  windDirection = abs(windDirection);
  if (windDirection < 45) {
    sailAngle = 1750;
  }
  else {
    sailAngle = map(windDirection, 45, 180, 1750, 1150);
  }
  return sailAngle;
}

void processGPS() {
  if (gps.location.isValid())
  {
    currentLat = gps.location.lat();
    currentLon = gps.location.lng();
    currentHeading = gps.course.deg();
    // velocity = gps.speed.knots();
  }
}

float calculateDistanceOffLine(float destLat, float destLon, float prevLat, float prevLon, float lat, float lon, float destHeading) {
  float headingFromPrevToCurrent = TinyGPSPlus::courseTo(prevLat, prevLon, lat, lon);
  float distanceFromPrevToCurrent = TinyGPSPlus::distanceBetween(prevLat, prevLon, lat, lon);
  float theta = radians(headingFromPrevToCurrent - destHeading);
  return distanceFromPrevToCurrent * sin(theta);
}
