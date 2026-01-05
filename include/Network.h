#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Ticker.h>
#include "Config.h"
#include "Display.h"
#include "Utils.h"

extern WiFiManager wm;
extern WiFiServer server;
extern bool flag_offline;
extern Ticker ledTicker;

void setupwifi();
void getntpTime();
String getTime();
void sendToGas(String* data,int count);
void configModeCallback(WiFiManager *myWiFiManager);
void blinkLed();

#endif
