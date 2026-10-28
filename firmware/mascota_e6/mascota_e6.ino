/*
  Mascota virtual "Pie de Metro" — Waveshare 1.54" e-Paper (E) Spectra 6 (240x240) + ESP32
  ----------------------------------------------------------------------------------------
  24 poses. Por defecto cambian en orden aleatorio (sin repetirse dentro de cada ciclo).
  Una página web servida por el propio ESP32 permite elegir:
    - modo aleatorio o una pose fija
    - cada cuánto cambia (mínimo 3 min, recomendado por Waveshare)

  Librería necesaria (Gestor de librerías): "Adafruit GFX Library"
  (WiFi, WebServer, ESPmDNS y Preferences vienen con el core ESP32)
  Placa: "ESP32 Dev Module"

  Conexión (igual a la guía oficial de Waveshare):
    VCC->3V3  GND->GND  DIN->14  CLK->13  CS->15  DC->27  RST->26  BUSY->25  PWR->33

  WiFi:
    - Si WIFI_SSID está vacío, el ESP32 crea su propia red:
        red "Mascota-PieDeMetro", clave "mascota123"  ->  abre http://192.168.4.1
    - Si pones tu red, se conecta a ella: abre http://mascota.local
      (o la IP que aparece en el Monitor Serie a 115200).
*/
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include "epd_1in54e.h"
#define MASC_SIZE 240
#include "mascota.h"
#include "pagina.h"

// ---------- Ajustes ----------
const char* WIFI_SSID = "";                 // tu red WiFi (vacío = crea su propia red)
const char* WIFI_PASS = "";
const char* AP_SSID   = "Mascota-PieDeMetro";
const char* AP_PASS   = "mascota123";       // mínimo 8 caracteres
const char* HOSTNAME  = "mascota";          // http://mascota.local

const uint32_t INTERVALO_DEF_S = 180;       // por defecto: 3 minutos
const uint32_t INTERVALO_MIN_S = 180;       // Waveshare recomienda >= 180 s entre refrescos
const uint32_t INTERVALO_MAX_S = 86400;     // y refrescar al menos 1 vez al día
const uint32_t REFRESCO_FIJA_S = 86400;     // en pose fija se re-dibuja 1 vez al día

// ---------- Lienzo de 240x240 a 4 bits por píxel ----------
class Lienzo : public Adafruit_GFX {
 public:
  Lienzo() : Adafruit_GFX(Epd::W, Epd::H) {}
  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if ((uint16_t)x >= Epd::W || (uint16_t)y >= Epd::H) return;
    uint32_t a = x / 2 + y * (Epd::W / 2);
    uint8_t sh = (x & 1) ? 0 : 4;
    buf[a] = (buf[a] & ~(0x0F << sh)) | ((c & 0x0F) << sh);
  }
  void fillScreen(uint16_t c) override { memset(buf, ((c & 0x0F) << 4) | (c & 0x0F), sizeof(buf)); }
  const uint8_t* buffer() const { return buf; }
 private:
  uint8_t buf[Epd::BYTES];
};
Lienzo lienzo;   // lo que se envía a la pantalla
Lienzo vista;    // para las vistas previas de la web

// ---------- Poses (ver poses.h) ----------
#include "poses.h"

// ---------- Configuración (se guarda en la flash) ----------
Preferences prefs;
bool     modoAleatorio = true;
int      poseFija      = 0;
uint32_t intervaloS    = INTERVALO_DEF_S;

void cargarConfig() {
  prefs.begin("mascota", true);
  modoAleatorio = prefs.getBool("aleat", true);
  poseFija      = prefs.getInt("pose", 0);
  intervaloS    = prefs.getUInt("interv", INTERVALO_DEF_S);
  prefs.end();
  if (poseFija < 0 || poseFija >= N_POSES) poseFija = 0;
  intervaloS = constrain(intervaloS, INTERVALO_MIN_S, INTERVALO_MAX_S);
}

void guardarConfig() {
  prefs.begin("mascota", false);
  prefs.putBool("aleat", modoAleatorio);
  prefs.putInt("pose", poseFija);
  prefs.putUInt("interv", intervaloS);
  prefs.end();
}

