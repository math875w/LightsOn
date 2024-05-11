/* This Sketch is tested on 
 * - Nano 33 RP2040 Connect
 * - Nano 33 IoT
 * - MKR WiFi 1010
 
 To compile and use it requires
 
 Libraries:
 - CNMAT OSC Library (https://github.com/CNMAT/OSC) - Arduino Library Manager
 - Arduino WiFiNINA (https://github.com/arduino-libraries/WiFiNINA) - Arduino Library Manager
 - Arduino_ConnectionHandler (https://github.com/arduino-libraries/Arduino_ConnectionHandler) - Arduino Library Manager
 
 OSC Server:
 A Max/MSP patch is provided as a comment at the bottom of this Sketch
*/

#include <OSCMessage.h>
#include <OSCBundle.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <Arduino_ConnectionHandler.h>
#include <Encoder.h>
#include <Keypad.h>
#include <FastLED_NeoPixel.h>

/* 
PINOUT
2- keypad kolonne 2 
3- keypad række 1
4- keypad kolonne 1 
5- keypad række 4 
6- keypad kolonne 3 
7- keypad række 3 
8- keypad række 2 
9- Encoder DT
10- Encoder CLK
11- Knap1 (grøn knap)
12- knap2 (gul knap)
13-
14/A0- JoyStick x-koordinat Analog 
15/A1- JoyStick y-koordinat Analog 
16/A2-
17/A3-
18/A4- relæ trigger 
19/A5-
20/A6- LED-Strip PIN
21/A7-
*/


#define ANALOG_SAMPLE_INTERVAL 10
#define ANALOG_SAMPLE_COUNT 10

#define VRX_PIN  A0 // Arduino pin connected to VRX pin
#define VRY_PIN  A1 // Arduino pin connected to VRY pin
#define relay_PIN 18

// Which pin on the Arduino is connected to the LEDs?
#define DATA_PIN 20
// How many LEDs are attached to the Arduino?
#define MAX_NUM_LEDS 6  // Maximum number of LEDs
// LED brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 50

int numLEDs = 0;        // Initial number of LEDs to light up
FastLED_NeoPixel<MAX_NUM_LEDS, DATA_PIN, NEO_GRB> strip; // NeoPixel strip object

Encoder myEnc(9, 10);
long oldPosition  = -999;

bool mouse = false;
int xValue = 0; // To store value of the X axis
int yValue = 0; // To store value of the Y axis
int coordinates[2];

const int buttonPin = 11;  // the number of the pushbutton pin
bool up = true;
const int buttonPin1 = 12;  // the number of the pushbutton pin
bool up1 = true;

bool doorOpen = false;
bool encoder = false;

String buttonFarve = "";
int lengthFarve = 0;

String keypadNum = "";
int keypadLen = 0;

const byte ROWS = 4;
const byte COLS = 3;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {3, 8, 7, 5}; 
byte colPins[COLS] = {4, 2, 6};

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

unsigned int samples[ANALOG_SAMPLE_COUNT];
unsigned long lastSampleTime;
unsigned short sampleIndex = 0;

WiFiUDP Udp;
WiFiConnectionHandler conMan("larsen_ext", "samitho3");
// WiFiConnectionHandler conMan("DigitalTeknik24", "DigiTek24");
// WiFiConnectionHandler conMan("Eucnvs-Guest", "");

//the Arduino board's IP (choose one which is on your network's pool and not reserved)
// IPAddress ip(192, 168, 50, 151);
IPAddress ip(10, 138, 102, 76);

//destination IP (use the IP of the computer you have your OSC Server running)
IPAddress outIp(192, 168, 50, 106);
// IPAddress outIp(10,138,102,211);
const unsigned int outPort = 11000;

// the MAC address is sort of arbitrary
// you can read more on MAC Physical Address online
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

bool wifiIsConnected = false;
void setup() {
  Serial.begin(9600);

  Udp.begin(8888);
  conMan.addCallback(NetworkConnectionEvent::CONNECTED, onNetworkConnect);
  conMan.addCallback(NetworkConnectionEvent::DISCONNECTED, onNetworkDisconnect);
  fillSampleBuffer(0);
  lastSampleTime = millis();
  pinMode(buttonPin, INPUT);
  pinMode(buttonPin1, INPUT);
  pinMode(relay_PIN, OUTPUT);
  strip.begin(); // Initialize strip
  strip.setBrightness(BRIGHTNESS);
  strip.clear();
  strip.show();
}

