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

// 現在のファームウェアバージョン
#define FIRMWARE_VERSION "v1.0.0"

// --- 設定項目 ---
// GitHubリポジトリの情報 (必ずご自身の情報に書き換えてください)
#define GITHUB_USER "your_github_username" // GitHubのユーザー名
#define GITHUB_REPO "your_github_repo"     // GitHubのリポジトリ名

WiFiManager wm;

#define LED_1 2

Ticker ledTicker;

void blinkLed() {
  digitalWrite(LED_1, !digitalRead(LED_1)); // LEDの状態を反転
}

// put function declarations here:
void checkForUpdates();

void setup() {
  pinMode(LED_1, OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Wi-Fi接続待機中にLEDを点滅させる
  ledTicker.attach(0.5, blinkLed);

  // Wifi接続 繋がらない場合は設定モードに移行
  if(wm.autoConnect()){
    Serial.println("connected");
    // Wi-Fi接続完了後、点滅を停止してLEDを消灯
    ledTicker.detach();
    digitalWrite(LED_1, LOW);
    checkForUpdates(); // アップデートをチェック
  }else{
    Serial.println("failed to connect");
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
}

void loop() {
  // put your main code here, to run repeatedly:
	ArduinoOTA.handle(); // OTAのハンドリング
	delay(100);
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
