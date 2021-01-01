#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <NeoPixelBus.h>
#include "credentials.h"
#include <Adafruit_NeoPixel.h>

const int led = LED_BUILTIN;
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
static const uint8_t LEFT_PIN = D5;
static const uint8_t RIGHT_PIN = D6;

const uint16_t PixelCount = 1;
const uint8_t PixelPin = 2;
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

static const int MAX_VALUE = 900;
static const int VUnits = MAX_VALUE;

static double DECAY = 0.8;

#define BUFFER_LEN 2

// Wifi and socket settings
unsigned int localPort = 7832;
char packetBuffer[BUFFER_LEN];

int lastLeft = 0;
int lastRight = 0;
int holdDelay = 81;
int dropDelay = 11;
int red = 255;
int green = 150;
int blue = 50;

WiFiUDP port;
ESP8266WebServer server(80);

void connectWifi() {
  WiFi.begin(STASSID, STAPSK);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.println(STASSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

void handleRoot() {
  String ret = String("<html><head><style>input{width: 16em;height: 16em;} body{font-size: 8em;}.slider{width:66%; -webkit-appearance: none; appearance: none; height: 16em;}.slider::-webkit-slider-thumb{-webkit-appearance: none; appearance: none; width: 16em; height: 16em;}.slider::-moz-range-thumb{width: 16em; height: 16em; cursor: pointer;}</style></head><body> <form id='form' method='post' action='/postform/'>${holdDelay}<input type='submit' name='decreaseDelay' value='-'><input type='submit' name='increaseDelay' value='+'><br>${dropDelay}<input type='submit' name='decreaseDropDelay' value='-'><input type='submit' name='increaseDropDelay' value='+'><br>R<input class='slider' type='range' min='0' max='255' value='${red}' name='red' oninput=\"document.getElementById('fRed').innerHTML=this.value\" onchange=\"document.getElementById('form').submit()\"><label id='fRed'></label><br>G<input class='slider' type='range' min='0' max='255' value='${green}' name='green' oninput=\"document.getElementById('fGreen').innerHTML=this.value\" onchange=\"document.getElementById('form').submit()\"><label id='fGreen'></label><br>B<input class='slider' type='range' min='0' max='255' value='${blue}' name='blue' oninput=\"document.getElementById('fBlue').innerHTML=this.value\" onchange=\"document.getElementById('form').submit()\"><label id='fBlue'></label><br></form></body></html>");
  ret.replace("${holdDelay}", String(holdDelay));
  ret.replace("${dropDelay}", String(dropDelay));
  ret.replace("${red}", String(red));
  ret.replace("${green}", String(green));
  ret.replace("${blue}", String(blue));
  server.send(200,"text/html", ret);
}

void handleForm() {
  String message = "POST form was:\n";
  if (server.argName(0) == "decreaseDelay" && holdDelay > 10) {
    holdDelay -= 10;
  }
  if (server.argName(0) == "increaseDelay") {
    holdDelay += 10;
  }
  if (server.argName(0) == "decreaseDropDelay" && dropDelay > 10) {
    dropDelay -= 10;
  }
  if (server.argName(0) == "increaseDropDelay") {
    dropDelay += 10;
  }
  if (server.hasArg("red")) {
    red = server.arg("red").toInt();
  }
  if (server.hasArg("green")) {
    green = server.arg("green").toInt();
  }
  if (server.hasArg("blue")) {
    blue = server.arg("blue").toInt();
  }
  setColor(red, green, blue);
  Serial.println(server.argName(0));
  Serial.print("Hold Delay: ");
  Serial.print(holdDelay);
  Serial.print(" Drop Delay: ");
  Serial.print(dropDelay);
  Serial.print(" RGB: ");
  Serial.print(red);
  Serial.print(" ");
  Serial.print(green);
  Serial.print(" ");
  Serial.println(blue);
  handleRoot();
}

void setColor(int redValue, int greenValue, int blueValue) {
  RgbColor c(redValue, greenValue, blueValue);
  strip.SetPixelColor(0, c);
  strip.SetPixelColor(1, c);
  strip.Show();

  //Serial.print(redValue);
  //Serial.print(" ");
  //Serial.print(greenValue);
  //Serial.print(" ");
  //Serial.println(blueValue);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting");
    
    pinMode(led, OUTPUT);
    pinMode(LEFT_PIN,OUTPUT);
    pinMode(RIGHT_PIN,OUTPUT);
    analogWrite(LEFT_PIN, LOW);
    analogWrite(RIGHT_PIN, LOW);

    strip.Begin();
    strip.Show();
    setColor(255,255,255);
 
    digitalWrite(led, LOW);

    connectWifi();
    
    digitalWrite(led, HIGH);
    delay(500);
    digitalWrite(led, LOW);
    port.begin(localPort);
    digitalWrite(led, HIGH);

    server.on("/",handleRoot);
    server.on("/postform/", handleForm);
    server.begin();
}


void loop() {
    int packetSize = port.parsePacket();
    if (packetSize) {
        int len = port.read(packetBuffer, 2);
        //Serial.println("Received ");
        float left = packetBuffer[0];
        float right = packetBuffer[1];
        //Serial.print("Left ");
        //Serial.print(left);
        //Serial.print(" Right ");
        //Serial.print(right);
        left = (left * MAX_VALUE) / 255;
        right = (right * MAX_VALUE) / 255;
        //Serial.print(" -> Left ");
        //Serial.print(left);
        //Serial.print(" Right ");
        //Serial.println(right);
        analogWrite(LEFT_PIN, left);
        analogWrite(RIGHT_PIN, right);
        setColor(red,green,blue);

        delay(holdDelay);
      
        analogWrite(LEFT_PIN, left / 2);
        analogWrite(RIGHT_PIN, right / 2);
        delay(dropDelay);
        
        lastLeft = left;
        lastRight = right;
    } else {
        Serial.println("Zero ");
        lastLeft = lastLeft * DECAY;
        lastRight = lastRight * DECAY;
        analogWrite(LEFT_PIN, lastLeft);
        analogWrite(RIGHT_PIN, lastRight);
        setColor(0,0,0);
        delay(holdDelay);
    }
    server.handleClient();
}