void loop() {
  digitalWrite(relay_PIN, LOW);
  // Handle OSC messages
  handleOSC();

  // creating the string for keypad code
  char customKey = customKeypad.getKey();
  if (customKey){
    keypadNum += customKey;
    keypadLen += 1;
  }

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if (digitalRead(buttonPin1) == HIGH && up1) {
      up1 = false;
      buttonFarve += "1";
      lengthFarve += 1;
      strip.setPixelColor(numLEDs, strip.Color(0, 255, 0));
      strip.show();
      numLEDs++;
    } else if(digitalRead(buttonPin1) == LOW){
      up1 = true;
    }

    if (numLEDs >= MAX_NUM_LEDS){
      numLEDs = 6;
    }

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if (digitalRead(buttonPin) == HIGH && up) {
      up = false;  
      buttonFarve += "0";
      lengthFarve += 1;
      doorOpen = true;
      Serial.println("door open");
    } else if(digitalRead(buttonPin) == LOW){
      up = true;
    }
    delay(50);

    if(doorOpen == true){
      digitalWrite(relay_PIN, HIGH);
      delay(7000);
      digitalWrite(relay_PIN, LOW );
      doorOpen = false;
    }

  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    oldPosition = newPosition;
    Serial.println(newPosition);
  }

  // read analog X and Y analog values
  xValue = analogRead(VRX_PIN);
  yValue = analogRead(VRY_PIN);

  // print data to Serial Monitor on Arduino IDE
  coordinates[0] = xValue;
  // Serial.print("x = ");
  // Serial.print(xValue);
  coordinates[1] = yValue;
  // Serial.print(", y = ");
  // Serial.println(yValue);
  unsigned long msNow = millis();
  
  conMan.check();
  if(msNow - lastSampleTime > ANALOG_SAMPLE_INTERVAL){
    samples[sampleIndex] = analogRead(A0);
    sampleIndex++;
    if(sampleIndex == ANALOG_SAMPLE_COUNT) sampleIndex = 0;
  }
  unsigned int potValue = getAverageSample();
  // Serial.println(potValue);
  //the message wants an OSC address as first argument
  if (wifiIsConnected && mouse == true) {
    OSCMessage msg("/joystick");
    for (int i = 0; i < 2; i++) {
      msg.add((int32_t)coordinates[i]);
    }

    delay(100);

    Udp.beginPacket(outIp, outPort);
    msg.send(Udp); // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    msg.empty(); // free space occupied by message
  }
  
  // the message wants an OSC address as first argument
  if (wifiIsConnected && encoder == true) {
    OSCMessage msg("/encoder");

    msg.add((int32_t)newPosition);

    // Serial.print("/encoder");
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp); // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    msg.empty(); // free space occupied by message
  }

  if (wifiIsConnected && lengthFarve == 5) {
    int buttonFarveInt = buttonFarve.toInt(); 
    OSCMessage msg("/farve");

    msg.add((int32_t)buttonFarveInt);

    // Serial.print("/encoder");
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp); // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    msg.empty(); // free space occupied by message
    lengthFarve = 0;
    buttonFarve = "";
  }

  if (wifiIsConnected && keypadLen == 6) {
    int keypadNumInt = keypadNum.toInt(); 
    OSCMessage msg("/passcode");

    msg.add((int32_t)keypadNumInt);

    // Serial.print("/encoder");
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp); // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    msg.empty(); // free space occupied by message
    keypadNum = "";
    keypadLen = 0;
  }
   

}

void handleOSC() {
  int size = Udp.parsePacket();
  if (size > 0) {
    OSCMessage msg;
    while (size--) {
      char c = Udp.read();
      msg.fill(c);
    }
    if (!msg.hasError()) {
      Serial.print("Received OSC Message: ");
      // Handle OSC message here
      handleOSCMessage(msg);
      Serial.println();
    }
  }
}


void handleOSCMessage(OSCMessage &msg) {
  Serial.println("Address: ");
  Serial.println(msg.getAddress());
  String stringOne = msg.getAddress();  
  // Check if the OSC message address is "/unlock"
  if (stringOne == "/unlock") {
    // Set the mouse variable to true
    mouse = true;
    Serial.println("Mouse unlocked");
  }

  if (stringOne == "/encoder") {
    // Set the mouse variable to true
    encoder = true;
    Serial.println("encoder unlocked");
  }
  // if (stringOne == "/opendoor") {
  // // Set the mouse variable to true
  //   doorOpen = true;
  //   Serial.println("Door opened");
  // }
}




void handleAnalogMessage(OSCMessage &msg, int addrOffset) {
  Serial.println("Analog Message Received:");
  for (int i = 0; i < msg.size(); i++) {
    Serial.print("Argument ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(msg.getInt(i));
  }
}

void onNetworkConnect() {
  Serial.println("Connection open");
  wifiIsConnected = true;
  Serial.print("Arduino IP Address: ");
  Serial.println(WiFi.localIP());
}
void onNetworkDisconnect() {
  Serial.println("Connection closed");
  wifiIsConnected = false;
}

void fillSampleBuffer(unsigned int _value){
  for(uint8_t s = 0; s < ANALOG_SAMPLE_COUNT; s++){
    samples[s] = _value;
  }
}

uint32_t getAverageSample(){
  uint32_t returnValue= 0;
  for(uint8_t s = 0; s < ANALOG_SAMPLE_COUNT; s++){
    returnValue += samples[s];
  }
  return returnValue / ANALOG_SAMPLE_COUNT;
}
