#include <Adafruit_GPS.h>

#define GPS_SEND_TIME 5000     //read gps every 5 seconds
#define LED_DATA_INDICATOR_DURATION 200
#define LED_DATA_INDICATOR_FLICKER_OFF_DURATION 50
#define LED_DATA_INDICATOR_FLICKER_ON_DURATION 5
#define BUFFER_LENGTH 25
const String LOST_SMS_TRIGGER="LOST";
#define GPS_DEBUG false     // set to true for raw GPS debugging to Serial console
#define gpsSerial Serial0   // serial0 uses pins 3 and 5

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
String payloadArray[BUFFER_LENGTH];

void setup() {
    Dash.begin();
    SerialCloud.begin(115200);
    SerialUSB.begin(9600);
    gpsSerial.begin(9600);
    GPS.begin(9600);

    //turn on RMC (recommended minimum) and GGA (fix data) including altitude
    GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
    // Set the update rate
    GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
    // Request updates on antenna status, comment out to keep quiet
    GPS.sendCommand(PGCMD_ANTENNA);

    delay(3000);
    SerialUSB.println("MotoBit Tracker");
    SerialUSB.print("Boot Version: ");
    SerialUSB.print(Dash.bootVersion());
    SerialUSB.print("  GPS Firmware: ");
    SerialUSB.println(PMTK_Q_RELEASE);
}

void loop() {
    char currChar;

    while (SerialCloud.available()) {
        currChar = (char) SerialCloud.read();

        if (!cloudReady && tempBuffer.startsWith("+++")) {
            SerialUSB.println("cloudReady");
            cloudReady = true;
        }

        if (!foundSMS && tempBuffer == "SMSRCVD") {
            SerialUSB.println("SMSRCVD");
            foundSMS = true;
        }

        if (foundSMS && currChar == '\n') {
            SerialUSB.println("handleSMS");
            handleSMS();
        } else {
            smsPayload.concat(currChar);
        }

        if (tempBuffer.length() >= 7) {
            tempBuffer.remove(0, 1);
        }

        tempBuffer.concat(currChar);
        SerialUSB.write(currChar);
    }

    GPSloop();
    gpsReadTimer = millis();

    // if millis() or timer wraps around, we'll just reset it
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

    SerialUSB.print("payloadArray length: ");
    SerialUSB.println(payloadArray.count());
    // if (payloadQueue.count() >= BUFFER_LENGTH) {
    //     sendDataToCloud();
    // }

    // print out the current stats and send to cloud
    if (millis() - gpsOutputTimer > GPS_SEND_TIME) {
        gpsOutputTimer = millis(); // reset the timer

        if (GPS.fix) {
            String payloadString = buildGPSPayload();
            // payloadQueue.push(payloadString);
            SerialUSB.print("Location String: ");
            SerialUSB.println(payloadString);

            SerialUSB.print("Quality: ");
            SerialUSB.print((int) GPS.fixquality);
            SerialUSB.print(" Satellites: ");
            SerialUSB.print((int) GPS.satellites);
        } else {
            String payloadString = buildGPSPayload();
            // payloadQueue.push(payloadString);
            SerialUSB.print("No GPS fix. Quality: ");
            SerialUSB.print((int) GPS.fixquality);
            SerialUSB.print("  Satellites: ");
            SerialUSB.print((int) GPS.satellites);
            SerialUSB.print("  Time: ");
            SerialUSB.print(GPS.hour, DEC); SerialUSB.print(':');
            SerialUSB.print(GPS.minute, DEC); SerialUSB.print(':');
            SerialUSB.print(GPS.seconds, DEC); SerialUSB.print('.');
            SerialUSB.print(GPS.milliseconds);
            SerialUSB.print("  Date: ");
            SerialUSB.print(GPS.day, DEC); SerialUSB.print('/');
            SerialUSB.print(GPS.month, DEC); SerialUSB.print("/20");
            SerialUSB.print(GPS.year, DEC);
            SerialUSB.println("");
        }
        // SerialUSB.print("payloadQueue length: ");
        // SerialUSB.println(payloadQueue.count());
        // SerialUSB.print("payloadQueue is full: ");
        // SerialUSB.println(payloadQueue.isFull());
    }
}

void sendDataToCloud() {
    SerialUSB.println("sendDataToCloud()");
    // while (!payloadQueue.isEmpty()) {
    //     String payload;
    //     for (int i = 0; i < BUFFER_LENGTH; i++) {
    //         payload.concat(payloadQueue.pop());
    //         payload.concat("#");
    //     }
    //     SerialCloud.println(payload);
    //     SerialUSB.print("Sent payload: ");
    //     SerialUSB.println(payload);
    // }

}

String buildGPSPayload() {
    // LAT|LON|speed|altitude|time
    String payloadString;
    char decimalBuffer[16];

    // latitude
    dtostrf(GPS.latitudeDegrees, 0, 5, decimalBuffer);
    payloadString.concat(decimalBuffer);
    payloadString.concat("|");
    // longitude
    dtostrf(GPS.longitudeDegrees, 0, 5, decimalBuffer);
    payloadString.concat(decimalBuffer);
    payloadString.concat("|");
    // speed
    dtostrf(GPS.speed, 0, 2, decimalBuffer);
    payloadString.concat(decimalBuffer);
    payloadString.concat("|");
    // altitude
    dtostrf(GPS.altitude, 0, 4, decimalBuffer);
    payloadString.concat(decimalBuffer);
    payloadString.concat("|");
    // time
    payloadString.concat(GPS.hour);
    payloadString.concat(":");
    payloadString.concat(GPS.minute);
    payloadString.concat(":");
    payloadString.concat(GPS.seconds);
    payloadString.concat(".");
    payloadString.concat(GPS.milliseconds);

    return payloadString;
}

void handleSMS() {
    smsPayload = stripOffLengthNumber(smsPayload);
    smsPayload.toUpperCase();

    if (smsPayload == LOST_SMS_TRIGGER) {
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
