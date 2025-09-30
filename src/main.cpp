#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>

WiFiManager wm;

#define LED_BUILTIN 2

// put function declarations here:
int myFunction(int, int);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
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
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  
  ArduinoOTA.begin();


}

void loop() {
  // put your main code here, to run repeatedly:
	ArduinoOTA.handle(); // OTAのハンドリング
	delay(100);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}