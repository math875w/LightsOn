/* 
BEKRIVELSE AF KODEN
*/

//bibloteker
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <WiFiNINA.h>
#include <WiFiUdp.h>
#include <Arduino_ConnectionHandler.h>
#include <Encoder.h>
#include <Keypad.h>
#include <FastLED_NeoPixel.h>
#include <U8g2lib.h>
#include <Wire.h>

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
17/A3- knap3 (gul knap midt på kassen)
18/A4- OLED SDA
19/A5- OLED SCL
20/A6- LED-Strip PIN
21/A7- relæ trigger
*/


#define ANALOG_SAMPLE_INTERVAL 10
#define ANALOG_SAMPLE_COUNT 10

#define VRX_PIN  A0 //Joystick y-koordinat tilsluttet til ananlog 0
#define VRY_PIN  A1 //Joystick y-koordinat tilsluttet til ananlog 1
bool lys = false;
bool mouse = false; //variable der tjekker om mus er aktiv
int xValue = 0; //gemmer x-værdien fra joystick
int yValue = 0; //gemmer x-værdien fra joystick
int coordinates[2]; //array men koordinaterne fra joystick

#define relay_PIN 21 //relæ tilsluttet til digital 18

#define DATA_PIN 20 //LED-strip tilsluttet til digital 20
#define MAX_NUM_LEDS 6  //Mængden af LED'er på LED-strip
#define BRIGHTNESS 50 //lysstyrke på LED-Strip,  0 (min) to 255 (max)

int numLEDs = 0;        //antaller af LED'er der skal lyse
int first = true;
FastLED_NeoPixel<MAX_NUM_LEDS, DATA_PIN, NEO_GRB> strip; //NeoPixel strip object

  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // Initialization for the used OLED display
unsigned long startTime;
const unsigned long duration = 900000; // 600 seconds (10 minutes) in milliseconds
bool game = true;
bool farve = true;
bool kode = false;
bool knapListener = false;
bool knapPressed = false;

Encoder myEnc(9, 10); //Encoder tilsluttet til digital 9 og digital 10
long oldPosition  = -999; //Variable der holder styr på den sidste encoder position

const int buttonPin = 11;  //Knap1 tilsluttet til digital 11
bool up = true; //variable der tjekker om knappen er oppe
const int buttonPin1 = 12;  //Knap2 tilsluttet til digital 12
bool up1 = true; //variable der tjekker om knappen er oppe
const int buttonPin2 = 17;  //Knap3 tilsluttet til digital 17
bool up2 = true; //variable der tjekker om knappen er oppe

bool doorOpen = false; //Variable der holder styr på om døren skal være åben
bool encoder = false; //Variable der holder styr på om encoder skal være aktiv

String buttonFarve = ""; //string med 0'er og 1'ere fra knap1 og knap2
int lengthFarve = 0; //varable der holder styr på hvor mange farver der er i String buttonFarve

String keypadNum = ""; //string med tal fra numpad
int keypadLen = 0; //varable der holder styr på hvor mange numre der er i String keypadNum

const byte ROWS = 4; //rækker i keypad
const byte COLS = 3; //kolonner i keypad

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[ROWS] = {3, 8, 7, 5}; //keypad tilsluttet til digital 3, 8 , 7 og 5
byte colPins[COLS] = {4, 2, 6}; //keypad tilsluttet til digital 4, 2 og 6

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

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
IPAddress outIp(192, 168, 50, 106);
// IPAddress outIp(10,138,102,211);
const unsigned int outPort = 11000;

// the MAC address is sort of arbitrary
// you can read more on MAC Physical Address online
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

bool wifiIsConnected = false;
//funktion som bliver kørt en gang i starten
void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(relay_PIN, OUTPUT);
  Serial.begin(9600);
  Udp.begin(8888);
  u8g2.begin();
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_fub11_tn);
  startTime = millis();

  conMan.addCallback(NetworkConnectionEvent::CONNECTED, onNetworkConnect);
  conMan.addCallback(NetworkConnectionEvent::DISCONNECTED, onNetworkDisconnect);
  fillSampleBuffer(0);
  lastSampleTime = millis();

  strip.begin();
  strip.setBrightness(BRIGHTNESS);
}

