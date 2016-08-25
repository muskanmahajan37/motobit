int pins[3] = {L09, L08, L07};
int curColVal[3] = {0,0,0};
int val;
int colorStep = 2;
int nextColorStep = 0;

int xPin = R04;
int yPin = R05;

void setup()
{
  for(int i = 0; i < 3; ++i) {
    pinMode(pins[i], OUTPUT);
  }
  SerialUSB.begin(9600);
  SerialCloud.begin(115200);
  delay(4000);

  Dash.begin();
  //Dash.pulseLED(100,5000);

  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
}
int pickRandom()
{
  int r = random(0,2);
  if(r == 1) {
    return 1;
  }
  return 0;
}
String tempBuffer = "";
String payload = "";
bool foundSMS = false;
int curColor = 0;
#define NUMCOLORS 10
String colors[] =
  {"BLACK", "RED", "GREEN", "BLUE", "WHITE",
   "PURPLE", "YELLOW", "CYAN", "PINK", "BT"};
int colorVals[][3] =
  { { 0, 0, 0 },
    { 255, 0, 0 },
    { 0, 255, 0 },
    { 0, 0, 255 },
    { 255, 255, 255},
    { 255, 0, 255},
    { 255, 255, 0 },
    { 0, 255, 255},
    { 255, 191, 204},
    { 0, 255, 204}
    };
int findColor(String color)
{
  color.toUpperCase();
  SerialUSB.println(color);

  int i;
  for (i = 0; i < NUMCOLORS; ++i) {
    SerialUSB.println(colors[i]);
    if (color == colors[i]) {
      SerialUSB.println("Color found");
      return i;
    }
  }
  return 0;
}
void loop()
{
//  char currChar;
//  while (SerialCloud.available()) {
//    currChar = (char)SerialCloud.read();
//    // check if the current buffer hits the SMSRCVD code.
//    if (!foundSMS) {
//      if (tempBuffer == "SMSRCVD") {
//        foundSMS = true;
//      }
//    }
//    // If it received the SMSRCVD code, the payload will get populated until
//    // we get a \n.
//    else if (currChar == '\n'){
//      SerialUSB.println("\nSMS received: ");
//      payload = stripOffLengthNumber(payload);
//      payload.trim();
//      SerialUSB.println(payload);
//      curColor = findColor(payload);
//      // reset foundSMS and the payload for the next iteration.
//      foundSMS = false;
//      payload = "";
//    }
//    else {
//      payload.concat(currChar);
//    }
//    // Only keep a sliding buffer length of size 7
//    // (need to only check for SMSRCVD).
//    if (tempBuffer.length() >= 7) {
//       tempBuffer.remove(0, 1);
//    }
//    // add latest char to our buffer.
//    tempBuffer.concat(currChar);
//    SerialUSB.write(currChar);
//  }
//  if(nextColorStep < millis()) {
//    nextColorStep = millis() + 100;
//    for(int i = 0; i < 3; ++i) {
//      if(curColVal[i] != colorVals[curColor][i]) {
//        int remaining = colorVals[curColor][i] - curColVal[i];
//        if(remaining < 0) {
//          curColVal[i] -= min((-1*remaining), colorStep);
//        } else {
//          curColVal[i] += min(remaining, colorStep);
//        }
//        analogWrite(pins[i], curColVal[i]);
//      }
//    }
//  }
  //delay(50);

  // variables to read the pulse widths:
  int pulseX, pulseY;
  // variables to contain the resulting accelerations
  int accelerationX, accelerationY;

  // read pulse from x- and y-axes:
  pulseX = pulseIn(xPin, HIGH);
  pulseY = pulseIn(yPin, HIGH);

  // convert the pulse width into acceleration
  // accelerationX and accelerationY are in milli-g's:
  // earth's gravity is 1000 milli-g's, or 1g.
  accelerationX = ((pulseX / 10) - 500) * 8;
  accelerationY = ((pulseY / 10) - 500) * 8;

  // print the acceleration
  SerialUSB.print(accelerationX);
  // print a tab character:
  SerialUSB.print("\t");
  SerialUSB.print(accelerationY);
  SerialUSB.println();

  delay(100);
}

unsigned long pulseIn(uint8_t pin, uint8_t state) {

    unsigned long pulseWidth = 0;
    unsigned long loopCount = 0;
    unsigned long loopMax = 5000000;

    // While the pin is *not* in the target state we make sure the timeout hasn't been reached.
    while ((digitalRead(pin)) != state) {
        if (loopCount++ == loopMax) {
            return 0;
        }
    }

    // When the pin *is* in the target state we bump the counter while still keeping track of the timeout.
    while ((digitalRead(pin)) == state) {
        if (loopCount++ == loopMax) {
            return 0;
        }
        pulseWidth++;
    }

    // Return the pulse time in microsecond!
    return pulseWidth * 2.36; // Calculated the pulseWidth++ loop to be about 2.36uS in length.
}

String stripOffLengthNumber(String payload) {
  int index = 0;
  while (payload[index] == ',') {
    ++index;
  }
  while (payload[index] != ',') {
    ++index;
  }
  ++index;
  payload.remove(0, index);
  return payload;
}
