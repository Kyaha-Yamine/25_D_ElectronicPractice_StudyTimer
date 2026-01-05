#include "Input.h"

//--信号入力--
//ボタン 0: no press, 1: single, 2: double, 3: triple, 4: long press
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
// エンコーダー入力
// ロータリーエンコーダのチャタリング防止
int checkEncoder() {
  static int last_a_state = HIGH;
  int result = 0;

  int a_state = digitalRead(lotaryenc_A);
    if (a_state != last_a_state) {
      if (a_state == LOW) { // Falling edge on pin A
        if (digitalRead(lotaryenc_B) == HIGH) {
          result = 1; // Clockwise
        } else {
          result = -1; // Counter-clockwise
        }
      }
      last_a_state = a_state;
    }
  return result;
}
