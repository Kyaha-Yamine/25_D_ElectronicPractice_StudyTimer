#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>
#include "Config.h"
#include "Utils.h"

extern Adafruit_ILI9341 tft;
extern U8G2_FOR_ADAFRUIT_GFX u8g2;

// 定数定義 (もともとmainにあったもの)
extern int disp_def_txt_color;
extern int disp_def_bg_color;
extern int disp_def_highlight_bg_color;
extern int disp_def_highlight_txt_color;

void setuptft();
void disp_showTitle(String title ,int color = disp_def_txt_color);
void disp_showfooter(String text, int color = disp_def_txt_color);
void disp_showDateTime();
void disp_clearMainScreen();
String disp_listMenu(String menuItems_return[], String menuItems_disp[], int list_length, String title = "Menu");
void displayQRCodeHelper(String text, int x, int y, int moduleSize = 4);

#endif
