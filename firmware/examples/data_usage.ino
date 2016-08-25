int plusSigns = 0;
bool cloudReady = false;
int bytesToSend = 800;
int loopCount = 0;
bool sent = false;

void setup()
{
   SerialUSB.begin(9600);
   Serial2.begin(9600);
   SerialCloud.begin(115200);
   
   Dash.begin();
   Dash.pulseLED(100,3000);

   SerialUSB.println("Hologram Data Usage Test Started!");
   SerialUSB.print("Using Boot Version: ");
   SerialUSB.println(Dash.bootVersion());
}

String generateBytes(int bytes)
{
    String buf = "";
    for (int i = 0; i < bytes; i++) {
        buf = buf + "a";
    }
    SerialUSB.println("Generated buffer of " + String(buf.length()) + " bytes: " + buf);
    return buf;
}

void sendBytesToCloud(int bytes)
{
    String payload = generateBytes(bytes);
    SerialCloud.println(payload);
}

void loop()
{
    char currChar;

    if (cloudReady) {
      loopCount++;

      if (bytesToSend > 0) {
        if (!sent) {
          sendBytesToCloud(bytesToSend);
          sent = true;
          Dash.pulseLED(50,300);
          delay(1000);
          Dash.pulseLED(100,3000);
        }
      } else {
        delay(60000); //sleep for one minute
        SerialUSB.println(loopCount + ") slept for 1 minute");
      }
    }

    while(SerialUSB.available()) {
        SerialCloud.write(SerialUSB.read());
    }

    while (Serial2.available()) {
        SerialCloud.write(Serial2.read());
    }

    while (SerialCloud.available()) {
        currChar = (char)SerialCloud.read();
        
        if (currChar == '+') {
            plusSigns++;
        } else {
            plusSigns = 0;
        }

        if (plusSigns == 3) {
            cloudReady = true;
        }
        
        SerialUSB.write(currChar);
        Serial2.write(currChar);
    }

    delay(50);
}