// ---------- Orden aleatorio ("bolsa barajada") ----------
// Cada ciclo muestra todas las poses una vez, en orden aleatorio,
// y nunca la misma pose dos veces seguidas (tampoco entre ciclos).
uint8_t bolsa[sizeof(POSES) / sizeof(POSES[0])];
int     posBolsa = N_POSES;   // >= N_POSES = hay que barajar
int     actual   = -1;        // pose que está en pantalla

int siguienteAleatoria() {
  if (posBolsa >= N_POSES) {
    for (int i = 0; i < N_POSES; i++) bolsa[i] = i;
    for (int i = N_POSES - 1; i > 0; i--) {             // Fisher-Yates
      int j = esp_random() % (i + 1);                   // generador aleatorio por hardware
      uint8_t t = bolsa[i]; bolsa[i] = bolsa[j]; bolsa[j] = t;
    }
    if (bolsa[0] == actual && N_POSES > 1) {
      int j = 1 + esp_random() % (N_POSES - 1);
      uint8_t t = bolsa[0]; bolsa[0] = bolsa[j]; bolsa[j] = t;
    }
    posBolsa = 0;
  }
  return bolsa[posBolsa++];
}

// ---------- Pantalla ----------
WebServer server(80);
bool     refrescando   = false;
bool     pedirRefresco = true;   // dibujar al arrancar
int      pedirPose     = -1;     // pose concreta solicitada desde la web (-1 = ninguna)
uint32_t ultimoRefresco = 0;

void atenderWeb() { server.handleClient(); }

void mostrar(int idx) {
  refrescando = true;
  Serial.printf("Dibujando pose %d (%s)...\n", idx + 1, NOMBRES[idx]);
  Mascota::draw(lienzo, POSES[idx]);
  Epd::busyError = false;
  Epd::begin();
  Epd::init();
  Epd::display(lienzo.buffer());       // ~27 s, la web sigue respondiendo
  Epd::sleep();
  actual = idx;
  ultimoRefresco = millis();
  refrescando = false;
  Serial.println(Epd::busyError ? "Terminó con errores de BUSY." : "Listo.");
}

uint32_t esperaActualS() { return modoAleatorio ? intervaloS : REFRESCO_FIJA_S; }

// ---------- Web ----------
void enviarEstado() {
  uint32_t trans = (millis() - ultimoRefresco) / 1000;
  uint32_t espera = esperaActualS();
  int32_t proximo = (actual < 0 || pedirRefresco) ? 0 : (int32_t)espera - (int32_t)trans;
  if (proximo < 0) proximo = 0;
  String j;
  j.reserve(1200);
  j += F("{\"modo\":\"");     j += modoAleatorio ? F("aleatorio") : F("fija");
  j += F("\",\"pose\":");      j += poseFija;
  j += F(",\"intervalo\":");   j += intervaloS;
  j += F(",\"intervaloMin\":"); j += INTERVALO_MIN_S;
  j += F(",\"intervaloMax\":"); j += INTERVALO_MAX_S;
  j += F(",\"actual\":");      j += actual;
  j += F(",\"refrescando\":"); j += (refrescando || pedirRefresco) ? F("true") : F("false");
  j += F(",\"proximo\":");     j += proximo;
  j += F(",\"errorPantalla\":"); j += Epd::busyError ? F("true") : F("false");
  j += F(",\"nombres\":[");
  for (int i = 0; i < N_POSES; i++) {
    if (i) j += ',';
    j += '"'; j += NOMBRES[i]; j += '"';
  }
  j += F("]}");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", j);
}

// POST /api/config  (modo=aleatorio|fija, pose=0..N-1, intervalo=segundos)
void recibirConfig() {
  bool antesAleat = modoAleatorio;
  int  antesPose  = poseFija;
  if (server.hasArg("modo"))      modoAleatorio = server.arg("modo") != "fija";
  if (server.hasArg("pose"))      poseFija = constrain(server.arg("pose").toInt(), 0, N_POSES - 1);
  if (server.hasArg("intervalo")) intervaloS = constrain((uint32_t)server.arg("intervalo").toInt(),
                                                         INTERVALO_MIN_S, INTERVALO_MAX_S);
  guardarConfig();
  // Cambiar de modo o de pose fija actualiza la pantalla de inmediato
  if (modoAleatorio != antesAleat || (!modoAleatorio && poseFija != antesPose)) {
    pedirPose = modoAleatorio ? -1 : poseFija;
    pedirRefresco = true;
  }
  enviarEstado();
}

