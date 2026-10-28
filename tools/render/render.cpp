// render.cpp — Dibuja las poses de la mascota en la PC, con el mismo código que usa el ESP32
// (firmware/mascota_e6/mascota.h + poses.h) y la librería Adafruit GFX real.
// Uso: render <carpeta_salida>
//   Escribe poseNN.raw (240x240, 1 byte por píxel = índice de color del panel)
//   y poses.json con los nombres.
#include <Adafruit_GFX.h>
#include <cstdio>
#include <string>
#define MASC_SIZE 240
#include "poses.h"

class Lienzo : public Adafruit_GFX {
 public:
  uint8_t px[MASC_SIZE * MASC_SIZE];
  Lienzo() : Adafruit_GFX(MASC_SIZE, MASC_SIZE) {}
  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if (x < 0 || y < 0 || x >= MASC_SIZE || y >= MASC_SIZE) return;
    px[y * MASC_SIZE + x] = c & 0x0F;
  }
};

int main(int argc, char** argv) {
  if (argc < 2) { fprintf(stderr, "uso: %s <carpeta_salida>\n", argv[0]); return 1; }
  std::string out = argv[1];
  const int n = sizeof(POSES) / sizeof(POSES[0]);
  static Lienzo l;
  for (int i = 0; i < n; i++) {
    Mascota::draw(l, POSES[i]);
    char f[512];
    snprintf(f, sizeof f, "%s/pose%02d.raw", out.c_str(), i + 1);
    FILE* o = fopen(f, "wb");
    if (!o) { perror(f); return 1; }
    fwrite(l.px, 1, sizeof l.px, o);
    fclose(o);
  }
  FILE* j = fopen((out + "/poses.json").c_str(), "wb");
  fprintf(j, "{\"tamano\":%d,\"nombres\":[", MASC_SIZE);
  for (int i = 0; i < n; i++) {
    fprintf(j, "%s\"", i ? "," : "");
    for (const char* c = NOMBRES[i]; *c; c++) { if (*c == '"' || *c == '\\') fputc('\\', j); fputc(*c, j); }
    fputc('"', j);
  }
  fprintf(j, "]}\n");
  fclose(j);
  printf("%d poses dibujadas en %s\n", n, out.c_str());
  return 0;
}
