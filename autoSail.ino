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


int windDirection;
int windSensorOutput;

int mainsheetTrim;
int rudderPosition;
float destHeading;
int targHeading;

void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);
  compass.init();
  rudder.attach(9);
  rudder.write(90);

  mainsheet.attach(11);
}

void loop() {
  while (ss.available() > 0) { // collect gps data first
    gps.encode(ss.read());
  }

  windSensorOutput = analogRead(windSensorPin);
  windDirection = map(windSensorOutput, 0,1006, 0, 359); // read wind direction next

  mainsheetTrim = calculateSailAngle(windDirection); // calculate trim angle for sail

  // destHeading = TinyGPS++::courseTo(gps.location.lat(), gps.location.lng(), targetLat, targetLon);

  // targHeading = calculateTargetHeading(windDirection, destHeading);

  // rudderPosition = calculateRudderAngle(targHeading, gps.course.deg());

  // rudder.write(rudderPosition);
  mainsheet.write(mainsheetTrim);

  delay(500);   
  }
    
      
}

int calculateTargetHeading(int windDirection, float headingToDestination) {
  int targetHeading;

  return targetHeading;
}

int calculateRudderAngle(int targetHeading, float heading) {
  int rudderAngle;
  return rudderAngle;
}

int calculateSailAngle(int windDirection) {
  int sailAngle;
  windDirection -= 180;
  windDirection = abs(windDirection);
  if (windDirection <= 40) {
    sailAngle = 0;
  }
  else {
    sailAngle = map(windDirection, 41, 180, 0, 179);
  }
  return sailAngle;
}