// MotoBit
// track GPS coords, push to cloud
//// keep a buffer to keep data costs down
// track lean angle with accelerometer
//// when GPS speed is 0, use that to orientate 0 degrees
//// when bike is off, use this as lojack feature to send SMS
// SMS feature
//// as lojack as mentioned above
//// can send LOST command and replies with coordinates
//// also crash detection feature potentially

#include <Adafruit_GPS.h>

#define GPS_SEND_TIME 10000
#define LED_DATA_INDICATOR_DURATION 200
#define LED_DATA_INDICATOR_FLICKER_OFF_DURATION 50
#define LED_DATA_INDICATOR_FLICKER_ON_DURATION 5
#define BUFFER_LENGTH 7
//#define LOST_SMS_TRIGGER "LOST"

// set to true for raw GPS debugging to Serial console
#define GPS_DEBUG false

#define gpsSerial Serial0

Adafruit_GPS GPS(&gpsSerial);

unsigned ledStartMillis;
unsigned ledNextFlickerMillis;
bool uplinkDataDetected;
bool downlinkDataDetected;
bool ledOn = false;
bool cloudReady = false;
bool foundSMS = false;
uint32_t gpsOutputTimer;
uint32_t gpsReadTimer;
String smsPayload = "";
String tempBuffer = "";

void setup() {
    Dash.begin();
    SerialUSB.begin(9600);
    Serial2.begin(9600);
    SerialCloud.begin(115200);

    GPS.begin(9600);
    gpsSerial.begin(9600);
    //turn on RMC (recommended minimum) and GGA (fix data) including altitude
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    // Set the update rate
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
    // Request updates on antenna status, comment out to keep quiet
    GPS.sendCommand(PGCMD_ANTENNA);
    delay(1000);
    // Ask for firmware version
    gpsSerial.println(PMTK_Q_RELEASE);

    SerialUSB.println("MotoBit Tracker");
    SerialUSB.print("Boot Version: ");
    SerialUSB.print(Dash.bootVersion());
    SerialUSB.print("  GPS Firmware: ");
    SerialUSB.print(PMTK_Q_RELEASE);
}

void loop() {
    char currChar;

    while (SerialCloud.available()) {
        currChar = (char) SerialCloud.read();

        if (!foundSMS && tempBuffer == "SMSRCVD") {
            foundSMS = true;
        }

        if (!cloudReady && tempBuffer.substring(3) == "+++") {
            cloudReady = true;
        }

        if (foundSMS && currChar == '\n') {
            // handleSMS();
        } else {
            smsPayload.concat(currChar);
        }

        if (tempBuffer.length() >= BUFFER_LENGTH) {
            tempBuffer.remove(0, 1);
        }

        tempBuffer.concat(currChar);
        SerialUSB.write(currChar);
    }

    GPSloop();
    gpsReadTimer = millis();

    //// if millis() or timer wraps around, we'll just reset it
   if (gpsOutputTimer > millis()) {
      gpsOutputTimer = millis();
   }
}

void GPSloop() {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPS_DEBUG && c) {
        SerialUSB.print(c);
    }
    // if a sentence is received, we can check the checksum, parse it...
    if (GPS.newNMEAreceived()) {
        // a tricky thing here is if we print the NMEA sentence, or data
        // we end up not listening and catching other sentences!
        // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
        //SerialUSB.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

        if (!GPS.parse(GPS.lastNMEA())) {  // this also sets the newNMEAreceived() flag to false
            return;  // we can fail to parse a sentence in which case we should just wait for another
        }
    }

    // print out the current stats and send to cloud
    if (millis() - gpsOutputTimer > GPS_SEND_TIME) {
        gpsOutputTimer = millis(); // reset the timer

        SerialUSB.print("\nTime: ");
        SerialUSB.print(GPS.hour, DEC); SerialUSB.print(':');
        SerialUSB.print(GPS.minute, DEC); SerialUSB.print(':');
        SerialUSB.print(GPS.seconds, DEC); SerialUSB.print('.');
        SerialUSB.println(GPS.milliseconds);
        SerialUSB.print("Date: ");
        SerialUSB.print(GPS.day, DEC); SerialUSB.print('/');
        SerialUSB.print(GPS.month, DEC); SerialUSB.print("/20");
        SerialUSB.println(GPS.year, DEC);
        SerialUSB.print("Fix: "); SerialUSB.print((int)GPS.fix);
        SerialUSB.print(" quality: "); SerialUSB.println((int)GPS.fixquality);
        if (GPS.fix) {
            SerialUSB.print("Location: ");
            SerialUSB.print(GPS.latitude, 4); SerialUSB.print(GPS.lat);
            SerialUSB.print(", ");
            SerialUSB.print(GPS.longitude, 4); SerialUSB.println(GPS.lon);

            SerialUSB.print("Speed (knots): "); SerialUSB.println(GPS.speed);
            SerialUSB.print("Angle: "); SerialUSB.println(GPS.angle);
            SerialUSB.print("Altitude: "); SerialUSB.println(GPS.altitude);
            SerialUSB.print("Satellites: "); SerialUSB.println((int)GPS.satellites);

            // Send latitude and longitude to cloud
            String loc;
            char coordbuf[16];
            dtostrf(GPS.latitudeDegrees, 0, 4, coordbuf);
            loc.concat(coordbuf);
            loc.concat(", ");
            dtostrf(GPS.longitudeDegrees, 0, 4, coordbuf);
            loc.concat(coordbuf);
            String ret = "{\"coords\": [";
            ret.concat(loc);
            ret.concat("]}");
            SerialCloud.println(ret);
        }
    }
}

void handleSMS() {
    smsPayload = stripOffLengthNumber(smsPayload);
    smsPayload.toUpperCase();

    if (smsPayload == "LOST") {
        SerialUSB.println("LOST mode triggered!");
        //respond with SMS to number with GPS coordinates here
    }
}

String stripOffLengthNumber(String payload) {
    int index = 0;
    while(payload[index] == ',') {
        ++index;
    }
    while(payload[index] != ',') {
        ++index;
    }
    ++index;

    payload.remove(0, index);
    payload.trim();
    return payload;
}
