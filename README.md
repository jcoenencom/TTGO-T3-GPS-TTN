# TTGO-T3-GPS-TTN
GPS tracker for TTN based on TTGO T3 LoRa 32 device

In order to implement this code, the following libraries need to be installed in the Arduino IDE:

- TinyGPS++
- EspSoftwareSerial
- ESPTelnet
- ElegantOTA 
- WiFi
- WebServer
- Adafruit_SSD1306


At the start of the module, the display will show the Adafruit logo, followed by text 
Hello
LILYGO T3 LoRa

Eventually when the WiFi is connected the IP address and the OTA directory

When GPS receiver is locked, the IP address, Latitude, longitude and altitude (in cm) will be displayed

The Red LED on the GPS module will flash every second indicating a GPS lock.

