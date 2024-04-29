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

#define ANALOG_SAMPLE_INTERVAL 10
#define ANALOG_SAMPLE_COUNT 10

#define VRX_PIN  A0 // Arduino pin connected to VRX pin
#define VRY_PIN  A1 // Arduino pin connected to VRY pin

Encoder myEnc(5, 6);
long oldPosition  = -999;

bool mouse = false;
int xValue = 0; // To store value of the X axis
int yValue = 0; // To store value of the Y axis
int coordinates[2];

unsigned int samples[ANALOG_SAMPLE_COUNT];
unsigned long lastSampleTime;
unsigned short sampleIndex = 0;

WiFiUDP Udp;
WiFiConnectionHandler conMan("DigitalTeknik24", "DigiTek24");
// WiFiConnectionHandler conMan("Eucnvs-Guest", "");

//the Arduino board's IP (choose one which is on your network's pool and not reserved)
// IPAddress ip(192, 168, 50, 151);
IPAddress ip(10, 138, 102, 76);

//destination IP (use the IP of the computer you have your OSC Server running)
IPAddress outIp(192, 168, 50, 107);
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
}

void loop() {
  // Handle OSC messages
  handleOSC();


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
  
    //the message wants an OSC address as first argument
  if (wifiIsConnected) {
    OSCMessage msg("/encoder");

    msg.add((int32_t)newPosition);

    // Serial.print("/encoder");
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp); // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    msg.empty(); // free space occupied by message
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