// POST /api/otra  -> muestra otra pose aleatoria ahora
void recibirOtra() {
  pedirPose = -1;
  pedirRefresco = true;
  enviarEstado();
}

// GET /vista.bmp?p=N  -> imagen de la pose N generada con el mismo código que la pantalla
void enviarVista() {
  int p = constrain(server.arg("p").toInt(), 0, N_POSES - 1);
  Mascota::draw(vista, POSES[p]);

  const uint32_t filaB = Epd::W / 2, datos = filaB * Epd::H, off = 14 + 40 + 16 * 4;
  uint8_t h[off];
  memset(h, 0, off);
  auto u16 = [&](int i, uint16_t v) { h[i] = v; h[i + 1] = v >> 8; };
  auto u32 = [&](int i, uint32_t v) { for (int k = 0; k < 4; k++) h[i + k] = v >> (8 * k); };
  h[0] = 'B'; h[1] = 'M'; u32(2, off + datos); u32(10, off);
  u32(14, 40); u32(18, Epd::W); u32(22, Epd::H); u16(26, 1); u16(28, 4); u32(34, datos);
  u32(46, 16); u32(50, 16);
  // Paleta parecida a los colores reales del panel (B, G, R, 0)
  const uint8_t pal[7][3] = {{30,30,30},{250,250,245},{240,200,20},{190,40,40},{0,0,0},{40,60,150},{40,130,70}};
  for (int i = 0; i < 7; i++) { h[54 + i*4] = pal[i][2]; h[55 + i*4] = pal[i][1]; h[56 + i*4] = pal[i][0]; }

  server.sendHeader("Cache-Control", "max-age=3600");
  server.setContentLength(off + datos);
  server.send(200, "image/bmp", "");
  server.sendContent((const char*)h, off);
  static uint8_t trozo[24 * 120];                       // 24 filas por envío
  const uint8_t* b = vista.buffer();
  for (int y = Epd::H - 1; y >= 0; ) {                   // BMP: filas de abajo hacia arriba
    int n = 0;
    while (n < 24 && y >= 0) { memcpy(trozo + n * filaB, b + y * filaB, filaB); n++; y--; }
    server.sendContent((const char*)trozo, n * filaB);
  }
}

void iniciarWifi() {
  bool conectado = false;
  if (strlen(WIFI_SSID) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Conectando a %s", WIFI_SSID);
    for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) { delay(250); Serial.print('.'); }
    Serial.println();
    conectado = WiFi.status() == WL_CONNECTED;
  }
  if (conectado) {
    Serial.printf("Conectado. Abre http://%s.local o http://%s\n", HOSTNAME, WiFi.localIP().toString().c_str());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("Red propia \"%s\" (clave %s). Abre http://%s\n",
                  AP_SSID, AP_PASS, WiFi.softAPIP().toString().c_str());
  }
  if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  cargarConfig();
  iniciarWifi();

  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html; charset=utf-8", PAGINA_HTML); });
  server.on("/api/estado", HTTP_GET, enviarEstado);
  server.on("/api/config", HTTP_POST, recibirConfig);
  server.on("/api/otra", HTTP_POST, recibirOtra);
  server.on("/vista.bmp", HTTP_GET, enviarVista);
  server.onNotFound([] { server.send(404, "text/plain", "No encontrado"); });
  server.begin();

  Epd::idleHook = atenderWeb;   // la web responde aunque la pantalla esté refrescando
  pedirPose = modoAleatorio ? -1 : poseFija;
  pedirRefresco = true;
}

void loop() {
  server.handleClient();
  if (refrescando) return;

  bool toca = actual >= 0 && (millis() - ultimoRefresco) >= esperaActualS() * 1000UL;
  if (pedirRefresco || toca) {
    int idx;
    if (pedirPose >= 0)      idx = pedirPose;
    else if (modoAleatorio)  idx = siguienteAleatoria();
    else                     idx = poseFija;
    pedirRefresco = false;
    pedirPose = -1;
    mostrar(idx);
  }
  delay(2);
}
