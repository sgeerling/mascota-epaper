// mascota.h — Mascota "Pie de Metro" dibujada con primitivas Adafruit_GFX.
// Pensada para la Waveshare 1.54" e-Paper (E) Spectra 6, 240x240 px.
#pragma once
#include <math.h>
#include <stdint.h>

#ifndef MASC_SIZE
#define MASC_SIZE 240          // lado de la pantalla en px
#endif
// Colores (valores del controlador Spectra 6 de Waveshare)
#ifndef MASC_BLACK
#define MASC_BLACK  0x0
#define MASC_WHITE  0x1
#define MASC_YELLOW 0x2
#define MASC_RED    0x3
#define MASC_BLUE   0x5
#define MASC_GREEN  0x6
#endif
#ifndef MASC_LINEA
#define MASC_LINEA MASC_BLACK  // contorno del pie de metro
#endif
#ifndef MASC_RASGOS
#define MASC_RASGOS MASC_BLUE  // ojos, cejas, brazos y manos (azul marino en la imagen)
#endif

// Adornos a color que acompañan algunas poses
enum Extra : uint8_t {
  EX_NADA = 0,
  EX_CORAZONES,   // rojo
  EX_ENOJO,       // marca de enojo roja
  EX_LAGRIMAS,    // azul
  EX_PREGUNTA,    // "?" azul
  EX_NOTAS,       // notas musicales verde/azul
  EX_ESTRELLAS,   // amarillo
  EX_SUDOR,       // gota azul
  EX_IDEA,        // ampolleta amarilla
  EX_ZZZ,         // "Zzz" azul
  EX_GAFAS,       // lentes de sol negros
  EX_MAREADO,     // ojos en espiral
  EX_TUERCA,      // mide una tuerca amarilla con las mandíbulas
  EX_ALERTA,      // "!" rojo
  EX_CAFE,        // taza roja humeante
  EX_SOL,         // sol amarillo
  EX_LLUVIA,      // nube con lluvia
  EX_ARCOIRIS,    // arcoíris
};

// Una pose = una expresión de la mascota
struct Pose {
  int8_t  lx, rx, py;   // pupilas: x ojo izq, x ojo der, y de ambas
  uint8_t blink;        // bit0 = ojo izq cerrado, bit1 = ojo der cerrado (3 = ambos)
  int8_t  brow;         // cejas levantadas
  int8_t  jaw;          // corredera: negativo cierra, positivo abre
  float   waveL, waveR; // ángulo de cada mano (radianes)
  int8_t  tilt;         // inclinación de cejas: + enojado, - triste (opcional)
  uint8_t extra;        // adorno, ver enum Extra (opcional)
};

