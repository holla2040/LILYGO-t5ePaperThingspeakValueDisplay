/* refs
  https://github.com/ZinggJM/GxEPD2
    Adafruit_GFX_Library
  https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
    used fontconvert
  
  250x128
*/

#include <WiFiClient.h>
#include <WiFi.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "fonts/sevensegment.h"

#include "config.h"

int ADC = 34;

GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(GxEPD2_213_BN(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // DEPG0213BN 128x250, SSD1680, TTGO T5 V2.4.1, V2.3.1


DynamicJsonDocument reading(512);

uint32_t now, updatePeriod = 60000;

float vmin = 100,vmax = -100, vcurrent;

void wmCallback(WiFiManager *wm){
  display.firstPage();
  do{
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(0,20);
    display.println("Set AP, connect to");
    display.println("ThingSpeak eDisplay");
    display.println("Browse to");
    display.println("   192.168.4.1");

  }while (display.nextPage());
}

void setup(void) {
    Serial.begin(115200);
    Serial.println("\n\nthingspeak display");

  display.init(115200, true, 2, false);
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);

  do{
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(0,20);
    display.print("Connecting");   //print the text
  }while (display.nextPage());

    WiFiManager wifiManager;
    wifiManager.setAPCallback(wmCallback);

    // wifiManager.resetSettings();

    wifiManager.autoConnect("ThingSpeak eDisplay");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());


    ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to 
      // unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();

  display.firstPage();
  do{
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(0,127);
    display.print(WiFi.SSID());
    display.print(" ");
    display.print(WiFi.localIP());
  }while (display.nextPage());

    update();
    Serial.printf("%dx%d",display.width(),display.height());
}

void displayUpdate(String label, String ts, String value) {
    vcurrent = value.toFloat();
    String time = ts.substring(11, 16);
    if ((time.substring(0,2) == "00") && (time.substring(3,4) == "0")) {
      vmin = 100;
      vmax = -100;
    }
    if (vcurrent > vmax) vmax = vcurrent;
    if (vcurrent < vmin) vmin = vcurrent;

    Serial.printf("%s %s\n",time,value);


    display.setFont(&sevensegment70pt7b);
    display.setPartialWindow(0, 20, 250, 188);
    display.firstPage();
    do{
      display.fillScreen(GxEPD_WHITE);
      display.setCursor(23,128);
      if (vcurrent > 100) {
        display.printf("%d", int(vcurrent));
      } else {
        display.printf("%0.1f", vcurrent);
      }
    }while(display.nextPage());

    do{
      display.setFont(&FreeMonoBold9pt7b);
      display.setPartialWindow(0, 0, 250, 17);
      display.fillScreen(GxEPD_WHITE);
      display.setCursor(0, 16);
      display.print(LABEL);
      display.setCursor(190, 17);
      display.printf("%s",time);
    }while(display.nextPage());


}

void update() {
    HTTPClient http;
    http.begin(TS_URL);
    int httpCode = http.GET();
    if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            deserializeJson(reading, payload);
            Serial.println(payload);
            displayUpdate(LABEL, reading["created_at"], reading["field2"]);
        }
    } else {
        Serial.printf("HTTP error: %s\n", http.errorToString(httpCode).c_str());
    }
}

void loop() {
    if ((millis() - now) > updatePeriod) {
        update();
        now = millis();
    }
    ArduinoOTA.handle();
}
