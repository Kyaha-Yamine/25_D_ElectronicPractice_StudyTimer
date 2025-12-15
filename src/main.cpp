#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>
#include <time.h>
#include <uTimerLib.h>

#define TFT_RST 16
#define TFT_CS 5
#define TFT_DC 17
#define disp_def_txt_color ILI9341_WHITE
#define disp_def_bg_color ILI9341_BLACK
#define button 34
#define lotaryenc_A 32
#define lotaryenc_B 33
#define pir 35


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC,TFT_RST);
U8G2_FOR_ADAFRUIT_GFX u8g2;

QRCode qrcode;
// PinAssign SDO=19,SCK=18,SDI=23,DC=17,RST=16,CS=5

// 現在のファームウェアバージョン
#define FIRMWARE_VERSION "v0.0.3"

// --- 設定項目 ---
#include "secrets.h"

#define GITHUB_USER "Kyaha-Yamine" // GitHubのユーザー名
#define GITHUB_REPO "25_D_ElectronicPractice_StudyTimer"     // GitHubのリポジトリ名

#ifndef GITHUB_TOKEN
#define GITHUB_TOKEN "" // secrets.h がない場合や定義されていない場合のフォールバック
#endif

const char* GAS_URL ="https://script.google.com/macros/s/AKfycbyL0FWlo3UmhNErxdD16dvFVqLP6x7mkSsrCrEQls978QsYkRuEGcYylM8zdMaKJ_jA/exec";
WiFiManager wm;
bool flag_offline = false;
#define LED_1 2

Ticker ledTicker;
WiFiServer server(80);

String mode = "Home";

