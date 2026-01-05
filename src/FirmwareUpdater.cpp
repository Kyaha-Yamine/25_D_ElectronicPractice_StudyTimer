#include "FirmwareUpdater.h"

void checkForUpdates() {
  disp_showfooter("Checking for updates...");
  Serial.println("Checking for updates...");
  String url = "https://api.github.com/repos/" + String(GITHUB_USER) + "/" + String(GITHUB_REPO) + "/releases/latest";

  WiFiClientSecure client;
  client.setInsecure();
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
    // Workflowで生成される複数のアーティファクト(bootloader.binなど)から正しいFWを選択する
    if (asset_name == "firmware.bin") {
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

  //size_t written = Update.writeStream(httpDl.getStream());
  Stream& stream = httpDl.getStream();
  uint8_t buff[1024] = { 0 };
  size_t written = 0;
  int progress = 0;

  while((httpDl.connected() || stream.available()) && (written < contentLength)) {
      size_t size = stream.available();
      if(size) {
          int c = stream.readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          size_t w = Update.write(buff, c);
          if (w != c) {
             Serial.println("Write error!");
             break;
          }
          written += w;

          int newProgress = (written * 100) / contentLength;
          if (newProgress > progress) {
             progress = newProgress;
             Serial.printf("Progress: %d%%\r", progress);
             disp_showfooter("Progress: " + String(progress) + "%");
          }
      }
      delay(1);
  }

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
