/*
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15  // Chip select control pin
#define TFT_DC   25  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
*/



#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebServer.h>


#include "image.h"// #include "fieldlevel.h" contains the pixel data for the background image (240x280px)

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);


const char* ssid = "Agri_IoT";//Agri_IoT
const char* password = "Agri@123";//Agri@123
WebServer server(80);

// 🔥 Static IP settings
IPAddress local_IP(192, 168, 0, 20);   // choose free IP (192, 168, 0, ??) field 2(20) and 3(30)
IPAddress gateway(192, 168, 0, 255);      // from ipconfig (192, 168, 0, 255)
IPAddress subnet(255, 255, 255, 0); //(255, 255, 255, 0)




#define IMG_W 240// Image width in pixels
#define IMG_H 280// Image height in pixels

/////////////////////////////////////oled//////////////////////////////////////////////
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C 
////////////////////////////////////////////////////////////////////////////////////

#define HEX565(c) tft.color565((c>>16)&0xFF, (c>>8)&0xFF, c&0xFF)// Convert 24-bit RGB color to 16-bit RGB565 format
#define WATER_COLOR HEX565(0x5BC0EB)// Light Blue (Water)
#define MOIST_COLOR HEX565(0x019b03)// Dark Green (Soil Moisture)

#define greenlight 27 //for esp32
#define redlight 26 //  for esp32



int field = 2;// 1, 2, 3

//////////////////////////////////
float waterlevel = 0.0;
float moisture = 0.0;
float ph = 50.0;
float rlevel = 0.0;
float rph = 0.0;
float rmoisture = 0.0;
String state = "";



bool newdata = true;

bool pump_on = false;

//////////////////////////////////


void updateValues();
void handleData();
void fieldupadete();
bool pumpstate();
void drawWiFiStatus(bool online);
void seperatescreen(String stateus);
/////////////////////////////////////////////

void setup() {
  Serial.begin(115200);



  pinMode(greenlight, OUTPUT); digitalWrite(greenlight, HIGH); 
  pinMode(redlight, OUTPUT); digitalWrite(redlight, HIGH); 


  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  sprite.createSprite(120, 30);// Create sprite (small area for text)
  

  if (!WiFi.config(local_IP, gateway, subnet)) Serial.println("Static IP Failed");// 🔥 Apply static IP BEFORE connecting

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    tft.pushImage(0, 0, IMG_W, IMG_H, image);// Draw background UI once
    drawWiFiStatus(false);
    delay(500);
    Serial.print(".");
  }
  tft.pushImage(0, 0, IMG_W, IMG_H, image);// Draw background UI once

  Serial.println("\nConnected!");
  Serial.print("IP Address: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/set?level=VALUE&pump=on&ph=23&moisture=15&status=STATUS");

  server.on("/set", handleData);
  server.begin();

}


boolean lastWiFiState = false; // Track last WiFi state for change detection
String lastStatus = "";

bool showStatusScreen = false;
unsigned long statusStartTime = 0;

void loop() {
  bool currentWiFiState = (WiFi.status() == WL_CONNECTED);
//////////////////////////////////////////////////
  // If WiFi just disconnected
 //////////////////////////////////////////////////////////////// 

    if (!currentWiFiState && lastWiFiState == true) {
    tft.pushImage(0, 0, IMG_W, IMG_H, image);
    Serial.println("WiFi Lost");
    drawWiFiStatus(false);   // 🔴 Offline
    }

    if (currentWiFiState && lastWiFiState == false) {
    tft.pushImage(0, 0, IMG_W, IMG_H, image);
    Serial.println("WiFi Connected");
    drawWiFiStatus(true);    // 🟢 Online
    }
  // Update last state
    lastWiFiState = currentWiFiState;

    if(state != lastStatus){
    showStatusScreen = true;
    statusStartTime = millis();
    lastStatus = state;   
}

server.handleClient();

if(showStatusScreen){
  seperatescreen(state);
  if(millis() - statusStartTime > 5000){
    showStatusScreen = false;
    tft.pushImage(0, 0, IMG_W, IMG_H, image); // restore UI
  }

} 
else {}
  if(newdata){
    updateValues();
    newdata = false; // Reset flag after updating values
  }
  fieldupadete();

  if(pump_on){
    digitalWrite(greenlight, HIGH);
    digitalWrite(redlight, LOW);
  }
  else{
    digitalWrite(greenlight, LOW);
    digitalWrite(redlight, HIGH);
  }
  drawWiFiStatus(currentWiFiState); // Update WiFi status indicator on main screen


  delay(50);
}

