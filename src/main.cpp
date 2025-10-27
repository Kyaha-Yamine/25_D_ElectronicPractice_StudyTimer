#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_RST 16
#define TFT_CS 5
#define TFT_DC 17
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC,TFT_RST);
// PinAssign SDO=19,SCK=18,SDI=23,DC=17,RST=16,CS=5

// 現在のファームウェアバージョン
#define FIRMWARE_VERSION "v1.0.0"

// --- 設定項目 ---
#define GITHUB_USER "your_github_username" // GitHubのユーザー名
#define GITHUB_REPO "your_github_repo"     // GitHubのリポジトリ名


WiFiManager wm;

#define LED_1 2

Ticker ledTicker;
WiFiServer server(80);

void blinkLed() {
  digitalWrite(LED_1, !digitalRead(LED_1)); // LEDの状態を反転
}

void setuptft(){
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_WHITE);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_BLACK);
}

void setupwifi(){
  ledTicker.attach(0.5, blinkLed);
  tft.printf("Wi-Fi Connecting...");
  if(wm.autoConnect()){
    tft.printf("Connected");
    Serial.println("connected");
    // Wi-Fi接続完了後、点滅を停止してLEDを消灯
    ledTicker.detach();
    digitalWrite(LED_1, LOW);
    //checkForUpdates(); // アップデートをチェック
  }else{
    Serial.println("failed to connect");
    tft.printf("failed to connect");
    // タイムアウトまたは失敗後、点滅を停止
    ledTicker.detach();
    digitalWrite(LED_1, LOW);
    ESP.restart();
  }
  //mdns
  if (MDNS.begin("studytimer")) {
    Serial.println("MDNS responder started");
  }
  //OTA
  ArduinoOTA.begin();
  Serial.println(WiFi.localIP());
  
  server.begin();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_1, OUTPUT);
  setuptft;
  setupwifi;
}
int value = 0;
void loop() {
  // put your main code here, to run repeatedly:
	ArduinoOTA.handle(); // OTAのハンドリング

  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> to turn the LED on pin 5 on.<br>");
            client.print("Click <a href=\"/L\">here</a> to turn the LED on pin 5 off.<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(2, HIGH);               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(2, LOW);                // GET /L turns the LED off
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }

}

void checkForUpdates() {
  String url = "https://api.github.com/repos/" + String(GITHUB_USER) + "/" + String(GITHUB_REPO) + "/releases/latest";

  HTTPClient http;
  http.begin(url);
  http.addHeader("User-Agent", "ESP32-OTA-Updater");

  int httpCode = http.GET();
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        http.end();
        return;
      }

      const char* latest_version = doc["tag_name"];
      Serial.print("Current version: ");
      Serial.println(FIRMWARE_VERSION);
      Serial.print("Latest version: ");
      Serial.println(latest_version);

      // NOTE: strcmpは単純な文字列比較です。"v1.0.10"と"v1.0.2"のようなバージョンを
      // 正しく比較できない可能性があります。より堅牢な比較のためには、バージョン番号を
      // パースして数値として比較するロジックが必要です。
      if (strcmp(latest_version, FIRMWARE_VERSION) != 0) {
        Serial.println("New version available.");
        JsonArray assets = doc["assets"];
        String bin_url;
        for (JsonObject asset : assets) {
          String asset_name = asset["name"];
          if (asset_name.endsWith(".bin")) {
            bin_url = asset["browser_download_url"].as<String>();
            break;
          }
        }

        if (bin_url.length() > 0) {
          Serial.print("Firmware URL: ");
          Serial.println(bin_url);

          // httpUpdateがリダイレクトを追跡するように設定
          httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

          WiFiClientSecure client;
          // DANGER: setInsecure()はサーバー証明書を検証しないため、中間者攻撃に対して脆弱になります。
          // 可能であれば、代わりにサーバーのルートCA証明書をESP32にロードして検証してください。
          client.setInsecure();

          t_httpUpdate_return ret = httpUpdate.update(client, bin_url);

          switch (ret) {
            case HTTP_UPDATE_FAILED:
              Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
              break;
            case HTTP_UPDATE_NO_UPDATES:
              Serial.println("HTTP_UPDATE_NO_UPDATES");
              break;
            case HTTP_UPDATE_OK:
              Serial.println("HTTP_UPDATE_OK");
              break;
          }
        }
      } else {
        Serial.println("Firmware is up to date.");
      }
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
