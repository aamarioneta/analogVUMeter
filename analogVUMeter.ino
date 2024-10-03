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

static const int MAX_STANDBY_BRIGHTNESS = 20;
static const int MILLIS_UNTIL_STANDBY = 3000;
static const uint8_t MAX_VOLTAGE = 1023;
static const float COLOR_DECAY = 0.75;

#define BUFFER_LEN 2

// Wifi and socket settings
unsigned int localPort = 7832;
char packetBuffer[BUFFER_LEN];

float lastLeft = 0;
float lastRight = 0;
int lastRed = 0;
int lastGreen = 0;
int lastBlue = 0;

int holdDelay = 1;
int dropDelay = 1;
int red = 255;
int green = 0;
int blue = 80;
float decay = 0.33;

WiFiUDP port;
ESP8266WebServer server(80);

void connectWifi() {
  WiFi.begin(STASSID, STAPSK);
  WiFi.hostname(HOSTNAME);
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

const char HTML[] PROGMEM = R"=====(
<html>
<head>
    <style>
        input {
            width: 16em;
            height: 16em;
        }

        body {
            font-size: 8em;
        }

        .slider {
            width: 66%;
            -webkit-appearance: none;
            appearance: none;
            height: 16em;
        }

        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 16em;
            height: 16em;
        }

        .slider::-moz-range-thumb {
            width: 16em;
            height: 16em;
            cursor: pointer;
        }
    </style>
</head>

<body>
    <form id='form' method='post' action='/postform/'>
        ${holdDelay}
        <input type='submit' name='decreaseDelay' value='-'>
        <input type='submit' name='increaseDelay' value='+'>
        <br>
        ${dropDelay}
        <input type='submit' name='decreaseDropDelay' value='-'>
        <input type='submit' name='increaseDropDelay' value='+'>
        <br><label id='decay'></label><br>decay<input class='slider' type='range' min='0' max='1000' value='${decay}' name='decay' oninput="document.getElementById('decay').innerHTML=this.value" onchange="document.getElementById('form').submit()">
        <br><label id='fRed'>R</label><input class='slider' type='range' min='0' max='255' value='${red}' name='red' oninput="document.getElementById('fRed').innerHTML=this.value" onchange="document.getElementById('form').submit()">
        <br><label id='fGreen'>G</label><input class='slider' type='range' min='0' max='255' value='${green}' name='green' oninput="document.getElementById('fGreen').innerHTML=this.value" onchange="document.getElementById('form').submit()">
        <br><label id='fBlue'>B</label><input class='slider' type='range' min='0' max='255' value='${blue}' name='blue' oninput="document.getElementById('fBlue').innerHTML=this.value" onchange="document.getElementById('form').submit()">
    </form>
</body>
</html>
)=====";

void handleRoot() {
  String ret = String(HTML);
  ret.replace("${holdDelay}", String(holdDelay));
  ret.replace("${dropDelay}", String(dropDelay));
  ret.replace("${decay}", String(decay * 1000));
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
  if (server.hasArg("decay")) {
    decay = server.arg("decay").toFloat() / 1000.0;
  }
  setColor(red, green, blue);
  Serial.println(server.argName(0));
  Serial.print("Hold Delay: ");
  Serial.print(holdDelay);
  Serial.print(" Drop Delay: ");
  Serial.print(dropDelay);
  Serial.print(" decay: ");
  Serial.print(decay);
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
    lastLeft = 512;
    lastRight = 512;
    lastRed = red;
    lastGreen = green;
    lastBlue = blue;
}

long lastMillis = 0;
float maxLeft = 255;
float maxRight = 255;

void loop() {
    int packetSize = port.parsePacket();
    if (packetSize) {
        int len = port.read(packetBuffer, 2);
        float left = packetBuffer[0];
        float right = packetBuffer[1];
        maxLeft = max(maxLeft, left);
        maxRight = max(maxRight, right);
        left = max(lastLeft, left);
        right = max(lastRight, right);
        int vLeft = (int)(left * MAX_VOLTAGE / maxLeft);
        int vRight = (int)(right * MAX_VOLTAGE / maxRight);
        Serial.print(" Input Left ");
        Serial.print(left);
        Serial.print(" Right ");
        Serial.print(right);
        Serial.print(" | maxLeft: ");
        Serial.print(maxLeft);
        Serial.print(" maxRight: ");
        Serial.print(maxRight);
        Serial.print(" | lastLeft: ");
        Serial.print(lastLeft);
        Serial.print(" lastRight: ");
        Serial.print(lastRight);
        Serial.print(" | decay: ");
        Serial.print(decay);
        Serial.print(" | Voltage Left ");
        Serial.print(vLeft);
        Serial.print(" Right ");
        Serial.println(vRight);

        analogWrite(LEFT_PIN, vLeft);
        analogWrite(RIGHT_PIN, vRight);
        setColor(red,green,blue);
        delay(holdDelay);
        
        lastRed = red;
        lastGreen = green;
        lastBlue = blue;
        lastLeft = left * decay;
        lastRight = right * decay;
        lastMillis = millis();
    } else {
        if ((millis() - lastMillis) > MILLIS_UNTIL_STANDBY) {
          // Serial.print("Standby ");
          // Serial.print(lastRed);
          // Serial.print(" ");
          // Serial.print(lastGreen);
          // Serial.print(" ");
          // Serial.println(lastBlue);
          if ((lastLeft > 1) || (lastRight > 1)) {
            lastLeft = lastLeft * decay;
            lastRight = lastRight * decay;
            analogWrite(LEFT_PIN, lastLeft);
            analogWrite(RIGHT_PIN, lastRight);
          }
          if ((lastRed > MAX_STANDBY_BRIGHTNESS) || (lastGreen > MAX_STANDBY_BRIGHTNESS) || (lastBlue > MAX_STANDBY_BRIGHTNESS)) {
            lastRed = lastRed * COLOR_DECAY;
            lastGreen = lastGreen * COLOR_DECAY;
            lastBlue = lastBlue * COLOR_DECAY;
            setColor(lastRed,lastGreen,lastBlue);
          }
          delay(holdDelay);
        }
    }
    server.handleClient();
}
