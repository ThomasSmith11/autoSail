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
int currentHeading;
int targHeading;
float velocity;
float distanceOffLine;

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

    distanceOffLine = calculateDistanceOffLine(destLat, destLon, prevLat, prevLon, currentLat, currentLon, destHeading);
    targHeading = calculateTargetHeading(destHeading, currentHeading, windDirection, distanceOffLine);
    rudderPosition = calculateRudderPosition(currentHeading, targHeading);
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
    currentHeading = int(gps.course.deg()+.5);
    // velocity = gps.speed.knots();
  }
}


float calculateDistanceOffLine(float destLat, float destLon, float prevLat, float prevLon, float lat, float lon, float destHeading) {
  float headingFromPrevToCurrent = TinyGPSPlus::courseTo(prevLat, prevLon, lat, lon);
  float distanceFromPrevToCurrent = TinyGPSPlus::distanceBetween(prevLat, prevLon, lat, lon);
  float theta = radians(headingFromPrevToCurrent - destHeading);
  return distanceFromPrevToCurrent * sin(theta);
}


int calculateRudderPosition(int currentHeading, int targHeading) {
  int rudderScale;
  
  int clockwiseDiff = (targHeading - currentHeading + 360) % 360;
  int counterclockwiseDiff = (currentHeading - targHeading + 360) % 360;

  if (clockwiseDiff < counterclockwiseDiff) {
      rudderScale = calcRudderScale(abs(targHeading-currentHeading));
  } else {
      rudderScale = calcRudderScale(abs(targHeading-currentHeading))*-1;
  }

  //change rudder scale in 5 degree increments, such that above ~40 degrees off course we get maximum steering,
  //then smoother steering as we approach the correct course.  Max rudder scale should be 6, min -6

  return rudderScale*50+1450;
}


int calcRudderScale(int headingDiff) {

  if (headingDiff >= 42) {
    return 6;
  }
  else {
    return headingDiff/7;
  }
}


int calculateTargetHeading(float destHeading, int currentHeading, int windDirection, float distanceAway) {
  int headingToWind = windDirection - 180;
  if (abs(headingToWind) > 40) {
    return int(destHeading+.5); //round destHeading to nearest whole number
  }
  else {
    int actualWindDirection = (windDirection+currentHeading)%360;
    if (distanceAway < 3) {
      int clockwiseDiff = (actualWindDirection - currentHeading + 360) % 360;
      int counterclockwiseDiff = (actualWindDirection - currentHeading + 360) % 360;
      bool starboard = (clockwiseDiff>counterclockwiseDiff);
      if (starboard) {
        return (actualWindDirection + 320) % 360;
      }
      else {
        return (actualWindDirection + 40) % 360;
      }
    }
    else {
      //figure out which side of the "line" you're on, and head back towards it
    }
  }
}
