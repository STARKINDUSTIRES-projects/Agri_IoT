#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebServer.h>

#include "greenonline.h"// #include "fieldlevel.h" contains the pixel data for the background image (240x280px)
#include "greenoffline.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);


const char* ssid = "Agri_IoT";//Agri_IoT
const char* password = "Agri@123";//Agri@123
WebServer server(80);


// 🔥 Static IP settings
IPAddress local_IP(192, 168, 0, 20);   // choose free IP (192, 168, 0, ??)
IPAddress gateway(192, 168, 0, 255);      // from ipconfig (192, 168, 0, 255)
IPAddress subnet(255, 255, 255, 0); //(255, 255, 255, 0)

#define IMG_W 240// Image width in pixels
#define IMG_H 280// Image height in pixels


#define HEX565(c) tft.color565((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF)// Convert 24-bit RGB color to 16-bit RGB565 format
#define WATER_COLOR HEX565(0x5BC0EB)// Light Blue (Water)
#define MOIST_COLOR HEX565(0x019b03)// Dark Green (Soil Moisture)


//////////////////////////////////

float temperature = 0.0;
float humidity = 0.0;
bool fan_on = false;

/////////////////////////////////

void updateValues();
void handleData();
void fieldupadete();
bool pumpstate();


void setup() {
  Serial.begin(115200);



  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  sprite.createSprite(120, 30);// Create sprite (small area for text)
  

  if (!WiFi.config(local_IP, gateway, subnet)) Serial.println("Static IP Failed");// 🔥 Apply static IP BEFORE connecting

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    tft.pushImage(0, 0, IMG_W, IMG_H, greenoffline);// Draw background UI once
    delay(500);
    Serial.print(".");
  }
  tft.pushImage(0, 0, IMG_W, IMG_H, greenonline);// Draw background UI once

  Serial.println("\nConnected!");
  Serial.print("IP Address: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/set?level=VALUE&pump=on|off");

  server.on("/set", handleData);
  server.begin();

}

void loop() {
  if(WiFi.status() != WL_CONNECTED){
    tft.pushImage(0, 0, IMG_W, IMG_H, greenoffline);// Draw background UI once
    delay(500);
    return; // Skip the rest of the loop if not connected
  }
  server.handleClient();
  updateValues();



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
      &greenonline[(y + row) * IMG_W + x]
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
  sprintf(buffer, "%.1f", temperature);
  drawValue(75, 90, buffer, TFT_WHITE);

  // MOISTURE
  sprintf(buffer, "%.1f%%", humidity);
  drawValue(75, 160, buffer, TFT_WHITE);

  // PH
  if(fan_on){
    sprintf(buffer, "ON");
    drawValue(75, 230, buffer, TFT_WHITE);
  }
   else{
     sprintf(buffer, "OFF");
    drawValue(75, 230, buffer, TFT_WHITE);
   }

}


void handleData() {////////////////////////for field esp only////////////////////////////////////////////
  String response = "";

  // LEVEL
  if (server.hasArg("level")) {
    temperature = server.arg("level").toInt();
    Serial.print("Level: ");
    Serial.println(temperature);
    response += "Temperature OK | ";
  }
  if (server.hasArg("humidity")) {
    humidity = server.arg("humidity").toInt();
    Serial.print("Humidity: ");
    Serial.println(humidity);
    response += "Humidity OK | ";
  }
  if (server.hasArg("fan")) {
    String fan = server.arg("fan");

    if (fan == "on") {
      fan_on = true;
      Serial.println("Fan ON");
      response += "Fan ON | ";
    } 
    else if (fan == "off") {
      fan_on = false;
      Serial.println("Fan OFF");
      response += "Fan OFF | ";
    }
  }

  // FINAL RESPONSE (ONLY ONCE)
  if (response == "") {
    server.send(400, "text/plain", "No valid parameters");
  } else {
    server.send(200, "text/plain", response);
  }
}



