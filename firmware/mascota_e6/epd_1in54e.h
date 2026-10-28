// epd_1in54e.h — Driver mínimo para Waveshare 1.54" e-Paper (E), Spectra 6, 240x240.
// Secuencia de comandos tomada del ejemplo oficial de Waveshare (EPD_1in54e.cpp),
// con SPI por software y aviso por Serial si el pin BUSY no responde.
#pragma once
#include <Arduino.h>

// Pines por defecto = los de la guía oficial de Waveshare para ESP32
#ifndef EPD_SCK_PIN
#define EPD_SCK_PIN   13   // CLK
#define EPD_MOSI_PIN  14   // DIN
#define EPD_CS_PIN    15
#define EPD_DC_PIN    27
#define EPD_RST_PIN   26
#define EPD_BUSY_PIN  25
#define EPD_PWR_PIN   33   // ¡obligatorio! alimenta el panel
#endif

namespace Epd {

const int W = 240, H = 240, BYTES = W * H / 2;   // 4 bits por píxel
static bool busyError = false;
// Función que se llama mientras el panel refresca (p. ej. para atender la web)
static void (*idleHook)() = nullptr;

inline void spiByte(uint8_t b) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(EPD_MOSI_PIN, (b & 0x80) ? HIGH : LOW);
    b <<= 1;
    digitalWrite(EPD_SCK_PIN, HIGH);
    digitalWrite(EPD_SCK_PIN, LOW);
  }
}
inline void cmd(uint8_t c) {
  digitalWrite(EPD_DC_PIN, LOW);  digitalWrite(EPD_CS_PIN, LOW);
  spiByte(c);
  digitalWrite(EPD_CS_PIN, HIGH);
}
inline void data(uint8_t v) {
  digitalWrite(EPD_DC_PIN, HIGH); digitalWrite(EPD_CS_PIN, LOW);
  spiByte(v);
  digitalWrite(EPD_CS_PIN, HIGH);
}

// BUSY: LOW = ocupado, HIGH = libre
inline void waitBusy(const char* donde) {
  uint32_t t0 = millis();
  while (digitalRead(EPD_BUSY_PIN) == LOW) {
    if (millis() - t0 > 60000) {
      Serial.printf("[EPD] BUSY sigue en LOW tras 60 s (%s). Revisa BUSY, PWR, VCC y GND.\n", donde);
      busyError = true;
      return;
    }
    if (idleHook) idleHook();
    delay(10);
  }
  delay(20);
}

inline void begin() {
  pinMode(EPD_BUSY_PIN, INPUT);
  pinMode(EPD_RST_PIN, OUTPUT);
  pinMode(EPD_DC_PIN, OUTPUT);
  pinMode(EPD_CS_PIN, OUTPUT);
  pinMode(EPD_SCK_PIN, OUTPUT);
  pinMode(EPD_MOSI_PIN, OUTPUT);
  pinMode(EPD_PWR_PIN, OUTPUT);
  digitalWrite(EPD_CS_PIN, HIGH);
  digitalWrite(EPD_DC_PIN, HIGH);
  digitalWrite(EPD_SCK_PIN, LOW);
  digitalWrite(EPD_PWR_PIN, HIGH);      // enciende el panel
  delay(50);
}

inline void init() {
  digitalWrite(EPD_RST_PIN, LOW);  delay(20);
  digitalWrite(EPD_RST_PIN, HIGH); delay(20);
  waitBusy("reset");
  delay(10);
  cmd(0xE9); data(0x01);
}

inline void turnOn() {
  cmd(0x04);              waitBusy("power on");  delay(10);
  cmd(0x12); data(0x00);  delay(10); waitBusy("refresco");  // ~27 s
  cmd(0x02); data(0x00);  waitBusy("power off"); delay(20);
}

// buf: 240*240/2 bytes, píxel par en el nibble alto
inline void display(const uint8_t* buf) {
  cmd(0x10);
  for (int i = 0; i < BYTES; i++) data(buf[i]);
  turnOn();
}

inline void clear(uint8_t color) {
  cmd(0x10);
  for (int i = 0; i < BYTES; i++) data((color << 4) | color);
  turnOn();
}

inline void sleep() {
  cmd(0x07); data(0xA5);
  delay(100);
  digitalWrite(EPD_PWR_PIN, LOW);       // corta la alimentación del panel
  digitalWrite(EPD_RST_PIN, LOW);
}

}  // namespace Epd
