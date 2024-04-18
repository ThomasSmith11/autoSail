#include <Servo.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

struct GPSCoordinate {
  float latitude;
  float longitude;
};

static const int rudderPin = 12;
static const int mainsheetPin = 13;
static const int windSensorPin = A5;
static const int TXPin = 4, RXPin = 5;
static const uint32_t GPSBaud = 9600;


Servo rudder;
Servo mainsheet;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);


int prevIndex, destIndex;
GPSCoordinate prevCoords, currCoords, destCoords;


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

    prevCoords = coordinates[prevIndex];
    destCoords = coordinates[prevIndex];

    float destHeading = TinyGPSPlus::courseTo(currCoords.latitude, currCoords.longitude, prevCoords.latitude, prevCoords.longitude);

    distanceOffLine = calculateDistanceOffLine(destHeading, currCoords, prevCoords);
    bool toRightOfLine = determineSideofLine(destCoords, currCoords, prevCoords);
    targHeading = calculateTargetHeading(destHeading, currentHeading, windDirection, distanceOffLine, toRightOfLine);
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
    currCoords.latitude = gps.location.lat();
    currCoords.longitude = gps.location.lng();
    currentHeading = int(gps.course.deg()+.5);
    // velocity = gps.speed.knots();
  }
}


float calculateDistanceOffLine(float destHeading, GPSCoordinate current, GPSCoordinate previous) {
  float currLat = current.latitude;
  float currLon = current.longitude;
  float prevLat = previous.latitude;
  float prevLon = previous.longitude;

  float headingFromPrevToCurrent = TinyGPSPlus::courseTo(prevLat, prevLon, currLat, currLon);
  float distanceFromPrevToCurrent = TinyGPSPlus::distanceBetween(prevLat, prevLon, currLat, currLon);
  float theta = radians(headingFromPrevToCurrent - destHeading);
  return distanceFromPrevToCurrent * sin(theta);
}


bool determineSideofLine(GPSCoordinate destination, GPSCoordinate current, GPSCoordinate previous) {
  float Ax = destination.latitude - previous.latitude;
  float Ay = destination.longitude - previous.longitude;
  float Bx = current.latitude - previous.latitude;
  float By = current.longitude - previous.longitude;

  float crossProduct = Ax * Ay - Bx * By;

  if (crossProduct > 0) {
    return 0;
  } else {
    return 1;
  }
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

  return rudderScale*50+1450;
}


int calcRudderScale(int headingDiff) {

// change rudder scale in 5 degree increments, such that above ~40 degrees off course we get maximum steering,
// then smoother steering as we approach the correct course.  Max rudder scale should be 6, min -6

  if (headingDiff >= 42) {
    return 6;
  }
  else {
    return headingDiff/7;
  }
}


int calculateTargetHeading(float destHeading, int currentHeading, int windDirection, float distanceAway, bool toRightOfLine) {
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
      if (toRightOfLine) {
        return (actualWindDirection + 320) % 360;
      }
      else {
        return (actualWindDirection + 40) % 360;
      }
    }
  }
}
