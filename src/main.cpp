#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>

WiFiManager wm;

#define LED_1 2

// put function declarations here:

void setup() {
  pinMode(LED_1, OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Wifi接続 繋がらない場合は設定モードに移行
  if(wm.autoConnect()){
    Serial.println("connected");
  }else{
    Serial.println("failed to connect");
    ESP.restart();
  }
  //mdns
  if (MDNS.begin("studytimer")) {
    Serial.println("MDNS responder started");
  }
  //OTA
  ArduinoOTA.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
	ArduinoOTA.handle(); // OTAのハンドリング
	delay(100);
}
