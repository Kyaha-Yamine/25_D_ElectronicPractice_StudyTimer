#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

#define LED_BUILTIN 2

// put function declarations here:
void connectWiFi(const char* ssid, const char* pass);
// put function declarations here:
int myFunction(int, int);
const char* w_ssid = "kyahayamine";
const char* w_pass = "kyahayamine";

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(115200);
  connectWiFi(w_ssid,w_pass);
  ArduinoOTA.begin();
  //mdns
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

}

void loop() {
  // put your main code here, to run repeatedly:
	ArduinoOTA.handle(); // OTAのハンドリング

	delay(100);

	if ( WiFi.status() == WL_DISCONNECTED ) {
		connectWiFi(w_ssid,w_pass); 
	}
}

void connectWiFi(const char* ssid, const char* pass){
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    
  }
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("");
  Serial.println("WiFi connected:"+WiFi.SSID());
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}