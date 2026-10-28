// poses.h — Las 24 poses de la mascota y sus nombres.
// Lo usan el firmware del ESP32 y el generador del sitio web (tools/render).
// Para agregar una pose: añade una fila a POSES y su nombre a NOMBRES.
#pragma once
#include "mascota.h"

//  lx, rx, py, blink, brow, jaw, waveL,  waveR, tilt, extra
const Pose POSES[] = {
  { 2, -2,  0, 0, 0,  0,  0.00f,  0.00f,  0, EX_NADA},      // 1  normal
  { 2, -2,  0, 0, 2,  0,  0.00f, -0.45f,  0, EX_NADA},      // 2  saludando
  {-3, -3,  0, 0, 0,  0,  0.00f,  0.00f,  0, EX_NADA},      // 3  mira a la izquierda
  { 2, -2,  0, 2, 1,  0,  0.00f, -0.30f,  0, EX_NADA},      // 4  guiño
  {-1, -2,  3, 0, 0, -6,  0.00f,  0.00f,  0, EX_NADA},      // 5  midiendo
  { 2, -2,  0, 0, 3,  3,  0.35f, -0.35f,  0, EX_NADA},      // 6  sorpresa
  { 3,  3,  0, 0, 0,  0, -0.30f,  0.00f,  0, EX_NADA},      // 7  mira a la derecha
  { 0,  0,  0, 3, 0, -2,  0.00f,  0.00f,  0, EX_ZZZ},       // 8  durmiendo
  { 1, -1, -3, 0, 3,  2,  0.30f, -0.30f, -1, EX_CORAZONES}, // 9  enamorado
  { 1, -1,  1, 0,-2, -6, -0.20f,  0.20f,  8, EX_ENOJO},     // 10 enojado
  { 0,  0,  3, 0, 1, -2,  0.35f, -0.35f, -7, EX_LAGRIMAS},  // 11 triste
  {-3, -3, -3, 0, 2,  1,  0.00f,  0.40f, -2, EX_PREGUNTA},  // 12 confundido
  { 0,  0,  0, 3, 3,  3, -0.30f,  0.30f,  0, EX_NOTAS},     // 13 cantando
  { 2, -2, -2, 0, 3,  3,  0.45f, -0.45f,  0, EX_ESTRELLAS}, // 14 celebrando
  { 3,  3,  0, 0, 1, -1,  0.15f, -0.15f, -2, EX_SUDOR},     // 15 nervioso
  { 3,  3, -3, 0, 3,  2,  0.00f, -0.45f,  0, EX_IDEA},      // 16 ¡idea!
  // ---- poses 17-24 ----
  { 0,  0,  0, 0, 1,  0,  0.00f, -0.45f,  0, EX_GAFAS},     // 17 con onda (lentes de sol)
  { 0,  0,  0, 0, 2,  3,  0.40f, -0.20f, -3, EX_MAREADO},   // 18 mareado
  {-3, -2,  3, 0, 1,  3,  0.00f,  0.00f,  0, EX_TUERCA},    // 19 midiendo una tuerca
  { 2, -2, -1, 0, 3,  3,  0.45f, -0.45f,  0, EX_ALERTA},    // 20 ¡alerta!
  { 0,  0,  0, 3, 1,  0,  0.00f, -0.30f, -1, EX_CAFE},      // 21 pausa para café
  { 3,  3, -3, 0, 2,  1,  0.30f, -0.30f,  0, EX_SOL},       // 22 día soleado
  { 0,  0,  3, 0, 0, -1,  0.20f, -0.20f, -5, EX_LLUVIA},    // 23 día lluvioso
  { 3,  3, -3, 0, 3,  2,  0.45f, -0.45f,  0, EX_ARCOIRIS},  // 24 arcoíris
};
const int N_POSES = sizeof(POSES) / sizeof(POSES[0]);

const char* const NOMBRES[] = {
  "Normal", "Saludando", "Mira a la izquierda", "Guiño", "Midiendo", "Sorpresa",
  "Mira a la derecha", "Durmiendo", "Enamorado", "Enojado", "Triste", "Confundido",
  "Cantando", "Celebrando", "Nervioso", "¡Idea!", "Con onda", "Mareado",
  "Midiendo una tuerca", "¡Alerta!", "Pausa para café", "Día soleado", "Día lluvioso", "Arcoíris",
};
static_assert(sizeof(NOMBRES) / sizeof(NOMBRES[0]) == sizeof(POSES) / sizeof(POSES[0]),
              "Cada pose necesita un nombre");