void getntpTime(){
  configTime(9 * 3600, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
}

String twoDigit(int number) {
  if (number < 10) {
    return "0" + String(number);
  }
  return String(number);
}

String datetext_showing;
void disp_showDateTime(){
  ArduinoOTA.handle(); // OTAのハンドリング
  struct tm timeinfo;  
  if(!getLocalTime(&timeinfo)){
    datetext_showing = "";
    return;
  }
  String datetext = String(twoDigit(timeinfo.tm_mon + 1)) + "/" + String(twoDigit(timeinfo.tm_mday)) + " " + String(twoDigit(timeinfo.tm_hour)) + ":" + String(twoDigit(timeinfo.tm_min));
  if(datetext != datetext_showing){
  tft.fillRect(230,0,90,15,disp_def_bg_color);
  u8g2.setCursor(230, 15);
  u8g2.setForegroundColor(disp_def_txt_color);
  tft.setTextColor(disp_def_txt_color);
  u8g2.print(datetext);
  datetext_showing = datetext;
  }
}

String s_to_hms(int s){
  int h = s / 3600;
  int m = (s % 3600) / 60;
  int sec = s % 60;
  return String(h) + ":" + String(twoDigit(m)) + ":" + String(twoDigit(sec));
}

String getTime(){
  struct tm timeinfo;  
  if(!getLocalTime(&timeinfo)){
    return "";
  }
  String timetext = String(twoDigit(timeinfo.tm_hour)) + ":" + String(twoDigit(timeinfo.tm_min)) + ":" + String(twoDigit(timeinfo.tm_sec));
  return timetext;
}

String titletext_showing;
void disp_showTitle(String title ,int color = disp_def_txt_color){
  if(titletext_showing == title){
    return;
  }
  titletext_showing = title;
  tft.fillRect(0,0,229,19,disp_def_bg_color); 
  u8g2.setCursor(0, 15);
  u8g2.setForegroundColor(color);
  tft.setTextColor(color);
  tft.drawLine(0,20,320,20,color);
  u8g2.print(title);
}

String footertext_showing;
void disp_showfooter(String text, int color = disp_def_txt_color){
  if(footertext_showing == text){
    return;
  }
  footertext_showing = text;
  tft.fillRect(0,224,320,86,disp_def_bg_color); 
  u8g2.setCursor(0, 238);
  u8g2.setForegroundColor(color);
  tft.setTextColor(color);
  tft.drawLine(0,223,320,223,color);
  u8g2.print(text);

}

void disp_clearMainScreen(){
  tft.fillRect(0,21,320,201,disp_def_bg_color);
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

void setuptft(){
  tft.begin();
  u8g2.begin(tft);
  tft.setRotation(3);
  tft.fillScreen(disp_def_bg_color);
  u8g2.setCursor(0, 10);
  tft.setTextColor(disp_def_txt_color);
  u8g2.setFontMode(1); // 透過モード
  u8g2.setFont(u8g2_font_unifont_t_japanese1); // 日本語フォント
  disp_showTitle("StudyTimer");
  disp_showfooter(FIRMWARE_VERSION);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  disp_showfooter("");
  String apSSID = myWiFiManager->getConfigPortalSSID();
  Serial.println("Entered config mode");
  Serial.print("Connect to AP: ");
  Serial.println(apSSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // TFTディスプレイに案内を表示
  disp_clearMainScreen();
  disp_showTitle("Wi-Fi接続設定");

  // 1. APに接続
  u8g2.setForegroundColor(disp_def_txt_color);
  u8g2.setCursor(10, 35); 
  u8g2.print(F("1. APに接続"));

  // AP SSID
  u8g2.setForegroundColor(ILI9341_CYAN);
  u8g2.setCursor(10, 50);
  u8g2.print(apSSID); 

  // 2. ブラウザを開く
  u8g2.setForegroundColor(ILI9341_WHITE);
  u8g2.setCursor(10, 75);
  u8g2.print(F("2. ブラウザを開く"));

  // IPアドレス
  u8g2.setForegroundColor(ILI9341_GREEN);
  u8g2.setCursor(10, 90);
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
  int y = 133; 
  
  // QRコード描画関数を呼び出し
  displayQRCodeHelper(qrText, x, y, moduleSize); 
}
void blinkLed() {
  digitalWrite(LED_1, !digitalRead(LED_1));
}
void setupwifi(){
  disp_showfooter("Connecting Wi-Fi...");
  wm.setAPCallback(configModeCallback);

  //ledTicker.attach(0.5, blinkLed);
  if(wm.autoConnect()){
    Serial.println("Wi-Fi connected!");
    disp_showfooter("Wi-Fi connected!");
    // Wi-Fi接続完了後、点滅を停止してLEDを消灯
    ledTicker.detach();
    digitalWrite(LED_1, LOW);
    //checkForUpdates(); // アップデートをチェック
  }else{
    Serial.println("failed to connect");
    disp_showfooter("failed to connect");
    // タイムアウトまたは失敗後、点滅を停止
    ledTicker.detach();
    digitalWrite(LED_1, LOW);
    ESP.restart();
  }
  //mdns
  if (MDNS.begin("studytimer")) {
    Serial.println("MDNS responder started");
    disp_showfooter("MDNS responder started");
  }
  //OTA
    ArduinoOTA
    .onStart([]() {
      disp_clearMainScreen();
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      disp_showfooter(type);
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
      disp_showTitle("OTA Update "+ type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
      disp_showfooter("End");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      disp_showfooter(String("Progress:" + (progress / (total / 100))) + "%");
    })
    .onError([](ota_error_t error) {
      String msg;
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) msg="Auth Failed";
      else if (error == OTA_BEGIN_ERROR) msg="Begin Failed";
      else if (error == OTA_CONNECT_ERROR) msg="Connect Failed";
      else if (error == OTA_RECEIVE_ERROR) msg="Receive Failed";
      else if (error == OTA_END_ERROR) msg="End Failed";
      else msg="Unknown Error";
      Serial.println(msg);
      disp_showfooter("Error[%u]: "+msg, ILI9341_RED);
      delay(1000);
      
    });
  ArduinoOTA.begin();


  Serial.println(WiFi.localIP());
  disp_showfooter(WiFi.localIP().toString());
  server.begin();
}


// 0: no press, 1: single, 2: double, 3: triple, 4: long press
int checkButton() {
    static int lastState = HIGH;
    static int currentState = HIGH;
    static unsigned long lastDebounceTime = 0;
    static unsigned long pressTime = 0;
    static unsigned long releaseTime = 0;
    static int pressCount = 0;
    static bool longPressSent = false;

    int reading = digitalRead(button);
    int result = 0; // 0 = no press

    // Debounce
    if (reading != lastState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > 50) {
        // If the button state has changed
        if (reading != currentState) {
            currentState = reading;

            if (currentState == LOW) {
                // Button was pressed
                pressTime = millis();
                longPressSent = false;
            } else {
                // Button was released
                releaseTime = millis();
                if (!longPressSent) {
                    pressCount++;
                }
            }
        }
    }

    lastState = reading;

    // Check for long press
    if (currentState == LOW && !longPressSent && (millis() - pressTime > 1000)) {
        result = 4; // Long press
        longPressSent = true;
        pressCount = 0; // Reset counter on long press
    }

    // Check for multi-press
    if (pressCount > 0 && (millis() - releaseTime > 300)) {
        if (pressCount == 1) {
            result = 1; // Single press
        } else if (pressCount == 2) {
            result = 2; // Double press
        } else if (pressCount >= 3) {
            result = 3; // Triple press
        }
        pressCount = 0;
    }

    return result;
}

void sendToGas(String* data,int count){
  if(flag_offline){
    return;
  }
  if(WiFi.status()== WL_CONNECTED){
    disp_showfooter("データを送信しています...");
    String clientid = WiFi.macAddress();
    String url = String(GAS_URL) + "?clientid=" + clientid;
    for(int i = 0;i < count;i++){
      url += "&data" + String(i) + "=" + String(data[i]);
    }
    Serial.println(url);
    
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
      disp_showfooter("送信しました！", ILI9341_GREEN);
    } else {
      Serial.println("Error on HTTP request");
      disp_showfooter("送信失敗", ILI9341_RED);
    }
    http.end();
  }
}

void timer_stopwatch_ui(String time, String paused_time){
  disp_clearMainScreen();
  disp_showTitle("Timer");
  tft.setTextSize(4);
  tft.setTextColor(disp_def_txt_color);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(time, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((320 - w) / 2, 50);
  tft.print(time);

  // 停止時間の描画
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_YELLOW); // 色を黄色に変更
  tft.getTextBounds(paused_time, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((320 - w) / 2, 100); // Y座標を下に
  tft.print(paused_time);
}

enum TimerState { STOPPED, RUNNING, PAUSED };
static TimerState timer_state = STOPPED; 
static String startHMD ="";
static String stopHMD="";
static unsigned long startTime = 0;
static unsigned long pauseTime = 0;
static unsigned long totalPausedTime = 0;
static unsigned long elapsedTime = 0;
static String time_showing = "";
static String paused_time_showing = "";
static bool needs_display_update = true;
static bool pir_pause = false;


void timer_stopwatch(){
  mode = "Stopwatch";
  //struct tm timeinfo;  
  int button_press = checkButton();
  if(digitalRead(pir)==LOW){
    if(timer_state == RUNNING){
      pir_pause = true;
      timer_state = PAUSED;
      pauseTime = millis();
      needs_display_update = true;
    }
  }else{
    if(pir_pause&&timer_state == PAUSED){
        pir_pause = false;
        timer_state = RUNNING;
        totalPausedTime += (millis() - pauseTime);
        pauseTime = 0;
        needs_display_update = true;
    }
  }


  if (button_press == 1) { // Single press
    switch(timer_state) {
      case STOPPED:
        timer_state = RUNNING;
        if(startHMD == ""){
          startHMD = getTime();
        };
        startTime = millis();
        totalPausedTime = 0;
        elapsedTime = 0;
        pauseTime = 0;
        break;
      case RUNNING:
        timer_state = PAUSED;
        stopHMD = getTime();
        pauseTime = millis();
        break;
      case PAUSED:
        timer_state = RUNNING;
        totalPausedTime += (millis() - pauseTime);
        pauseTime = 0;
        pir_pause = false;
        break;
    }
    needs_display_update = true;
  }

  if (button_press == 4) { // Long press for reset
    if(timer_state == PAUSED){
    timer_state = STOPPED;
    
    Serial.println("reset");
    Serial.println(startHMD);
    Serial.println(stopHMD);
    String sendData[] = {"stopwatch",startHMD,stopHMD,String(elapsedTime / 1000),String(totalPausedTime / 1000)};
    sendToGas(sendData,5);
    pir_pause = false;
    startTime = 0;
    pauseTime = 0;
    startHMD = "";
    stopHMD = "";
    totalPausedTime =0;
    elapsedTime = 0;
    needs_display_update = true;
    }

  }

  if (timer_state == RUNNING) {
    elapsedTime = millis() - startTime - totalPausedTime;
    disp_showfooter("・一時停止");
  }

  unsigned long current_total_paused_time = totalPausedTime;
  if (timer_state == PAUSED) {
      current_total_paused_time += (millis() - pauseTime);
      if(pir_pause){
        disp_showfooter("【自動停止】動くと再開 -リセット");
      }else{
      disp_showfooter("・再開 -リセット");
      }
  }
  // When stopped, don't show accumulated pause time.
  if (timer_state == STOPPED) {
      current_total_paused_time = 0;
      elapsedTime = 0; // Ensure elapsed time is also zero when stopped
      disp_showfooter("・スタート");
  }
  

  String time_now_str = s_to_hms(elapsedTime / 1000);
  String paused_time_str = "Paused: " + s_to_hms(current_total_paused_time / 1000);

  if (needs_display_update || time_showing != time_now_str || paused_time_showing != paused_time_str) {
    timer_stopwatch_ui(time_now_str, paused_time_str);
    time_showing = time_now_str;
    paused_time_showing = paused_time_str;
    needs_display_update = false;
  }

}
  
void checkForUpdates() {
  String url = "https://api.github.com/repos/" + String(GITHUB_USER) + "/" + String(GITHUB_REPO) + "/releases/latest";

  WiFiClientSecure client;
  client.setInsecure(); // GitHubの証明書検証をスキップ (本番運用ではルート証明書推奨)

  HTTPClient http;
  http.begin(client, url);
  http.addHeader("User-Agent", "ESP32-OTA-Updater");
  if (String(GITHUB_TOKEN) != "") {
    http.addHeader("Authorization", "token " + String(GITHUB_TOKEN));
  }

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return;
  }

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

  if (strcmp(latest_version, FIRMWARE_VERSION) == 0) {
    Serial.println("Firmware is up to date.");
    http.end();
    return;
  }

  Serial.println("New version available.");
  JsonArray assets = doc["assets"];
  String asset_url = "";

  for (JsonObject asset : assets) {
    String asset_name = asset["name"];
    if (asset_name.endsWith(".bin")) {
       // プライベートリポジトリ対応のため、url (API URL) を使用
       asset_url = asset["url"].as<String>();
       break;
    }
  }

  http.end(); // リリース情報取得用の接続を閉じる

  if (asset_url.length() == 0) {
    Serial.println("No suitable binary found in release.");
    return;
  }

  Serial.print("Asset URL: ");
  Serial.println(asset_url);

  // 1. アセットのダウンロードURL（リダイレクト先）を取得
  HTTPClient httpAsset;
  httpAsset.begin(client, asset_url);
  httpAsset.addHeader("User-Agent", "ESP32-OTA-Updater");
  httpAsset.addHeader("Accept", "application/octet-stream");
  if (String(GITHUB_TOKEN) != "") {
    httpAsset.addHeader("Authorization", "token " + String(GITHUB_TOKEN));
  }

  const char * headerKeys[] = {"Location"};
  httpAsset.collectHeaders(headerKeys, 1);
  httpAsset.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS); // 自動リダイレクト無効

  int assetHttpCode = httpAsset.GET();
  String downloadUrl = "";

  if (assetHttpCode == 302) {
     downloadUrl = httpAsset.header("Location");
     Serial.print("Redirect to: ");
     Serial.println(downloadUrl);
  } else if (assetHttpCode == 200) {
     // リダイレクトされずに直接返ってきた場合（稀だが）
     downloadUrl = asset_url;
  } else {
     Serial.printf("Failed to get download URL, error: %d\n", assetHttpCode);
     httpAsset.end();
     return;
  }
  httpAsset.end();

  if (downloadUrl.length() == 0) {
     return;
  }

  // 2. バイナリのダウンロードと書き込み (Authヘッダーなし)
  HTTPClient httpDl;
  httpDl.begin(client, downloadUrl);
  httpDl.addHeader("User-Agent", "ESP32-OTA-Updater");
  // AWS S3などの署名付きURLの場合、Authorizationヘッダーをつけてはいけない
  httpDl.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int dlHttpCode = httpDl.GET();
  if (dlHttpCode != HTTP_CODE_OK) {
    Serial.printf("Download failed, error: %s\n", httpDl.errorToString(dlHttpCode).c_str());
    httpDl.end();
    return;
  }

  int contentLength = httpDl.getSize();
  Serial.printf("Firmware size: %d\n", contentLength);

  if (contentLength <= 0) {
     Serial.println("Content-Length is 0 or invalid");
     httpDl.end();
     return;
  }

  bool canBegin = Update.begin(contentLength);
  if (!canBegin) {
      Serial.println("Not enough space to begin OTA");
      disp_showfooter("Not enough space", ILI9341_RED);
      httpDl.end();
      return;
  }

  Serial.println("Begin OTA. This may take 2 - 5 mins...");
  disp_showTitle("Updating...");
  disp_showfooter("Downloading...", ILI9341_YELLOW);

  size_t written = Update.writeStream(httpDl.getStream());

  if (written == contentLength) {
     Serial.println("Written : " + String(written) + " successfully");
  } else {
     Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
  }

  if (Update.end()) {
     Serial.println("OTA done!");
     if (Update.isFinished()) {
        Serial.println("Update successfully completed. Rebooting.");
        disp_showfooter("Update Success! Rebooting...", ILI9341_GREEN);
        delay(2000);
        ESP.restart();
     } else {
        Serial.println("Update not finished? Something went wrong!");
        disp_showfooter("Update Failed (Unknown)", ILI9341_RED);
     }
  } else {
     Serial.println("Error Occurred. Error #: " + String(Update.getError()));
     disp_showfooter("Update Error: " + String(Update.getError()), ILI9341_RED);
  }

  httpDl.end();
}



void setup() 
{
  pinMode(pir, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(lotaryenc_A, INPUT_PULLUP);
  pinMode(lotaryenc_B, INPUT_PULLUP);
  setuptft();
  if(digitalRead(button)==LOW){
    flag_offline = true;
    disp_showfooter("Offline Mode");
    delay(1000);
  }
  if(!flag_offline){
    setupwifi();
    getntpTime();
  }
  
  TimerLib.setInterval_us(disp_showDateTime, 1000000);
  //wm.resetSettings();
  Serial.begin(115200);
  disp_clearMainScreen();
  disp_showTitle("StudyTimer");
  disp_showfooter(FIRMWARE_VERSION);
  //checkForUpdates();
}
int value = 0;

void loop() {
  // put your main code here, to run repeatedly:
  if(!flag_offline){
	  ArduinoOTA.handle(); // OTAのハンドリング
  }
  timer_stopwatch();
  
  static int last_a_state = HIGH;
  int a_state = digitalRead(lotaryenc_A);
  if (a_state != last_a_state) {
    if (a_state == LOW) { // Falling edge on pin A
      if (digitalRead(lotaryenc_B) == HIGH) {
        Serial.println("Rotary Encoder: Clockwise");
      } else {
        Serial.println("Rotary Encoder: Counter-Clockwise");
      }
    }
    last_a_state = a_state;
  }
  Serial.println(digitalRead(pir));
  

}

