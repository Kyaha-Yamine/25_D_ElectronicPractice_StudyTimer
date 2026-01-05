#include <Arduino.h>
#include <SPI.h>
#include <uTimerLib.h>
#include "Config.h"
#include "Utils.h"
#include "Display.h"
#include "Input.h"
#include "Network.h"
#include "FirmwareUpdater.h"

String mode = "Home";

void mode_stopwatch_loop_ui(String time, String paused_time){
  disp_clearMainScreen();
  disp_showTitle("Stopwatch");
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

enum TimerState { STOPPED, RUNNING, PAUSED, SETUP };
static TimerState sw_state = SETUP; 
static String sw_startHMD ="";
static String sw_stopHMD="";
static unsigned long sw_startTime = 0;
static unsigned long sw_pauseTime = 0;
static unsigned long sw_totalPausedTime = 0;
static unsigned long sw_elapsedTime = 0;
static String sw_time_showing = "";
static String sw_paused_time_showing = "";
static bool sw_needs_display_update = true;
static bool sw_pir_pause = false;


void mode_stopwatch_loop(){
  if(sw_state == SETUP){
    sw_state = STOPPED;
    sw_needs_display_update = true;
  }

  int button_press = checkButton();
  if(digitalRead(pir)==LOW){
    if(sw_state == RUNNING){
      sw_pir_pause = true;
      sw_state = PAUSED;
      sw_pauseTime = millis();
      sw_needs_display_update = true;
    }
  }else{
    if(sw_pir_pause&&sw_state == PAUSED){
        sw_pir_pause = false;
        sw_state = RUNNING;
        sw_totalPausedTime += (millis() - sw_pauseTime);
        sw_pauseTime = 0;
        sw_needs_display_update = true;
    }
  }

  if(button_press !=0){
    if (button_press == 1) { // Single press
      switch(sw_state) {
        case STOPPED:
          sw_state = RUNNING;
          if(sw_startHMD == ""){
            sw_startHMD = getTime();
          };
          sw_startTime = millis();
          sw_totalPausedTime = 0;
          sw_elapsedTime = 0;
          sw_pauseTime = 0;
          break;
        case RUNNING:
          sw_state = PAUSED;
          sw_stopHMD = getTime();
          sw_pauseTime = millis();
          break;
        case PAUSED:
          sw_state = RUNNING;
          sw_totalPausedTime += (millis() - sw_pauseTime);
          sw_pauseTime = 0;
          sw_pir_pause = false;
          break;
      }
      sw_needs_display_update = true;
    }

    if (button_press == 4) { // Long press for reset
      if(sw_state == PAUSED){
        Serial.println("reset");
        Serial.println(sw_startHMD);
        Serial.println(sw_stopHMD);
        String sendData[] = {"stopwatch",sw_startHMD,sw_stopHMD,String(sw_elapsedTime / 1000),String(sw_totalPausedTime / 1000)};
        sendToGas(sendData,5);
        sw_pir_pause = false;
        sw_startTime = 0;
        sw_pauseTime = 0;
        sw_startHMD = "";
        sw_stopHMD = "";
        sw_totalPausedTime =0;
        sw_elapsedTime = 0;
        sw_needs_display_update = true;
        sw_state = STOPPED;
      }
      if(sw_state == STOPPED){
        mode = "Menu";
        sw_state = SETUP;
      }
    }
  }

  if (sw_state == RUNNING) {
    sw_elapsedTime = millis() - sw_startTime - sw_totalPausedTime;
    disp_showfooter("・一時停止");
  }

  unsigned long current_total_paused_time = sw_totalPausedTime;
  if (sw_state == PAUSED) {
      current_total_paused_time += (millis() - sw_pauseTime);
      if(sw_pir_pause){
        disp_showfooter("【自動停止】動くと再開 -リセット");
      }else{
      disp_showfooter("・再開 -リセット");
      }
  }
  // When stopped, don't show accumulated pause time.
  if (sw_state == STOPPED) {
      current_total_paused_time = 0;
      sw_elapsedTime = 0; // Ensure elapsed time is also zero when stopped
      disp_showfooter("・スタート -Menu");
      //mode = "Menu";
  }
  
  String time_now_str = s_to_hms(sw_elapsedTime / 1000);
  String paused_time_str = "Paused: " + s_to_hms(current_total_paused_time / 1000);

  if (sw_needs_display_update || sw_time_showing != time_now_str || sw_paused_time_showing != paused_time_str) {
    mode_stopwatch_loop_ui(time_now_str, paused_time_str);
    sw_time_showing = time_now_str;
    sw_paused_time_showing = paused_time_str;
    sw_needs_display_update = false;
  }
}


void mode_settings_loop() {
  disp_clearMainScreen();
  disp_showTitle("Settings");
  String menu_items[] = {"もどる", "アップデート" ,"Wi-Fi設定","機器情報"};
  String menu_items_mode[] = {"MainMenu", "Update", "Wifi", "Info"};
  disp_showfooter("・決定 ");
  mode = disp_listMenu(menu_items_mode, menu_items, 4, "Settings");
  if (mode == "Update") {
    checkForUpdates();
  }
  if (mode == "Wifi") {
    setupwifi();
  }
  if (mode == "Info") {
    disp_clearMainScreen();
    disp_showTitle("Info");
    disp_showfooter("・もどる");
    tft.setTextSize(1);
    //firmware
    tft.setCursor(0, 20);
    tft.println("Firmware Version:");
    tft.println(FIRMWARE_VERSION);
    //wifi
    tft.println("Wi-Fi SSID:");
    tft.println(WiFi.SSID());
    tft.println("Wi-Fi IP:");
    tft.println(WiFi.localIP());
    tft.println("Wi-Fi MAC:");
    tft.println(WiFi.macAddress());
    tft.println("Wi-Fi Channel:");
    tft.println(WiFi.channel());
    tft.println("Wi-Fi RSSI:");
    tft.println(WiFi.RSSI());
    tft.println("GAS URL:");
    tft.println(GAS_URL);
    while (true) {
      int button_press = checkButton();
      if (button_press == 1) {
        mode = "Settings";
        break;
      }
    }
  }
  if (mode == "MainMenu") {
    mode = "Menu";
  }
}

void mode_menu(){
  disp_clearMainScreen();
  disp_showTitle("Menu");
  String menu_items[] = {"ストップウォッチ", "設定"};
  String menu_items_mode[] = {"Stopwatch", "Settings"};
  mode = disp_listMenu(menu_items_mode, menu_items, 2, "MainMenu");
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
  Serial.begin(115200);
  disp_clearMainScreen();
  disp_showTitle("StudyTimer");
  disp_showfooter(FIRMWARE_VERSION);
  checkForUpdates();
}

void loop() {
  if(!flag_offline){
	  ArduinoOTA.handle(); // OTAのハンドリング
  }
  if (mode == "Menu"){
    mode_menu();
  }
  else if (mode == "Stopwatch"){
    mode_stopwatch_loop();
  }else if (mode == "Timer"){
    //mode_timer_loop();
    mode = "Menu";
  }else if (mode == "Settings"){
    mode_settings_loop();
  }else if (mode == "Settings"){
    mode_settings_loop();
  }
  else{
    mode = "Menu";
  }
}
