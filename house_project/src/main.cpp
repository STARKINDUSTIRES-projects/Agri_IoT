#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include "houseimage.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

const char* ssid = "STIC";
const char* password = "stic@123";

WebServer server(80);

// Network
IPAddress local_IP(192, 168, 0, 5);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

#define IMG_W 240
#define IMG_H 280

#define greenlight 27
#define redlight 26

// ✅ RENAMED
int house = 3;

// Data
float waterConsumed = 0.0;
float waterPrice = 0.0;

bool newdata = true;
bool lastWiFiState = false;

//----------------------------------

void drawValue(int x, int y, const char* text, uint16_t color) {
  int w = 140, h = 30;

  for (int row = 0; row < h; row++) {
    tft.pushImage(x, y + row, w, 1,
      &houseimage[(y + row) * IMG_W + x]);
  }

  sprite.fillSprite(TFT_BLACK);
  sprite.setTextSize(3);
  sprite.setTextColor(color, TFT_BLACK);
  sprite.setCursor(0, 5);
  sprite.print(text);
  sprite.pushSprite(x, y, TFT_BLACK);
}

//----------------------------------

void updateValues() {
  char buffer[30];

  // Water Consumed
  sprintf(buffer, "%.1f L", waterConsumed);
  drawValue(100, 90, buffer, TFT_CYAN);

  // Water Price
  sprintf(buffer, "Rs %.1f", waterPrice);
  drawValue(100, 180, buffer, TFT_YELLOW);
}

//----------------------------------

void drawWiFiStatus(bool connected) {
  int x = 140, y = 10;
  int w = 80, h = 25;

  for (int row = 0; row < h; row++) {
    tft.pushImage(x, y + row, w, 1,
      &houseimage[(y + row) * IMG_W + x]);
  }

  if (connected) {
    tft.fillCircle(x + 3, y + 10, 5, TFT_GREEN);
    tft.setCursor(x + 13, y + 3);
    tft.setTextSize(2);
    tft.print("Online");
  } else {
    tft.fillCircle(x + 3, y + 10, 5, TFT_RED);
    tft.setCursor(x + 13, y + 3);
    tft.setTextSize(2);
    tft.print("Offline");
  }
}

//----------------------------------

void drawHouse() {
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(100, 40);
  tft.print(house);
}

//----------------------------------

void handleData() {
  String response = "";

  // House number
  if (server.hasArg("house")) {
    house = server.arg("house").toInt();
    response += "House OK | ";
  }

  // Water Consumed
  if (server.hasArg("consumed")) {
    waterConsumed = server.arg("consumed").toFloat();
    response += "Consumed OK | ";
  }

  // Water Price
  if (server.hasArg("price")) {
    waterPrice = server.arg("price").toFloat();
    response += "Price OK | ";
  }

  if (response == "") {
    server.send(400, "text/plain", "No valid parameters");
  } else {
    newdata = true;
    server.send(200, "text/plain", response);
  }
}

//----------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(greenlight, OUTPUT);
  pinMode(redlight, OUTPUT);

  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  sprite.createSprite(140, 30);

  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    tft.pushImage(0, 0, IMG_W, IMG_H, houseimage);
    drawWiFiStatus(false);
    delay(500);
  }

  tft.pushImage(0, 0, IMG_W, IMG_H, houseimage);
  drawWiFiStatus(true);

  server.on("/set", handleData);
  server.begin();
}

//----------------------------------

void loop() {
  bool currentWiFiState = (WiFi.status() == WL_CONNECTED);

  // WiFi status update (same as your original)
  if (currentWiFiState != lastWiFiState) {
    tft.pushImage(0, 0, IMG_W, IMG_H, houseimage);
    drawWiFiStatus(currentWiFiState);
    lastWiFiState = currentWiFiState;
  }

  server.handleClient();

  if (newdata) {
    updateValues();
    newdata = false;
  }

  drawHouse();

  delay(50);
}