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


#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "fonts/sevensegment.h"

#include "config.h"

int ADC = 35;

GxEPD2_BW < GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT > display(GxEPD2_213_BN( /*CS=5 */ SS, /*DC= */ 17, /*RST= */ 16, /*BUSY= */ 4));      // DEPG0213BN 128x250, SSD1680, TTGO T5 V2.4.1, V2.3.1

DynamicJsonDocument reading(512);

uint32_t updatePeriod = 120000000;

float vmin = 100, vmax = -100, vcurrent;
float vn,va;
float vn1 = 0;
float va1 = 0;

void setup(void) {
  Serial.begin(115200);
  Serial.println("\n\nthingspeak edisplay");

/*
  pinMode(ADC,INPUT);

  analogSetPinAttenuation(ADC,(adc_attenuation_t)3);

  for (int i=0; i< 400; i++) {
    // 3.3/4096*2
    vn = analogRead(ADC)*0.00161133;
    va = 0.969*va1 + 0.0155*vn + 0.0155*vn1;
    delay(1);
    vn1 = vn;
    va1 = va;
  }
*/

  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(50);
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  delay(50);

  getReading();
  WiFi.mode(WIFI_OFF);
  display.init(115200, true, 2, false);



  // Serial.printf("%dx%d\n",display.width(),display.height());
}

void displayConnecting() {
  display.setFullWindow();
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.firstPage();
  do {
  } while (display.nextPage());

  do {
    display.setCursor(20, 20);
    display.printf("Connecting");
  } while (display.nextPage());
}

void displayUpdate(String label, String ts, String value) {
  vcurrent = value.toFloat();
  String time = ts.substring(11, 16);
  if ((time.substring(0, 2) == "00") && (time.substring(3, 4) == "0")) {
    vmin = 100;
    vmax = -100;
  }
  if (vcurrent > vmax)
    vmax = vcurrent;
  if (vcurrent < vmin)
    vmin = vcurrent;

  Serial.printf("%s %s\n", time, value);

  display.setFullWindow();
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
  display.fillScreen(GxEPD_WHITE);
  display.firstPage();
  do {
  } while (display.nextPage());

  // display.setPartialWindow(0,0, display.width(), 20);
  display.setPartialWindow(0, 0, display.width(), display.height());
  do {
    display.setFont(&sevensegment70pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(23, 128);
    if (vcurrent > 100) {
      display.printf("%d", int (vcurrent));
    } else {
      display.printf("%0.1f", vcurrent);
    }

    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(0, 17);
    // display.printf("%s %.2fV",LABEL,va);
    display.printf(LABEL);
    display.setCursor(190, 17);
    display.printf("%s", time);
  } while (display.nextPage());
}

void getReading() {
  for (int i = 0; i < 5; i++) {
    HTTPClient http;
    http.begin(TS_URL);
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        deserializeJson(reading, payload);
        Serial.println(payload);
        // displayUpdate(LABEL, reading["created_at"], reading["field2"]);
        return;
      }
    } else {
      Serial.printf("HTTP error: %s\n", http.errorToString(httpCode).c_str());
    }
    delay(1000);                // retry delay
  }
}

void loop() {
  displayUpdate(LABEL, reading["created_at"], reading["field2"]);
  Serial.printf("sleep %d\n", millis());
  delay(50);
  esp_sleep_enable_timer_wakeup(updatePeriod);
  esp_deep_sleep_start();
}
