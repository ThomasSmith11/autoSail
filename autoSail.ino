#include <Servo.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>


static const int rudderPin = 12;
static const int mainsheetPin = 13;
static const int windSensorPin = A5;
static const int TXPin = 4, RXPin = 5;
static const uint32_t GPSBaud = 9600;


Servo rudder;
Servo mainsheet;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);


float prevLat, prevLon;
float currentLat, currentLon;
float destLat, destLon;


// Sensor inputs
int windSensorValue, mainsheetValue, rudderValue;


int windDirection;
int mainsheetTrim;
int rudderPosition;
float destHeading;
float currentHeading;
int targHeading;
float velocity;

struct GPSCoordinate {
  float latitude;
  float longitude;
};

char inData[26];
int numCoords = 0;
GPSCoordinate coordinates[32];


volatile bool manualControl;


void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud);
  mainsheet.attach(11);
  Serial.println("Ready\n");
  while (true) {
    int index = 0;
    while (true) {
      if (Serial.available() > 0) {
        char c = Serial.read();
        if (c == '\n') {
          inData[index++] = '\0';
          break;
        } else {
          inData[index++] = c;
        }
      }
    }
    
    if (strcmp(inData, "␄") == 0) {
      break;
    }
    
    coordinates[numCoords++] = parseCoordinateString(inData);
  }
}


void loop() {
  if (manualControl) {
    mainsheetValue = pulseIn(mainsheetPin, HIGH);
    mainsheetTrim = map(mainsheetValue, 1150, 1800, 50, 130);
    mainsheet.write(mainsheetTrim);
    rudderValue = pulseIn(rudderPin, HIGH);
    rudderPosition = map(rudderValue, 1150, 1800, 45, 135);
    rudder.write(rudderPosition);
  }
  else {
    while (ss.available() > 0)
      if (gps.encode(ss.read()))
        processGPS();
    windSensorValue = analogRead(windSensorPin);
    windDirection = map(windSensorValue, 0,1006, 0, 359);
    mainsheetTrim = calculateSailAngle(windDirection);
    mainsheet.writeMicroseconds(mainsheetTrim);
    rudderPosition = calculateRudderPosition(prevLat, prevLon, currentLat, currentLon, destLat, destLon, currentHeading, destHeading, windDirection);
    rudder.writeMicroseconds(rudderPosition);
  }
}


GPSCoordinate parseCoordinateString(char* gpsString) {
  GPSCoordinate coord;
  char *lat_str = strtok(gpsString, ",");
  char *lon_str = strtok(NULL, ",");
  if (lat_str != NULL && lon_str != NULL) {
    float lat = atof(lat_str);
    float lon = atof(lon_str);
    coord = {lat, lon};
  }
  return coord;
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


int calculateRudderPosition(float prevLat, float prevLon, float currentLat, float currentLon, float destLat, float destLon, float currentHeading, float destHeading, int windDirection) {
  int rudderScale;
  targHeading = calculateTargetHeading(destHeading, windDirection);
  if (targHeading == -1) {
    float distanceAway = calculateDistanceOffLine(destLat, destLon, prevLat, prevLon, currentLat, currentLon, destHeading);
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


int calculateTargetHeading(float destHeading, int windDirection) {
  if (abs(windDirection-180) > 40) {
    return int(destHeading+.5);
  }
  else {
    return -1;
  }
}
