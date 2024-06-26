#include <Servo.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

struct GPSCoordinate {
  float latitude;
  float longitude;
};

static const int controlPin = 2;
static const int gpsControlPin = 4;
static const int rudderOutputPin = 9;
static const int mainsheetOutputPin = 11;
static const int rudderPin = 12;
static const int mainsheetPin = 13;
static const int windSensorPin = A5;
static const uint32_t GPSBaud = 9600;
unsigned long time = 1400;

unsigned long waveStart;
unsigned long waveEnd;



Servo rudder;
Servo mainsheet;
TinyGPSPlus gps;


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


volatile bool manualControl = 1;
bool temp;
//May need to be switched


void setup() {
  Serial.begin(115200);
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
  attachInterrupt(digitalPinToInterrupt(controlPin), handleInterrupt, CHANGE);
  //audible ten second countdown before turning on gps and beginning loop
  for (int i = 0; i<10; i++) {
    blockingTone(7, 2000, 500);
    blockingTone(7, 0, 500);
  }
  Serial.begin(GPSBaud);
  pinMode(gpsControlPin, OUTPUT);
  digitalWrite(gpsControlPin, HIGH); // turn gps on
  pinMode(controlPin, INPUT);
  pinMode(rudderPin, INPUT);
  pinMode(mainsheetPin, INPUT);
  rudder.attach(rudderOutputPin);
  mainsheet.attach(mainsheetOutputPin);
}


void loop() {
  if (manualControl) {
    mainsheetValue = pulseIn(mainsheetPin, HIGH);
    rudderValue = pulseIn(rudderPin, HIGH);
    mainsheet.writeMicroseconds(mainsheetValue);
    rudder.writeMicroseconds(rudderValue);
  }
  else {
    finishedWaypoints:
    if (destIndex >= numCoords) {
      rudder.writeMicroseconds(1750);
    }
    while (Serial.available() > 0)
      if (gps.encode(Serial.read()))
        processGPS();

    if (prevIndex == NULL) {
      prevIndex = -1;
      destIndex = 0;
      prevCoords = currCoords;
      destCoords = coordinates[destIndex];
    }

    if (TinyGPSPlus::distanceBetween(currCoords.latitude, currCoords.longitude, destCoords.latitude, destCoords.longitude) < 3) {
      prevIndex++;
      destIndex++;
      if (destIndex >= numCoords) {
        goto finishedWaypoints;
      }
      destCoords = coordinates[destIndex];
      prevCoords = coordinates[prevIndex];
    }

    windSensorValue = analogRead(windSensorPin);
    Serial.println(windSensorValue);
    windDirection = map(windSensorValue, 0,1006, 0, 359);
    Serial.println(windDirection);
    mainsheetTrim = calculateSailAngle(windDirection);
    mainsheet.writeMicroseconds(mainsheetTrim);

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

void handleInterrupt(){
  int value = digitalRead(controlPin);
  if (value == 1) {
    waveStart = micros();
  }
  else {
    waveEnd = micros();
    time = waveEnd - waveStart;
  }
  if(time<1250){
    manualControl = 1;
  }
  if(time>1800){
    manualControl = 0;
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

  float crossProduct = Ax * By - Bx * Ay;

  if (crossProduct > 0) {
    return 1;
  } else {
    return 0;
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

  return rudderScale+1515;
}


int calcRudderScale(int headingDiff) {

// change rudder scale in 8 degree increments, such that above ~40 degrees off course we get maximum steering,
// then smoother steering as we approach the correct course.  Max rudder scale should be 6, min -6

  if (headingDiff >= 60) {
    return 400;
  }
  else {
    return map(headingDiff, 0, 59, 0, 400);
  }
}


int calculateTargetHeading(float destHeading, int currentHeading, int windDirection, float distanceAway, bool toRightOfLine) {
  int actualWindDirection = (windDirection+currentHeading)%360;
  if (abs(actualWindDirection-destHeading) > 40) {
    return int(destHeading+.5); //round destHeading to nearest whole number
  }
  else {
    if (distanceAway < 3) {
      int clockwiseDiff = (actualWindDirection - currentHeading + 360) % 360;
      int counterclockwiseDiff = (currentHeading - actualWindDirection + 360) % 360;
      bool starboard = (clockwiseDiff<counterclockwiseDiff);
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


void blockingTone(uint8_t pin, unsigned int frequency, unsigned long duration) {
    pinMode(pin, OUTPUT);
    unsigned long startTime = micros();

    while (micros() - startTime < duration * 1000) {
        unsigned long currentTime = micros() - startTime;
        if (currentTime % (1000000 / frequency) < (1000000 / (2 * frequency))) {
            digitalWrite(pin, HIGH);
        } else {
            digitalWrite(pin, LOW);
        }
    }

    digitalWrite(pin, LOW);
}
