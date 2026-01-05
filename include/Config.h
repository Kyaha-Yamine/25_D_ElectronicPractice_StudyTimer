#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- ハードウェアピン設定 ---
#define TFT_RST 16
#define TFT_CS 5
#define TFT_DC 17

#define button 34
#define lotaryenc_A 32
#define lotaryenc_B 33
#define pir 35
#define LED_1 2

// --- ファームウェアバージョン ---
#define FIRMWARE_VERSION "v1.0.0"

// --- GitHub設定 ---
#define GITHUB_USER "Kyaha-Yamine"
#define GITHUB_REPO "25_D_ElectronicPractice_StudyTimer"
#define GITHUB_TOKEN ""

// --- Google Apps Script設定 ---
const char* const GAS_URL ="https://script.google.com/macros/s/AKfycbyL0FWlo3UmhNErxdD16dvFVqLP6x7mkSsrCrEQls978QsYkRuEGcYylM8zdMaKJ_jA/exec";

#endif