namespace Mascota {

const float SC = MASC_SIZE / 200.0f;          // diseño base a 200 px
const float K = 0.104f * SC, OX = 5.7f * SC, OY = -4.0f * SC;
inline int X(float x) { return (int)lroundf(OX + x * K); }   // coords de la imagen original 1920x1920
inline int Y(float y) { return (int)lroundf(OY + y * K); }
inline int S(float v) { return (int)lroundf(v * SC); }        // escala de medidas en px
const uint16_t B = MASC_LINEA, W = MASC_WHITE, F = MASC_RASGOS;
const float DEG = 0.0174533f;

template <class G> void dot(G& d, int x, int y, int r, uint16_t c) {
  if (r <= 0) d.drawPixel(x, y, c); else d.fillCircle(x, y, r, c);
}

template <class G> void thick(G& d, float x0, float y0, float x1, float y1, int r, uint16_t c) {
  float dx = x1 - x0, dy = y1 - y0;
  int n = (int)ceilf(fmaxf(fabsf(dx), fabsf(dy)));
  if (n < 1) n = 1;
  for (int i = 0; i <= n; i++) {
    float t = (float)i / n;
    dot(d, (int)lroundf(x0 + dx * t), (int)lroundf(y0 + dy * t), r, c);
  }
}

template <class G> void line2(G& d, int x0, int y0, int x1, int y1) {
  d.drawLine(x0, y0, x1, y1, B);
  d.drawLine(x0 + 1, y0, x1 + 1, y1, B);
  d.drawLine(x0, y0 + 1, x1, y1 + 1, B);
}

template <class G> void poly(G& d, const int16_t (*p)[2], int n, int dy) {
  int x0 = X(p[0][0]), y0 = Y(p[0][1]) + dy;
  for (int i = 1; i < n - 1; i++)
    d.fillTriangle(x0, y0, X(p[i][0]), Y(p[i][1]) + dy, X(p[i + 1][0]), Y(p[i + 1][1]) + dy, W);
  for (int i = 0; i < n; i++) {
    int j = (i + 1) % n;
    line2(d, X(p[i][0]), Y(p[i][1]) + dy, X(p[j][0]), Y(p[j][1]) + dy);
  }
}

template <class G> void box(G& d, int x0, int y0, int x1, int y1, int dy) {
  int a = X(x0), b = Y(y0) + dy, w = X(x1) - a, h = Y(y1) + dy - b;
  d.fillRoundRect(a, b, w, h, 2, W);
  d.drawRoundRect(a, b, w, h, 2, B);
  d.drawRoundRect(a + 1, b + 1, w - 2, h - 2, 1, B);
}

template <class G> void curve(G& d, float x0, float y0, float cx, float cy, float x1, float y1) {
  float px = x0, py = y0;
  for (int i = 1; i <= 24; i++) {
    float t = i / 24.f, u = 1 - t;
    float x = u * u * x0 + 2 * u * t * cx + t * t * x1;
    float y = u * u * y0 + 2 * u * t * cy + t * t * y1;
    thick(d, px, py, x, y, 1, F);
    px = x; py = y;
  }
}

// Brazo + mano; la mano gira w radianes alrededor de un "codo"
template <class G> void arm(G& d, float sx, float sy, float cx, float cy,
                            float hx, float hy, const float* fing, float thumb, float w) {
  float L = S(18), pvx = hx, pvy = hy + L;
  float px = pvx - L * sinf(w), py = pvy - L * cosf(w);
  curve(d, sx, sy, cx, cy, px, py);
  d.fillCircle((int)lroundf(px), (int)lroundf(py), S(3.5f), F);
  for (int i = 0; i < 3; i++) {
    float a = fing[i] * DEG + w;
    thick(d, px + S(2) * cosf(a), py - S(2) * sinf(a), px + S(10) * cosf(a), py - S(10) * sinf(a), 1, F);
  }
  float a = thumb * DEG + w;
  thick(d, px, py, px + S(7) * cosf(a), py - S(7) * sinf(a), 1, F);
}

template <class G> void eye(G& d, int cx, int cy, int px, int py, bool closed, int hl) {
  int ro = S(9);
  d.fillCircle(cx, cy, ro, F);
  d.fillCircle(cx, cy, ro - 2, W);
  if (closed) {                 // párpado cerrado
    float ox = 0, oy = 0, r = S(5);
    for (int a = 190; a <= 350; a += 20) {
      float x = cx + r * cosf(a * DEG), y = cy - S(2) - r * sinf(a * DEG);
      if (a > 190) thick(d, ox, oy, x, y, 1, F);
      ox = x; oy = y;
    }
    return;
  }
  px = S(px); py = S(py);
  int lim = ro - 2 - S(5);
  if (px > lim) px = lim; if (px < -lim) px = -lim;
  if (py > lim) py = lim; if (py < -lim) py = -lim;
  d.fillCircle(cx + px, cy + py, S(5), F);
  d.fillCircle(cx + px + hl, cy + py, 1, W);   // brillo
}

// innerAtStart: el extremo interior (hacia el centro de la cara) es el de a0
template <class G> void brow(G& d, int cx, int cy, float a0, float a1, int lift,
                             int tilt, bool innerAtStart) {
  float ox = 0, oy = 0, r = S(15);
  for (int i = 0; i <= 10; i++) {
    float t = i / 10.f;
    float a = (a0 + (a1 - a0) * t) * DEG;
    float k = innerAtStart ? (1 - t) : t;            // 1 en el extremo interior
    float x = cx + r * cosf(a), y = cy - lift - r * sinf(a) + tilt * k;
    if (i) thick(d, ox, oy, x, y, 1, F);
    ox = x; oy = y;
  }
}

// ---------------- Adornos ----------------
template <class G> void heart(G& d, int cx, int cy, int r, uint16_t c) {
  int h = r / 2 + 1;
  d.fillCircle(cx - h, cy, h + 1, c);
  d.fillCircle(cx + h, cy, h + 1, c);
  d.fillTriangle(cx - r - 1, cy + 1, cx + r + 1, cy + 1, cx, cy + r + h + 1, c);
}

template <class G> void star(G& d, int cx, int cy, int R, uint16_t c) {
  float px[10], py[10];
  for (int i = 0; i < 10; i++) {
    float a = (-90 + i * 36) * DEG, rr = (i & 1) ? R * 0.45f : R;
    px[i] = cx + rr * cosf(a); py[i] = cy + rr * sinf(a);
  }
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    d.fillTriangle(cx, cy, (int)px[i], (int)py[i], (int)px[j], (int)py[j], c);
  }
  for (int i = 0; i < 10; i++) {
    int j = (i + 1) % 10;
    d.drawLine((int)px[i], (int)py[i], (int)px[j], (int)py[j], MASC_BLACK);
  }
}

