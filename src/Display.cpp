#include "Display.h"
#include "Input.h" // checkButton, checkEncoderが必要 (循環依存に注意、Input.hはDisplayに依存しないはず)
#include <ArduinoOTA.h>

// グローバルオブジェクトの実体定義
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
U8G2_FOR_ADAFRUIT_GFX u8g2;
QRCode qrcode;

int disp_def_txt_color = ILI9341_WHITE;
int disp_def_bg_color = ILI9341_BLACK;
int disp_def_highlight_bg_color = ILI9341_WHITE;
int disp_def_highlight_txt_color = ILI9341_BLACK;

// 内部状態変数
static String titletext_showing;
static String footertext_showing;
static String datetext_showing;

void setuptft(){
  tft.begin();
  u8g2.begin(tft);
  tft.setRotation(3);
  tft.fillScreen(disp_def_bg_color);
  u8g2.setCursor(0, 10);
  tft.setTextColor(disp_def_txt_color);
  u8g2.setFontMode(1); // 透過モード
  u8g2.setFont(u8g2_font_unifont_t_japanese1); // 日本語フォント
  disp_showTitle("StudyTimer "+String(FIRMWARE_VERSION));
}

void disp_showTitle(String title ,int color){
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

void disp_showfooter(String text, int color){
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

void disp_showDateTime(){
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

void disp_clearMainScreen(){
  tft.fillRect(0,21,320,202,disp_def_bg_color);
}

String disp_listMenu(String menuItems_return[], String menuItems_disp[], int list_length, String title) {
  //setup
  disp_clearMainScreen();
  disp_showTitle(title);
  disp_showfooter("・決定");
  tft.setTextSize(2);
  int selected = 0;
  int show_start = 0;
  int bg_highlight_color = ILI9341_WHITE;
  int txt_highlight_color = ILI9341_BLACK;
  bool needs_redraw = true;
  //loop
  while (1)
  {
    if(needs_redraw){
      // listの描画
      for (int i = 0; i < list_length; i++) {
        if (i >= show_start && i < show_start + 10) {
          if (i == selected) {
            tft.fillRect(0, 21 + (i - show_start) * 20, 320, 20, bg_highlight_color);
            u8g2.setForegroundColor(txt_highlight_color);
            u8g2.setBackgroundColor(bg_highlight_color);
            tft.setTextColor(txt_highlight_color);
            u8g2.setCursor(20, 21 + (i - show_start) * 20 +15);
          } else {
            tft.fillRect(0, 21 + (i - show_start) * 20, 320, 20, disp_def_bg_color);
            u8g2.setForegroundColor(disp_def_txt_color);
            u8g2.setBackgroundColor(disp_def_bg_color);
            tft.setTextColor(disp_def_txt_color);
            u8g2.setCursor(20, 21 + (i - show_start) * 20 +15);

          }
          u8g2.print(menuItems_disp[i]);
        }
        if(list_length > 10){
          // スクロールバーの描画
          int scrollbarHeight = 201; // 222 - 21
          int scrollbarWidth = 10;
          int scrollbarX = 310;
          int scrollbarY = 21;
          tft.fillRect(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight, ILI9341_BLACK);

          // スクロールバーの位置を計算
          float scrollRatio = (float)show_start / (list_length - 10);
          int scrollBarHeight = max(20, (int)(scrollbarHeight * (10.0 / list_length)));
          int scrollBarY = scrollbarY + (scrollbarHeight - scrollBarHeight) * scrollRatio;

          tft.fillRect(scrollbarX, scrollBarY, scrollbarWidth, scrollBarHeight, ILI9341_WHITE);
        }
      u8g2.setForegroundColor(disp_def_txt_color);
      u8g2.setBackgroundColor(disp_def_bg_color);
      tft.setTextColor(disp_def_txt_color);
      needs_redraw = false;
      }
    }
      int button_press = checkButton();
      if (button_press != 0) {
        if (button_press == 1) { // Single press
          break;
        }
      }
      int encoder_change = checkEncoder();
      if (encoder_change != 0) {
        selected += encoder_change;
        if (selected < 0) {
          selected = 0;
        } else if (selected >= list_length) {
          selected = list_length - 1;
        }

        if (selected >= show_start + 10) {
          show_start = selected - 9;
        } else if (selected < show_start) {
          show_start = selected;
        }
        needs_redraw = true;
      }
    }

  return menuItems_return[selected];
}

String disp_yesno_form(String title, String message) {
  //setup
  disp_clearMainScreen();
  disp_showTitle(title);
  disp_showfooter("・決定");
  tft.setTextSize(2);
  int selected = 0;
  int bg_highlight_color = ILI9341_WHITE;
  int txt_highlight_color = ILI9341_BLACK;
  bool needs_redraw = true;
  String menuItems_disp[] = {"はい", "いいえ"};
  String menuItems_return[] = {"yes", "no"};
  //loop
  while (1)
  {
    if(needs_redraw){
      // listの描画
      for (int i = 0; i < 2; i++) {
          if (i == selected) {
            tft.fillRect(0, 201 + i * 20, 320, 20, bg_highlight_color);
            u8g2.setForegroundColor(txt_highlight_color);
            u8g2.setBackgroundColor(bg_highlight_color);
            tft.setTextColor(txt_highlight_color);
          } else {
            tft.fillRect(0, 201 + i * 20, 320, 20, disp_def_bg_color);
            u8g2.setForegroundColor(disp_def_txt_color);
            u8g2.setBackgroundColor(disp_def_bg_color);
            tft.setTextColor(disp_def_txt_color);  
          }
          u8g2.setCursor(20, 201 + i * 20 +15);
          u8g2.print(menuItems_disp[i]);
      u8g2.setForegroundColor(disp_def_txt_color);
      u8g2.setBackgroundColor(disp_def_bg_color);
      tft.setTextColor(disp_def_txt_color);
      needs_redraw = false;
      }
    }
      int button_press = checkButton();
      if (button_press != 0) {
        if (button_press == 1) { // Single press
          break;
        }
      }
      int encoder_change = checkEncoder();
      if (encoder_change != 0) {
        selected += encoder_change;
        if (selected < 0) {
          selected = 0;
        }
        needs_redraw = true;
      }
    }

  return menuItems_return[selected];
}

void displayQRCodeHelper(String text, int x, int y, int moduleSize) {
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
