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
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>

#define TFT_RST 16
#define TFT_CS 5
#define TFT_DC 17
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC,TFT_RST);
U8G2_FOR_ADAFRUIT_GFX u8g2;

QRCode qrcode;
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

void setuptft(){
  tft.begin();
  u8g2.begin(tft);
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.print("Study Timer");
}


void displayQRCodeHelper(String text, int x, int y, int moduleSize = 4) {
    // QRコードのデータを生成 (バージョン3, 低エラー訂正)
    // バッファサイズは qrcode_getBufferSize(3) を使用
    byte qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, text.c_str());

    int qrSize = qrcode.size; // QRコードのサイズ (バージョン3なら 29x29)
    int margin = moduleSize;  // QRコードの周囲の余白 (ピクセル)

    // 背景を白で描画 (余白込み)
    tft.fillRect(x - margin, y - margin, 
                 qrSize * moduleSize + margin * 2, 
                 qrSize * moduleSize + margin * 2, 
                 ILI9341_WHITE);

    // QRコードのモジュールを一つずつ描画
    for (int yModule = 0; yModule < qrSize; yModule++) {
        for (int xModule = 0; xModule < qrSize; xModule++) {
            if (qrcode_getModule(&qrcode, xModule, yModule)) {
                // 黒いモジュール
                tft.fillRect(x + xModule * moduleSize, y + yModule * moduleSize, 
                             moduleSize, moduleSize, 
                             ILI9341_BLACK);
            }
            // (白いモジュールは背景色で描画済みなのでスキップ)
        }
    }
}
void configModeCallback (WiFiManager *myWiFiManager) {
  String apSSID = myWiFiManager->getConfigPortalSSID();
  Serial.println("Entered config mode");
  Serial.print("Connect to AP: ");
  Serial.println(apSSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // TFTディスプレイに案内を表示
  tft.fillScreen(ILI9341_BLACK); 
  u8g2.setFontMode(1); // 透過モード
  u8g2.setFont(u8g2_font_unifont_t_japanese1); // 日本語フォント

  // --- テキスト描画 (日本語化) ---

  // タイトル
  u8g2.setForegroundColor(ILI9341_YELLOW);
  u8g2.setCursor(10, 25); // Y座標はフォントのベースライン
  u8g2.print(F("Wi-Fi設定モード")); 

  // 1. APに接続
  u8g2.setForegroundColor(ILI9341_WHITE);
  u8g2.setCursor(10, 70); 
  u8g2.print(F("1. APに接続"));

  // AP SSID
  u8g2.setForegroundColor(ILI9341_CYAN);
  u8g2.setCursor(10, 95);
  u8g2.print(apSSID); 

  // 2. ブラウザを開く
  u8g2.setForegroundColor(ILI9341_WHITE);
  u8g2.setCursor(10, 140);
  u8g2.print(F("2. ブラウザを開く"));

  // IPアドレス
  u8g2.setForegroundColor(ILI9341_GREEN);
  u8g2.setCursor(10, 165);
  u8g2.print(F("192.168.4.1"));
  
  // QRスキャン
  u8g2.setForegroundColor(ILI9341_WHITE);
  u8g2.setCursor(10, 210);
  u8g2.print(F("QRスキャン ->"));

  // --- 右側にQRコードを表示 ---

  // QRコード用の文字列 (Wi-Fi簡単接続形式)
  // "WIFI:S:<SSID>;T:<WPA|WEP|nopass>;P:<PASSWORD>;;"
  // WiFiManagerのAPはデフォルトでパスワード無し
  String qrText = "WIFI:S:" + apSSID + ";T:nopass;P:;;";

  // 描画設定
  int moduleSize = 3; // 1モジュールのサイズ (5x5ピクセル)
  // バージョン3 (29x29モジュール) の場合: 29 * 5 = 145 ピクセル幅
  
  // X座標: 320 (画面幅) - 145 (QR幅) - 10 (右マージン) = 165
  int x = 223; 
  // Y座標: (240 (画面高さ) - 145 (QR幅)) / 2 (上下中央) = 47
  int y = 143; 
  
  // QRコード描画関数を呼び出し
  displayQRCodeHelper(qrText, x, y, moduleSize); 
}
void blinkLed() {
  digitalWrite(LED_1, !digitalRead(LED_1));
}
void setupwifi(){
  wm.setAPCallback(configModeCallback);

  //ledTicker.attach(0.5, blinkLed);
  tft.println("Wi-Fi Connecting...");
  if(wm.autoConnect()){
    tft.println("Connected");
    Serial.println("connected");
    // Wi-Fi接続完了後、点滅を停止してLEDを消灯
    ledTicker.detach();
    digitalWrite(LED_1, LOW);
    //checkForUpdates(); // アップデートをチェック
  }else{
    Serial.println("failed to connect");
    tft.print("failed to connect");
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

void setup() 
{
  wm.resetSettings();

  Serial.begin(115200);
  pinMode(LED_1, OUTPUT);

  setuptft();
  setupwifi();
  //checkForUpdates();
}
int value = 0;

void loop() {
  // put your main code here, to run repeatedly:
	//ArduinoOTA.handle(); // OTAのハンドリング
}