// Gota con la punta hacia arriba
template <class G> void drop(G& d, int cx, int cy, int r, uint16_t c) {
  d.fillCircle(cx, cy, r, c);
  d.fillTriangle(cx - r, cy, cx + r, cy, cx, cy - r * 2 - 1, c);
}

template <class G> void letraZ(G& d, int x, int y, int s, uint16_t c) {
  thick(d, x, y, x + s, y, 1, c);
  thick(d, x + s, y, x, y + s, 1, c);
  thick(d, x, y + s, x + s, y + s, 1, c);
}

template <class G> void pregunta(G& d, int x, int y, int s, uint16_t c) {
  float ox = 0, oy = 0;
  for (int i = 0; i <= 12; i++) {                 // arco superior
    float a = (160 - i * 20) * DEG;
    float px = x + s * cosf(a), py = y + s * sinf(-a);
    if (i) thick(d, ox, oy, px, py, 1, c);
    ox = px; oy = py;
  }
  thick(d, ox, oy, x, y + s * 1.6f, 1, c);        // trazo hacia abajo
  d.fillCircle(x, (int)(y + s * 2.3f), 2, c);    // punto
}

template <class G> void enojo(G& d, int cx, int cy, int L, uint16_t c) {
  int g = 2;
  for (int sx = -1; sx <= 1; sx += 2)
    for (int sy = -1; sy <= 1; sy += 2) {
      thick(d, cx + sx * g, cy + sy * L, cx + sx * g, cy + sy * g, 1, c);
      thick(d, cx + sx * g, cy + sy * g, cx + sx * L, cy + sy * g, 1, c);
    }
}

template <class G> void nota(G& d, int x, int y, int s, uint16_t c) {
  int r = s / 3 + 1;
  d.fillCircle(x, y, r, c);
  thick(d, x + r, y, x + r, y - s * 1.5f, 0, c);
  thick(d, x + r + 1, y, x + r + 1, y - s * 1.5f, 0, c);
  thick(d, x + r, y - s * 1.5f, x + r + s * 0.6f, y - s * 0.9f, 1, c);
}

template <class G> void ampolleta(G& d, int cx, int cy, int r) {
  for (int i = 0; i < 8; i++) {                   // rayos
    if (i == 0) continue;                         // sin rayo hacia abajo
    float a = (i * 45 - 90 + 180) * DEG;
    thick(d, cx + (r + 3) * cosf(a), cy + (r + 3) * sinf(a),
             cx + (r + 7) * cosf(a), cy + (r + 7) * sinf(a), 0, MASC_YELLOW);
  }
  d.fillCircle(cx, cy, r + 1, MASC_BLACK);
  d.fillCircle(cx, cy, r - 1, MASC_YELLOW);
  int bw = r - 1, by = cy + r - 1;                // casquillo
  d.fillRoundRect(cx - bw / 2 - 1, by, bw + 2, r / 2 + 3, 1, MASC_BLACK);
  d.drawFastHLine(cx - bw / 2, by + 2, bw, MASC_WHITE);
}

