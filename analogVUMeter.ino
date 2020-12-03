#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include "credentials.h"

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

static const int MAX_VALUE = 900;
static const int VUnits = MAX_VALUE;
int vibrationDelay = 100;

static const uint8_t LEFT_PIN = D5;
static const uint8_t RIGHT_PIN = D6;

#define BUFFER_LEN 2

// Wifi and socket settings
unsigned int localPort = 7777;
char packetBuffer[BUFFER_LEN];


WiFiUDP port;
ESP8266WebServer server(80);


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Analog VU meter</title>
</head>
<body>
    <h1>Analog VU meter</h1>
     <form method='post' enctype='application/x-www-form-urlencoded' action='/postform/'>
        Delay:
        <input type='submit' name='decreaseDelay' value='-'>
        <input type='submit' name='increaseDelay' value='+'><br>
    </form>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200,"text/html",index_html);
}

void handleForm() {
  String message = "POST form was:\n";
  if (server.argName(0) == "decreaseDelay") {
    vibrationDelay -= 10;
  }
  if (server.argName(0) == "increaseDelay") {
    vibrationDelay += 10;
  }
  Serial.print("Delay: ");
  Serial.println(vibrationDelay);
  handleRoot();
}

void setup() {
    pinMode(led, OUTPUT);
    pinMode(LEFT_PIN,OUTPUT);
    pinMode(RIGHT_PIN,OUTPUT);
    analogWrite(LEFT_PIN, LOW);
    analogWrite(RIGHT_PIN, LOW);    
    digitalWrite(led, LOW);
    Serial.begin(115200);
    Serial.println("Starting");

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
        Serial.println(packetBuffer[0]);
        Serial.println(packetBuffer[1]);
        int left = packetBuffer[0] * MAX_VALUE / 255;
        int right = packetBuffer[1] * MAX_VALUE / 255;

        left = random(MAX_VALUE);
        right = random(MAX_VALUE);
        /*
        Serial.print("Left ");
        Serial.print(left);
        Serial.print(" Right ");
        Serial.println(right);
        */
        analogWrite(LEFT_PIN, left);
        analogWrite(RIGHT_PIN, right);
        delay(vibrationDelay);
        analogWrite(LEFT_PIN, 0);
        analogWrite(RIGHT_PIN, 0);
        delay(vibrationDelay);
    } else {
        analogWrite(LEFT_PIN, 0);
        analogWrite(RIGHT_PIN, 0);
        delay(vibrationDelay);
    }
    server.handleClient();
}


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
