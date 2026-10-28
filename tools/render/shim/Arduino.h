// Arduino.h mínimo para compilar Adafruit GFX y el dibujo de la mascota en una PC.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
typedef bool boolean;
typedef uint8_t byte;
#ifndef PROGMEM
#define PROGMEM
#endif
#define pgm_read_byte(a)  (*(const uint8_t*)(a))
#define pgm_read_word(a)  (*(const uint16_t*)(a))
#define pgm_read_dword(a) (*(const uint32_t*)(a))
class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper*>(s))
#ifndef PI
#define PI 3.14159265358979323846
#endif
#define radians(d) ((d) * PI / 180.0)
#define degrees(r) ((r) * 180.0 / PI)
using std::min; using std::max;
#include <string>
class String {
 public:
  std::string s;
  String(const char* c = "") : s(c) {}
  unsigned int length() const { return s.size(); }
  const char* c_str() const { return s.c_str(); }
};
#include "Print.h"