// ---- Adornos de las poses 17-24 ----
template <class G> void gafas(G& d) {
  int ey = Y(285), ro = S(9), w = 2 * ro + 4, h = (ro * 3) / 2;
  int cxs[2] = {X(745), X(950)};
  for (int k = 0; k < 2; k++) {
    int cx = cxs[k];
    d.fillRoundRect(cx - w / 2, ey - h / 2, w, h, S(5), MASC_BLACK);
    thick(d, cx - ro / 2, ey - h / 4, cx - ro / 2 + S(4), ey - h / 4 - S(3), 0, MASC_WHITE);  // brillo
  }
  thick(d, cxs[0] + w / 2, ey - h / 4, cxs[1] - w / 2, ey - h / 4, 1, MASC_BLACK);          // puente
  thick(d, cxs[0] - w / 2, ey - h / 4, cxs[0] - w / 2 - S(5), ey - h / 2, 1, MASC_BLACK);   // patillas
  thick(d, cxs[1] + w / 2, ey - h / 4, cxs[1] + w / 2 + S(5), ey - h / 2, 1, MASC_BLACK);
}

template <class G> void espirales(G& d) {
  int ey = Y(285), ro = S(9);
  int cxs[2] = {X(745), X(950)};
  for (int k = 0; k < 2; k++) {
    d.fillCircle(cxs[k], ey, ro - 2, MASC_WHITE);
    float ox = cxs[k], oy = ey, rmax = ro - 3, dir = k ? -1 : 1;
    for (int i = 1; i <= 60; i++) {
      float t = i / 60.f, a = dir * t * 3 * 6.2832f, r = rmax * t;
      float x = cxs[k] + r * cosf(a), y = ey + r * sinf(a);
      thick(d, ox, oy, x, y, 0, MASC_RASGOS);
      thick(d, ox + 1, oy, x + 1, y, 0, MASC_RASGOS);
      ox = x; oy = y;
    }
  }
}

// Tuerca hexagonal justo entre las mandíbulas izquierdas
template <class G> void tuerca(G& d, int jaw) {
  float top = Y(418) + 2, bot = Y(488) + jaw;
  float cy = (top + bot) / 2, R = (bot - top) / 1.732f + 0.5f, cx = X(745);
  float px[6], py[6];
  for (int i = 0; i < 6; i++) { px[i] = cx + R * cosf(i * 60 * DEG); py[i] = cy + R * sinf(i * 60 * DEG); }
  for (int i = 0; i < 6; i++) {
    int j = (i + 1) % 6;
    d.fillTriangle((int)cx, (int)cy, (int)px[i], (int)py[i], (int)px[j], (int)py[j], MASC_YELLOW);
  }
  for (int i = 0; i < 6; i++) {
    int j = (i + 1) % 6;
    d.drawLine((int)px[i], (int)py[i], (int)px[j], (int)py[j], MASC_BLACK);
  }
  d.fillCircle((int)cx, (int)cy, (int)(R * 0.45f), MASC_BLACK);
  d.fillCircle((int)cx, (int)cy, (int)(R * 0.45f) - 1, MASC_WHITE);
}

template <class G> void alerta(G& d, int x, int y, int s, uint16_t c) {
  for (int i = 0; i <= s * 2; i++) {        // barra que se angosta hacia abajo
    float t = (float)i / (s * 2);
    int w = (int)(S(2.5f) * (1 - t * 0.6f));
    d.drawFastHLine(x - w, y + i, 2 * w + 1, c);
  }
  d.fillCircle(x, y + s * 2 + S(5), S(2.5f), c);
}

template <class G> void cafe(G& d, int x, int y) {
  int s = S(6);
  d.fillCircle(x + s + 1, y, S(4), MASC_BLACK);              // asa
  d.fillCircle(x + s + 1, y, S(2), MASC_WHITE);
  d.fillRoundRect(x - s, y - s, 2 * s, 2 * s + 2, 2, MASC_RED);
  d.drawRoundRect(x - s, y - s, 2 * s, 2 * s + 2, 2, MASC_BLACK);
  d.drawFastHLine(x - s - 2, y + s + 2, 2 * s + 4, MASC_BLACK);   // platillo
  d.drawFastHLine(x - s - 2, y + s + 3, 2 * s + 4, MASC_BLACK);
  for (int k = -1; k <= 1; k++) {                            // vapor
    float ox = 0, oy = 0;
    for (int i = 0; i <= 10; i++) {
      float t = i / 10.f;
      float xx = x + k * S(4) + S(1.5f) * sinf(t * 6.2832f), yy = y - s - S(2) - t * S(10);
      if (i) thick(d, ox, oy, xx, yy, 0, MASC_BLUE);
      ox = xx; oy = yy;
    }
  }
}