void loop() {
  // Håndterer OSC beskeder
  handleOSC();

  //tjekker om game er igang
  if(game){
    digitalWrite(relay_PIN, LOW);
    // første gang koden kører
    if(first == true){
      strip.setPixelColor(0, strip.Color(0, 0, 255));
      strip.show();
      delay(50);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();
      delay(10000);
      first = false;
    } 

    u8g2.clearBuffer();

    // udregner tiden der er gået
    unsigned long elapsedTime = millis() - startTime;
    
    // udregner tid der er tilbage
    unsigned long remainingTime = duration - elapsedTime;
    
    // laver tiden om til en mere læselig format (mm:ss.sss)
    unsigned int minutes = remainingTime / 60000;
    unsigned int seconds = (remainingTime % 60000) / 1000;
    unsigned int milliseconds = remainingTime % 1000;

    //viser tid på skærm
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d.%03d", minutes, seconds, milliseconds);
    u8g2.setFont(u8g2_font_fub11_tn);
    u8g2.drawStr(5, 64, timeStr); // Viser tiden der er tilbage
    //viser tallet 238 hvis spillerne er nået den opgave
    if(kode){
      u8g2.setFont(u8g2_font_logisoso34_tn);
      u8g2.drawStr(35, 35, "238");
    }
    u8g2.sendBuffer();


    //Laver string for keypadkoden
    char customKey = customKeypad.getKey();
    if (customKey){
      keypadNum += customKey;
      keypadLen += 1;
    }

    //tjekker om Knap1 er klikker og up er sand
    if (digitalRead(buttonPin) == HIGH && up && farve) {
      up = false;
      buttonFarve += "0";
      lengthFarve ++;
    } else if(digitalRead(buttonPin) == LOW && up == false){
      up = true;
    }

    //tjekker om Knap2 er klikker og up1 er sand
    if (digitalRead(buttonPin1) == HIGH && up1 && farve) {
      up1 = false;
      buttonFarve += "1";
      lengthFarve++;
    } else if(digitalRead(buttonPin1) == LOW && up1 == false){
      up1 = true;
    }

    //tjekker om Knap3 er klikker og up2 er sand
    if (digitalRead(buttonPin2) == HIGH && up2) {
      up2 = false;
      lys = true;
      if(knapListener){
        knapPressed = true;
      }
    } else if(digitalRead(buttonPin2) == LOW && up2 == false){
      up2 = true;
    }

    // gør så numLEDs ikke kan blive større end 6
    if (numLEDs >= MAX_NUM_LEDS){
      numLEDs = 6;
    }

    // åbner relæet så brugeren kan åbne døren
    if(doorOpen == true){
      Serial.println("door open");
      digitalWrite(relay_PIN, HIGH);
      delay(7000);
      digitalWrite(relay_PIN, LOW);
      doorOpen = false;
    }

    // printer encoder væærdi
    long newPosition = myEnc.read();
    if (newPosition != oldPosition) {
      oldPosition = newPosition * -1;
    }

    // læser x og y analog værdier
      xValue = analogRead(VRX_PIN);
      yValue = analogRead(VRY_PIN);
      coordinates[0] = xValue;
      coordinates[1] = yValue;

    unsigned long msNow = millis();
    
    conMan.check();
    if(msNow - lastSampleTime > ANALOG_SAMPLE_INTERVAL){
      samples[sampleIndex] = analogRead(A0);
      sampleIndex++;
      if(sampleIndex == ANALOG_SAMPLE_COUNT) sampleIndex = 0;
    }
    unsigned int potValue = getAverageSample();
 
    //tjekker om mus er blevet aktiveret
    if (wifiIsConnected && mouse) {
      OSCMessage msg("/joystick");
      for (int i = 0; i < 2; i++) {
        msg.add((int32_t)coordinates[i]);
      }

      Udp.beginPacket(outIp, outPort);
      msg.send(Udp);
      Udp.endPacket();
      msg.empty();
    }
    
   //tjekker om encoder er blevet aktiveret
    if (wifiIsConnected && encoder) {
      OSCMessage msg("/encoder");
      Serial.println(newPosition);

      msg.add((int32_t)newPosition);

      Udp.beginPacket(outIp, outPort);
      msg.send(Udp);
      Udp.endPacket();
      msg.empty();
    }

   //tjekker om lys er blevet aktiveret
    if (wifiIsConnected && lys) {
      OSCMessage msg("/lys");

      msg.add((int32_t)lys);

      Udp.beginPacket(outIp, outPort);
      msg.send(Udp);
      Udp.endPacket();
      msg.empty();
      Serial.print(lys);
      lys = false;
    }

    //tjekker om der er sket en fejl og der er kommet en længde på over 5 og derefter clear den
    if(lengthFarve > 5){
      lengthFarve = 0;
      buttonFarve = "";
    }

    //tjekker om der er blevet klikket på den guld og/eller grønne knap 5 gange
    if (wifiIsConnected && lengthFarve == 5) {
      int buttonFarveInt = buttonFarve.toInt();
      OSCMessage msg("/farve");

      msg.add((int32_t)buttonFarveInt);

      Udp.beginPacket(outIp, outPort);
      msg.send(Udp);
      Udp.endPacket();
      msg.empty();
      lengthFarve = 0;
      buttonFarve = "";
    }

    //tjekker om der er blevet klikket på keypad 6 gange
    if (wifiIsConnected && keypadLen == 6) {
      int keypadNumInt = keypadNum.toInt(); 
      OSCMessage msg("/passcode");

      msg.add((int32_t)keypadNumInt);

      Udp.beginPacket(outIp, outPort);
      msg.send(Udp);
      Udp.endPacket();
      msg.empty();
      keypadNum = "";
      keypadLen = 0;
    }
    //tjekker om lys-knapen bliver holdt inde
    if (wifiIsConnected && knapPressed) {
      OSCMessage msg("/knapPressed");

      msg.add((int32_t)knapPressed);

      Udp.beginPacket(outIp, outPort);
      msg.send(Udp);
      Udp.endPacket();
      msg.empty();
    }
    
    //tjekker om spillet er slut
    if (wifiIsConnected && !game) {
      OSCMessage msg("/gameover");

      msg.add((int32_t)game);

      Udp.beginPacket(outIp, outPort);
      msg.send(Udp);
      Udp.endPacket();
      msg.empty();
    }

    //tjekker om tiden er løbet ud
    if (remainingTime > duration) {
      remainingTime = 0;
      game = false;
    }

  } else { //hvis spiller er done 
      // laver LED-strip til rød og clear skærmen
      strip.setPixelColor(0, strip.Color(255, 0, 0));
      strip.setPixelColor(1, strip.Color(255, 0, 0));
      strip.setPixelColor(2, strip.Color(255, 0, 0));
      strip.setPixelColor(3, strip.Color(255, 0, 0));
      strip.setPixelColor(4, strip.Color(255, 0, 0));
      strip.setPixelColor(5, strip.Color(255, 0, 0));
      strip.show();
      u8g2.clearBuffer();
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

  /*
  Tjekker hvilken OSC-besked den har modtaget og 
  går vidre til næste opgave. 
  Samt aktiverer lys på LED-Strip så spilleren kan se når de er 
  færdige med den nuværende opgave
  */
  if (stringOne == "/joystickUnlock") {
    mouse = true;
  }

  if (stringOne == "/knapUnlock") {
    farve = true;
    mouse = false;
    strip.setPixelColor(0, strip.Color(0, 0, 255));
    strip.show();
  }

  if (stringOne == "/displayUnlock") {
    kode = true;
    farve = false;
    strip.setPixelColor(1, strip.Color(0, 0, 255));
    strip.show();
  }

  if (stringOne == "/encoderUnlock") {
    encoder = true;
    kode = false;
    strip.setPixelColor(2, strip.Color(0, 0, 255)); 
    strip.show();
  }

  if (stringOne == "/doorOpen") {
    doorOpen = true;
    encoder = false;
    knapListener = true;
    strip.setPixelColor(3, strip.Color(0, 0, 255));
    strip.show();
  }

  if(stringOne == "/gameDone"){
    game = false;
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
