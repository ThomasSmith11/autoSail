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

double prevLat, prevLon;
double currentLat, currentLon;
double destLat, destLon;

int windDirection, windSensorOutput;

int mainsheetTrim;
int rudderPosition;
double destHeading;
double currentHeading;
int targHeading;
double velocity;

double waypointList[10][2] = {
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
  rudderPosition = calculateRudderPosition(prevLat, prevLon, currentLat, currentLon, destLat, destLon, currentHeading, destHeading, windDirection);
  rudder.writeMicroseconds(rudderPosition);
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


double calculateDistanceOffLine(double destLat, double destLon, double prevLat, double prevLon, double lat, double lon, double destHeading) {
  double headingFromPrevToCurrent = TinyGPSPlus::courseTo(prevLat, prevLon, lat, lon);
  double distanceFromPrevToCurrent = TinyGPSPlus::distanceBetween(prevLat, prevLon, lat, lon);
  double theta = radians(headingFromPrevToCurrent - destHeading);
  return distanceFromPrevToCurrent * sin(theta);
}


int calculateRudderPosition(double prevLat, double prevLon, double currentLat, double currentLon, double destLat, double destLon, double currentHeading, double destHeading, int windDirection) {
  int rudderScale;
  targHeading = calculateTargetHeading(destHeading, windDirection);
  if (targHeading == -1) {
    double distanceAway = calculateDistanceOffLine(destLat, destLon, prevLat, prevLon, currentLat, currentLon, destHeading);
    if (distanceAway < 3) {
      //targetHeading = 40 degrees off wind in whatever direction already facing
    }
    else {
      //initiate tack
      //targetHeading = 40 degrees off wind in whatever direction not facing
    }
  }
  
  //change rudder scale in 5 degree increments, such that above 30 degrees off course we get maximum steering,
  //then smoother steering as we approach the correct course.  Max rudder scale should be 6, min -6

  return rudderScale*50+1450;
}


int calculateTargetHeading(double destHeading, int windDirection) {
  if (abs(windDirection-180) > 40) {
    return int(destHeading+.5);
  }
  else {
    return -1;
  }
}
