# Agri_IoT
/*
this is a project to display image in 1.69 inch display for display a image screen
pinouts
Din = 23
clk = 18
cs = 15
dc = 21
rst = 4
bl = 32
motor incidcator = 5
*/


#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebServer.h>

#include "fieldlevelonline.h"// #include "fieldlevel.h" contains the pixel data for the background image (240x280px)
#include "fieldleveloffline.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);


const char* ssid = "vc";//Agri_IoT
const char* password = "vikram7417";//Agri@123
WebServer server(80);

// 🔥 Static IP settings
IPAddress local_IP(192, 168, 0, 40);   // choose free IP (192, 168, 0, ??)
IPAddress gateway(192, 168, 0, 255);      // from ipconfig (192, 168, 0, 255)
IPAddress subnet(255, 255, 255, 0); //(255, 255, 255, 0)

#define IMG_W 240// Image width in pixels
#define IMG_H 280// Image height in pixels


#define HEX565(c) tft.color565((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF)// Convert 24-bit RGB color to 16-bit RGB565 format
#define WATER_COLOR HEX565(0x5BC0EB)// Light Blue (Water)
#define MOIST_COLOR HEX565(0x019b03)// Dark Green (Soil Moisture)

#define pumpPin 5


float waterlevel = 0.0;
float moisture = 0.0;
float ph = 50.0;
float rlevel = 0.0;
int field = 1;

void updateValues();
void handleData();
void fieldupadete();
bool pumpstate();

void setup() {
  Serial.begin(115200);

  pinMode(pumpPin, OUTPUT);
  digitalWrite(pumpPin, LOW); // Ensure pump is off at startup


  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  sprite.createSprite(120, 30);// Create sprite (small area for text)
  

  if (!WiFi.config(local_IP, gateway, subnet)) Serial.println("Static IP Failed");// 🔥 Apply static IP BEFORE connecting

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    tft.pushImage(0, 0, IMG_W, IMG_H, fieldleveloffline);// Draw background UI once
    delay(500);
    Serial.print(".");
  }
  tft.pushImage(0, 0, IMG_W, IMG_H, fieldlevelonline);// Draw background UI once

  Serial.println("\nConnected!");
  Serial.print("IP Address: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/set?level=VALUE&pump=on|off");

  server.on("/set", handleData);
  server.begin();

}

void loop() {
  server.handleClient();
  updateValues();
  fieldupadete();

  if(pumpstate()){
    digitalWrite(pumpPin, HIGH);
  }
  else{
    digitalWrite(pumpPin, LOW);
  }


  delay(1000);
}

void drawValue(int x, int y, const char* text, uint16_t color) {

  int w = 120;
  int h = 30;

  // ✅ Step 1: Restore background properly (row by row)
  for (int row = 0; row < h; row++) {
    tft.pushImage(
      x,
      y + row,
      w,
      1,
      &fieldlevelonline[(y + row) * IMG_W + x]
    );
  }

  // ✅ Step 2: Draw text using transparent sprite
  sprite.fillSprite(TFT_BLACK);
  sprite.setTextColor(color, TFT_BLACK); // Set text color with black background for transparency
  sprite.setTextSize(3);
  sprite.setCursor(0, 5);

  sprite.print(text);

  sprite.pushSprite(x, y, TFT_BLACK);

}

void updateValues() {
  char buffer[20];

  // WATER LEVEL
  sprintf(buffer, "%.1fcm", rlevel);
  drawValue(75, 90, buffer, WATER_COLOR);

  // MOISTURE
  sprintf(buffer, "%.1f%%", moisture);
  drawValue(75, 160, buffer, TFT_GREEN);

  // PH
  sprintf(buffer, "%.1f", ph);
  drawValue(75, 230, buffer, TFT_WHITE);


}


void handleData() {
  if (server.hasArg("level")) {
    String val = server.arg("level");
    rlevel = val.toInt();

    Serial.print("Received Value: ");
    Serial.println(rlevel);

    server.send(200, "text/plain", "Value received");
  } else {
    server.send(400, "text/plain", "No value sent");
  }
}

bool pumpstate(){
  if(server.hasArg("pump")){
    String val = server.arg("pump");
    if(val == "on"){
      Serial.println("Pump ON");
      server.send(200, "text/plain", "Pump turned ON");
      return true;
    }
    else if(val == "off"){
      Serial.println("Pump OFF");
      server.send(200, "text/plain", "Pump turned OFF");
      return false;
    }
  }
}

void fieldupadete(){
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(215, 15);
  tft.print((String)field);
}
