#include "Network.h"

WiFiManager wm;
WiFiServer server(80);
Ticker ledTicker;
bool flag_offline = false;

void getntpTime(){
  configTime(9 * 3600, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
}

String getTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "";
  }
  String timetext = String(twoDigit(timeinfo.tm_hour)) + ":" + String(twoDigit(timeinfo.tm_min)) + ":" + String(twoDigit(timeinfo.tm_sec));
  return timetext;
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