template <class G> void sol(G& d, int cx, int cy, int r) {
  for (int i = 0; i < 8; i++) {
    float a = i * 45 * DEG;
    thick(d, cx + (r + S(3)) * cosf(a), cy + (r + S(3)) * sinf(a),
             cx + (r + S(7)) * cosf(a), cy + (r + S(7)) * sinf(a), 1, MASC_RED);
  }
  d.fillCircle(cx, cy, r + 1, MASC_BLACK);
  d.fillCircle(cx, cy, r - 1, MASC_YELLOW);
}

template <class G> void nube(G& d, int cx, int cy, bool lluvia) {
  const int c[3][3] = {{-S(7), S(2), S(5)}, {0, -S(2), S(7)}, {S(8), S(2), S(5)}};
  for (int i = 0; i < 3; i++) d.fillCircle(cx + c[i][0], cy + c[i][1], c[i][2] + 1, MASC_BLACK);
  d.fillRect(cx - S(7), cy + S(2), S(15), S(5) + 2, MASC_BLACK);
  for (int i = 0; i < 3; i++) d.fillCircle(cx + c[i][0], cy + c[i][1], c[i][2] - 1, MASC_WHITE);
  d.fillRect(cx - S(7), cy + S(2), S(15), S(5), MASC_WHITE);
  if (!lluvia) return;
  for (int k = 0; k < 4; k++) {
    int x = cx - S(8) + k * S(5), y = cy + S(10) + (k & 1) * S(3);
    thick(d, x, y, x - S(2), y + S(4), 0, MASC_BLUE);
    thick(d, x + 1, y, x - S(2) + 1, y + S(4), 0, MASC_BLUE);
  }
}

template <class G> void arcoiris(G& d, int cx, int cy) {
  const uint16_t col[4] = {MASC_RED, MASC_YELLOW, MASC_GREEN, MASC_BLUE};
  for (int b = 0; b < 4; b++) {
    float r = S(18) - b * S(3);
    float ox = 0, oy = 0;
    for (int i = 0; i <= 36; i++) {
      float a = (180 + i * 5) * DEG;
      float x = cx + r * cosf(a), y = cy + r * sinf(a);
      if (i) thick(d, ox, oy, x, y, 1, col[b]);
      ox = x; oy = y;
    }
  }
  nube(d, cx - S(15), cy + S(1), false);
  nube(d, cx + S(15), cy + S(1), false);
}

template <class G> void extras(G& d, const Pose& p) {
  switch (p.extra) {
    case EX_CORAZONES:
      heart(d, S(143), S(16), S(6), MASC_RED);
      heart(d, S(165), S(8),  S(4), MASC_RED);
      heart(d, S(50),  S(14), S(5), MASC_RED);
      break;
    case EX_ENOJO:
      enojo(d, S(147), S(16), S(9), MASC_RED);
      break;
    case EX_LAGRIMAS:
      drop(d, S(77),  S(40), S(3), MASC_BLUE);
      drop(d, S(75),  S(52), S(2), MASC_BLUE);
      break;
    case EX_PREGUNTA:
      pregunta(d, S(148), S(12), S(8), MASC_BLUE);
      break;
    case EX_NOTAS:
      nota(d, S(140), S(30), S(7), MASC_GREEN);
      nota(d, S(160), S(18), S(6), MASC_BLUE);
      nota(d, S(55),  S(26), S(6), MASC_GREEN);
      break;
    case EX_ESTRELLAS:
      star(d, S(145), S(16), S(8), MASC_YELLOW);
      star(d, S(50),  S(14), S(6), MASC_YELLOW);
      star(d, S(168), S(40), S(5), MASC_YELLOW);
      star(d, S(40),  S(140), S(6), MASC_YELLOW);
      star(d, S(165), S(150), S(7), MASC_YELLOW);
      break;
    case EX_SUDOR:
      drop(d, S(124), S(13), S(4), MASC_BLUE);
      break;
    case EX_IDEA:
      ampolleta(d, S(152), S(18), S(9));
      break;
    case EX_ZZZ:
      letraZ(d, S(132), S(26), S(5), MASC_BLUE);
      letraZ(d, S(143), S(14), S(7), MASC_BLUE);
      letraZ(d, S(157), S(1),  S(9), MASC_BLUE);
      break;
    case EX_GAFAS:    gafas(d);                           break;
    case EX_MAREADO:  espirales(d);                       break;
    case EX_TUERCA:   tuerca(d, S(p.jaw));                break;
    case EX_ALERTA:   alerta(d, S(148), S(3), S(8), MASC_RED); break;
    case EX_CAFE:     cafe(d, S(150), S(26));             break;
    case EX_SOL:      sol(d, S(156), S(18), S(7));        break;
    case EX_LLUVIA:   nube(d, S(152), S(12), true);       break;
    case EX_ARCOIRIS: arcoiris(d, S(170), S(26));         break;
  }
}

