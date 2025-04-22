#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include "ESPTelnet.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>
//#include <Adafruit_GFX.h>

TinyGPSPlus gps;

int lstate;

IPAddress ip;
uint16_t  port = 23;

//OLED oled(128,64);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 4

Adafruit_SSD1306 display(OLED_RESET);

// The GPS module serial connection to pin 0 and 4 of GPIO

#define RXD2 0
#define TXD2 4
#define GPS_BAUD 9600
HardwareSerial ss(2);

char IPaddress[26];  // variable to store IP address of WiFi conneexion


unsigned long ota_progress_millis = 0;

WebServer server(80); // web server necessary for OTA

// Error message function, if restart = true, reboot the esp

void errorMsg(String error, bool restart = true) {
  Serial.println(error);
  if (restart) {
    Serial.println("Rebooting now...");
    delay(2000);
    ESP.restart();
    delay(2000);
  }
}

// Telnet configuration callback functions definition

ESPTelnet telnet;

void setupTelnet() {  
  // passing on functions for various telnet events
  Serial.println("Setting up telnet");
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onDisconnect(onTelnetDisconnect);
  telnet.onInputReceived(onTelnetInput);

  Serial.print("- Telnet: ");
  if (telnet.begin(port)) {
    Serial.println("running");
  } else {
    Serial.println("error.");
    errorMsg("Will reboot...");
  }
}

void onTelnetConnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" connected");
  
  telnet.println("\nWelcome " + telnet.getIP());
  telnet.println("(Use ^] + q  to disconnect.)");
}

void onTelnetDisconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" disconnected");
}

void onTelnetReconnect(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" reconnected");
}

void onTelnetConnectionAttempt(String ip) {
  Serial.print("- Telnet: ");
  Serial.print(ip);
  Serial.println(" tried to connected");
}

void onTelnetInput(String str) {
  // checks for a certain command
  if (str == "ping") {
    telnet.println("> pong"); 
    Serial.println("- Telnet: pong");
  // disconnect the client
  } else if (str == "bye") {
    telnet.println("> disconnecting you...");
    telnet.disconnectClient();
  } else if (str == "raz") {
    errorMsg("rebooting");
  } else {
    telnet.println(str);
  }
}

// OTA definition and call back

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
}

// toggle green led status

void toggleLED() {
  if (lstate == 0) {
    lstate =1;
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    lstate = 0;
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void setup() {

// initialise serial
// TTGO V 1.6ESP32 PNs 0,4 the console being on TX/RX or GPIO1/GPIO3 and cannot be used for the GPS

  Serial.begin(115200);

// set LED PIN
  pinMode(LED_BUILTIN, OUTPUT);

// OLED display 

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display.display();
  delay(2000);  // Wait for 2 seconds

  display.clearDisplay();
  display.setTextSize(1);      
  display.setTextColor(SSD1306_WHITE);  
  display.setCursor(0,0);
  display.println("Hello");
  display.println("LILYGO T3 LoRa!");
  display.display(); 									// give time to switch to the serial monitor


// setup GPS uart on ss

  Serial.println("Setup UART for GPS @ 9600 8N1");
  ss.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);

// Wifi Connexion

  Serial.println("Connecting WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin("Malperthuis","9725145239910203");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }


  ip = WiFi.localIP(),
    server.on("/", []() {
    server.send(200, "text/plain", "OTA Server on /update");
  });

// attach OTA server to Web server
  ElegantOTA.begin(&server);    // Start ElegantOTA

  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

//start web Server
  server.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(ip);

  display.clearDisplay();
  display.setCursor(0,0);
  display.println(ip);
  display.println("OTA in /update");
  display.display();

// start telnet server

  setupTelnet();
  Serial.println();
  server.begin();


}

void loop() {
  server.handleClient();
  ElegantOTA.loop();
  telnet.loop();
  String gdata;

// main loop. waiting for GPS data

  while (ss.available() > 0){
    // get the byte data from the GPS
    char gpsData = ss.read();
    gdata += String(gpsData);
    if (gps.encode(gpsData)) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println(ip);
      display.print(String(gps.date.day())+"/"+String(gps.date.month())+"/"+String(gps.date.year()));
      display.print(" - ");
      display.println(String(gps.time.hour())+":"+String(gps.time.minute())+":"+String(gps.time.second()));
      display.print(gps.location.lat(), 6);
      display.print("  ");
      display.println(gps.location.lng(), 6);
      display.print(gps.altitude.meters());
      display.print(" (m) ");
      display.print(gps.satellites.value());
      display.println(" sats");
      display.display(); 
      //display.println(gpsData);
      //toggleLED();
        telnet.print(String(gps.date.day())+"/"+String(gps.date.month())+"/"+String(gps.date.year()));
        telnet.print(" - ");
        telnet.print(String(gps.time.hour())+":"+String(gps.time.minute())+":"+String(gps.time.second()));
        telnet.print(" : ");
        telnet.print(gps.location.lat(), 6);
        telnet.print(" , ");
        telnet.print(gps.location.lng(), 6);
        telnet.print(" , ");
        telnet.print(gps.altitude.meters());
        telnet.print(" (mtr) , ");
        telnet.print(gps.satellites.value());
        telnet.println(" Sat");

      gdata="";    
    }
    
  }
}
