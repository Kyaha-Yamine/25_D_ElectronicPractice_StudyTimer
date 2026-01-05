#include "Utils.h"

String twoDigit(int number) {
  if (number < 10) {
    return "0" + String(number);
  }
  return String(number);
}

String s_to_hms(int s){
  int h = s / 3600;
  int m = (s % 3600) / 60;
  int sec = s % 60;
  return String(h) + ":" + String(twoDigit(m)) + ":" + String(twoDigit(sec));
}