// Piezas del pie de metro (coordenadas de la imagen original)
const int16_t JAW_UL[][2] = {{893, 322}, {655, 360}, {612, 418}, {893, 418}};
const int16_t JAW_LL[][2] = {{612, 488}, {893, 505}, {850, 592}, {632, 522}};
const int16_t JAW_UR[][2] = {{1018, 365}, {1165, 372}, {1100, 402}, {1018, 410}};
const int16_t JAW_LR[][2] = {{1018, 428}, {1090, 435}, {1160, 486}, {1018, 490}};

template <class G> void draw(G& d, const Pose& p) {
  d.fillScreen(W);
  const int j = S(p.jaw);

  static const float fL[3] = {72, 108, 144}, fR[3] = {72, 108, 144};
  arm(d, X(892), Y(890), X(700), Y(935), X(592), Y(790), fL, 20, p.waveL);
  arm(d, X(1022), Y(872), X(1262), Y(880), X(1242), Y(690), fR, 200, p.waveR);

  d.fillCircle(X(848), Y(792) + j, S(4), B);      // perilla izquierda
  d.fillCircle(X(848), Y(792) + j, S(2), W);

  poly(d, JAW_LR, 4, j);                         // mandíbulas móviles
  box(d, 1038, 485, 1058, 515, j);
  poly(d, JAW_LL, 4, j);
  poly(d, JAW_UL, 4, 0);                         // mandíbulas fijas
  poly(d, JAW_UR, 4, 0);
  box(d, 1016, 320, 1058, 372, 0);

  box(d, 850, 505, 1058, 858, j);                // corredera + rueda
  box(d, 1056, 655, 1076, 676, j);
  box(d, 1072, 632, 1122, 700, j);

  int bx0 = X(890), bx1 = X(1020), by0 = Y(330), by1 = Y(1745);   // regla
  d.fillRoundRect(bx0, by0, bx1 - bx0, by1 - by0, 2, W);
  d.drawRoundRect(bx0, by0, bx1 - bx0, by1 - by0, 2, B);
  d.drawRoundRect(bx0 + 1, by0 + 1, bx1 - bx0 - 2, by1 - by0 - 2, 1, B);
  int i = 0;
  for (int y = Y(432); y <= Y(1655); y += S(3), i++) {
    int len = (i % 10 == 0) ? S(7) : (i % 5 == 0) ? S(5) : S(3);
    d.drawFastHLine(bx0 + 2, y, len, B);
  }
  d.fillCircle(X(915), Y(1695), 1, B);
  d.fillCircle(X(985), Y(1695), 1, B);

  line2(d, X(930), by1, X(944), Y(1845));        // varilla de profundidad
  line2(d, X(944), Y(1845), X(950), Y(1880));
  line2(d, X(950), Y(1880), X(956), Y(1845));
  line2(d, X(956), Y(1845), X(970), by1);

  eye(d, X(745), Y(285), p.lx, p.py, p.blink & 1, -S(3));
  eye(d, X(950), Y(285), p.rx, p.py, p.blink & 2, S(3));
  brow(d, X(745), Y(285), 89, 129, S(p.brow), S(p.tilt), true);
  brow(d, X(950), Y(285), 55, 94, S(p.brow), S(p.tilt), false);

  extras(d, p);
}

}  // namespace Mascota