void drawValue(int x, int y, const char* text, uint16_t color,bool online = false){

  int w = 120;
  int h = 30;

  // ✅ Step 1: Restore background properly (row by row)
  for (int row = 0; row < h; row++) {
    tft.pushImage(
      x,
      y + row,
      w,
      1,
      &image[(y + row) * IMG_W + x]
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
  sprintf(buffer, "%.1f%%", rmoisture);
  drawValue(75, 160, buffer, TFT_GREEN);

  // PH
  sprintf(buffer, "%.1f", rph);
  drawValue(75, 230, buffer, TFT_WHITE);


}


void handleData() {////////////////////////for field esp only////////////////////////////////////////////
  String response = "";

  // LEVEL
  if (server.hasArg("level")) {
    rlevel = server.arg("level").toInt();
    Serial.print("Level: ");
    Serial.println(rlevel);
    response += "Level OK | ";
  }

  // PUMP
  if (server.hasArg("pump")) {
    String pump = server.arg("pump");

    if (pump == "on") {
      pump_on = true;
      Serial.println("Pump ON");
      response += "Pump ON | ";
    } 
    else if (pump == "off") {
      pump_on = false;
      Serial.println("Pump OFF");
      response += "Pump OFF | ";
    }
  }

  // PH
  if (server.hasArg("ph")) {
    rph = server.arg("ph").toInt();
    Serial.print("pH: ");
    Serial.println(rph);
    response += "pH OK | ";
  }

  // MOISTURE
  if (server.hasArg("moisture")) {
    rmoisture = server.arg("moisture").toInt();
    Serial.print("Moisture: ");
    Serial.println(rmoisture);
    response += "Moisture OK | ";
  }
  //state
  if(server.hasArg("status")){
    String STATUS= server.arg("status");

    if (STATUS != "") {          // only update if not empty
    state = STATUS;           // ✅ IMPORTANT
    }
    Serial.print("status: ");
    Serial.println(state);
    response += "status OK | ";
  }

  // FINAL RESPONSE (ONLY ONCE)
  if (response == "") {
    server.send(400, "text/plain", "No valid parameters");
  } else {
    newdata = true; // Flag to indicate new data received
    server.send(200, "text/plain", response);
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
  tft.setTextFont(1);
  tft.setTextSize(2.5);
  tft.setCursor(100, 40);
  tft.print((String)field);
}

void drawWiFiStatus(bool connected) {
  int x = 140;   // adjust based on your screen width
  int y = 10;

  int w = 100;
  int h = 25;

  // 🔹 Restore background (same method you used)
  for (int row = 0; row < h; row++) {
    tft.pushImage(
      x,
      y + row,
      w,
      1,
      &image[(y + row) * IMG_W + x]
    );
  }

  // 🔹 Draw circle + text
  if (connected) {
    tft.fillCircle(x + 3, y + 10, 6, TFT_GREEN);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(1);
    tft.setCursor(x + 13, y + 3);
    tft.setTextSize(2.5);
    tft.print("Online");
  } else {
    tft.fillCircle(x + 2, y + 10, 6, TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(1);
    tft.setCursor(x + 12, y + 3);
    tft.setTextSize(2.5);
    tft.print("Offline");
  }
}   

void seperatescreen(String stateus){
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setTextSize(3);
  tft.setCursor(30, 120);
  tft.print(stateus);
}