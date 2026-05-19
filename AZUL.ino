#include <SPI.h>
#include <SD.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <DHT.h>

// ===================== PINES =====================
#define TFT_CS     15
#define TFT_DC     2
#define TFT_RST    4
#define TOUCH_CS   21
#define TOUCH_IRQ  22
#define SD_CS      5
#define BUZZER     27
#define DHT_PIN    16
#define LED_PIN    17

// ===================== DHT11 =====================
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// ===================== CALIBRACIÓN TOUCH =====================
#define TS_MINX 300
#define TS_MAXX 3700
#define TS_MINY 300
#define TS_MAXY 3700

// ===================== PALETA ACTUALIZADA (MIDNIGHT EMERALD) =====================
#define COLOR_FONDO         0x0841  // Gris muy oscuro (Fondo principal)
#define COLOR_HEADER        0x0208  // Verde oscuro profundo (Encabezado)
#define COLOR_HEADER_TEXTO  0x07E0  // Verde Esmeralda brillante
#define COLOR_HEADER_LINE   0x03E0  // Verde medio
#define COLOR_BOTON         0x0102  // Fondo de botón oscuro
#define COLOR_BORDE         0x0400  // Borde verde bosque
#define COLOR_BORDE_INT     0x07E0  // Borde interno esmeralda
#define COLOR_TEXTO         0xFFFF  // Blanco puro para lectura principal
#define COLOR_TEXTO_SEC     0xAD55  // Gris seda para subtítulos
#define COLOR_TEXTO_BODY    0xDEFB  // Blanco azulado para contenido
#define COLOR_TEXTO_LABEL   0x07E0  // Verde esmeralda para etiquetas
#define COLOR_BORDE_VOL     0x03E0  // Verde medio para botón volver
#define COLOR_OK            0x07E0  // Verde éxito
#define COLOR_ERROR         0xF800  // Rojo error

#define ANCHO_PANTALLA  320
#define ALTO_PANTALLA   240

// ===================== PALETA JUEGO =====================
#define COLOR_PASTO          0xAFE5
#define COLOR_PASTO_ALT      0x9EE5
#define COLOR_MANZANA        0xF800
#define COLOR_MANZANA_HOJA   0x07E0
#define COLOR_SERP_CUERPO    0x0320
#define COLOR_SERP_CABEZA    0x05E0
#define COLOR_SERP_BOCA      0xF800
#define COLOR_SERP_OJO       0xFFFF
#define COLOR_SANGRE         0x9800
#define COLOR_SANGRE_OSC     0x6000
#define COLOR_GAMEOVER       0xF800
#define COLOR_DPAD           0xC618
#define COLOR_DPAD_BORDE     0x8410

// ===================== JUEGO (SNAKE) =====================
#define TAMANO_CELDA       8
#define COLS_JUEGO         (ANCHO_PANTALLA / TAMANO_CELDA)
#define FILAS_JUEGO        (160 / TAMANO_CELDA)
#define CANTIDAD_MANZANAS  50
#define MAX_SERPENTE       (COLS_JUEGO * FILAS_JUEGO)
#define VELOCIDAD_MS       180
#define ANIM_BOCA_MS       120

#define DPAD_CX      160
#define DPAD_CY      200
#define DPAD_R       30

enum Direccion { DIR_ARRIBA, DIR_ABAJO, DIR_IZQ, DIR_DER };

struct Punto {
  int8_t x;
  int8_t y;
};

struct EstadoJuego {
  Punto serpiente[MAX_SERPENTE];
  int largo;
  Direccion direccion;
  Direccion direccionPendiente;
  Punto manzanas[CANTIDAD_MANZANAS];
  int cantManzanas;
  int puntaje;
  bool activo;
  bool gameOver;
  bool comiendo;
  bool bocaAbierta;
  unsigned long ultimoMovimiento;
  unsigned long inicioAnimBoca;
  bool musicaActiva;
  unsigned long inicioMusica;
  unsigned long ultimoCambioNota;
  int notaMusicaActual;
};

// ===================== LAYOUT =====================
#define HEADER_Y    0
#define HEADER_H    30

// ── Fila 1: 2 botones más anchos (NOTAS fue movida a ARCHIVOS) ──
#define BTN_F1_Y    38
#define BTN_F1_H    62
#define BTN_F1_W    148
#define BTN_F1_X1   4
#define BTN_F1_X2   160

// ── Fila 2: DHT11 y JUEGOS ──
#define BTN_F2_Y    112
#define BTN_F2_H    62
#define BTN_F2_W    145
#define BTN_F2_X1   10
#define BTN_F2_X2   165

// ── Botón Volver ──
#define BTN_VOL_X   10
#define BTN_VOL_Y   195
#define BTN_VOL_W   300
#define BTN_VOL_H   35

// ── Botón NOTAS dentro de la pantalla ARCHIVOS ──
#define BTN_NOTAS_X   10
#define BTN_NOTAS_Y   153
#define BTN_NOTAS_W   140
#define BTN_NOTAS_H   28

// ===================== OBJETOS =====================
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// ===================== ESTADO =====================
bool sdOk = false;
EstadoJuego juego;

enum Pantalla {
  BOOT,
  MENU,
  ARCHIVOS,
  NOTAS,
  INFO,
  TEMPERATURA,
  JUEGOS
};
Pantalla pantallaActual = BOOT;

// ===================== PROTOTIPOS =====================
void dibujarBoot();
void dibujarMenu();
void dibujarArchivos();
void dibujarNotas();
void dibujarInfo();
void dibujarTemperatura();
void dibujarJuego();
void dibujarHeader(const char* titulo);
void dibujarBoton(int x, int y, int ancho, int alto, const char* etiqueta, const char* subtitulo);
void dibujarBotonVolver();
void manejarMenuPrincipal(int x, int y);
void manejarArchivos(int x, int y);
void manejarBotonVolver(int x, int y);
bool obtenerTouch(int &x, int &y);
bool enRectangulo(int tx, int ty, int rx, int ry, int rw, int rh);
void beepBoton();
void parpadearLED(int veces, int ms_on, int ms_off);
void procesarJuego();

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ── Todos los CS en HIGH antes de begin() para evitar conflicto SPI ──
  pinMode(TFT_CS,   OUTPUT);
  digitalWrite(TFT_CS,   HIGH);
  pinMode(TOUCH_CS, OUTPUT); digitalWrite(TOUCH_CS, HIGH);
  pinMode(SD_CS,    OUTPUT); digitalWrite(SD_CS,    HIGH);

  tft.begin();
  tft.setRotation(3);
  ts.begin();
  ts.setRotation(3);

  dht.begin();
  randomSeed(analogRead(0)); // Semilla aleatoria para el juego

  parpadearLED(6, 120, 100);
  dibujarBoot();
  if (SD.begin(SD_CS)) {
    sdOk = true;
    Serial.println("[SD] OK");
  } else {
    Serial.println("[SD] Sin tarjeta o error");
  }

  delay(1800);
  parpadearLED(2, 300, 200);
  digitalWrite(LED_PIN, LOW);

  dibujarMenu();
}

// ===================== LOOP =====================
void loop() {
  if (pantallaActual == JUEGOS) {
    procesarJuego();
    return;
  }

  int x, y;
  if (obtenerTouch(x, y)) {
    Serial.printf("[TOUCH] x=%d y=%d\n", x, y);
    if (pantallaActual == MENU) {
      manejarMenuPrincipal(x, y);
    } else if (pantallaActual == ARCHIVOS) {
      manejarArchivos(x, y);
    } else {
      manejarBotonVolver(x, y);
    }
  }
}

// ===================== LED =====================
void parpadearLED(int veces, int ms_on, int ms_off) {
  for (int i = 0; i < veces; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(ms_on);
    digitalWrite(LED_PIN, LOW);  delay(ms_off);
  }
}

// ===================== BEEP =====================
void beepBoton() {
  tone(BUZZER, 1200, 60);
  delay(70);
  noTone(BUZZER);
}

// ===================== LECTURA DE TOUCH =====================
bool obtenerTouch(int &x, int &y) {
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  Serial.printf("[RAW] x=%d y=%d z=%d\n", p.x, p.y, p.z);
  if (p.z < 100) return false;
  x = map(p.x, TS_MAXX, TS_MINX, 0, ANCHO_PANTALLA);
  y = map(p.y, TS_MAXY, TS_MINY, 0, ALTO_PANTALLA);
  x = constrain(x, 0, ANCHO_PANTALLA - 1);
  y = constrain(y, 0, ALTO_PANTALLA - 1);
  delay(80);
  return true;
}

// ===================== HIT TEST =====================
bool enRectangulo(int tx, int ty, int rx, int ry, int rw, int rh) {
  return (tx >= rx && tx <= rx + rw && ty >= ry && ty <= ry + rh);
}

// ═══════════════════════════════════════════════════════════
//                        PANTALLAS
// ═══════════════════════════════════════════════════════════

void dibujarBoot() {
  pantallaActual = BOOT;

  // Fondo basado en la nueva paleta oscura
  tft.fillScreen(COLOR_FONDO);
  
  // Usamos tonos verdes para el arranque en lugar de morados
  const uint16_t VERDE_LUZ  = COLOR_HEADER_TEXTO;
  const uint16_t VERDE_MED  = COLOR_HEADER_LINE;
  const uint16_t VERDE_OSC  = COLOR_HEADER;

  // Marco decorativo centrado (no cubre toda la pantalla)
  int marcoX = 30, marcoY = 70, marcoW = 260, marcoH = 100;
  tft.drawRect(marcoX,     marcoY,     marcoW,     marcoH,     VERDE_OSC);
  tft.drawRect(marcoX + 2, marcoY + 2, marcoW - 4, marcoH - 4, VERDE_MED);
  // Texto "AtheniumOS" en fuente pixel tamaño 3, centrado
  const char* titulo = "AtheniumOS";
  int16_t txtX = (ANCHO_PANTALLA - (int16_t)(strlen(titulo) * 18)) / 2;
  int16_t txtY = marcoY + 20;
  tft.setTextColor(VERDE_LUZ);
  tft.setTextSize(3);
  tft.setCursor(txtX, txtY);
  tft.print(titulo);

  // Subtitulo pequeño centrado
  const char* sub = ">> Iniciando sistema...";
  int16_t subX = (ANCHO_PANTALLA - (int16_t)(strlen(sub) * 6)) / 2;
  tft.setTextColor(VERDE_MED);
  tft.setTextSize(1);
  tft.setCursor(subX, marcoY + 62);
  tft.print(sub);
  // Barra de progreso animada
  int barX = marcoX + 10, barY = marcoY + 78, barW = marcoW - 20, barH = 10;
  tft.drawRect(barX, barY, barW, barH, VERDE_MED);
  tft.fillRect(barX + 1, barY + 1, barW - 2, barH - 2, VERDE_OSC);
  for (int p = 0; p <= barW - 2; p += 8) {
    tft.fillRect(barX + 1, barY + 1, p, barH - 2, VERDE_LUZ);
    delay(25);
  }
}

void dibujarMenu() {
  pantallaActual = MENU;
  tft.fillScreen(COLOR_FONDO);
  dibujarHeader(">> Menu Principal");
  dibujarBoton(BTN_F1_X1, BTN_F1_Y, BTN_F1_W, BTN_F1_H, "ARCH",   "SD + Notas");
  dibujarBoton(BTN_F1_X2, BTN_F1_Y, BTN_F1_W, BTN_F1_H, "INFO",   "Sistema");
  dibujarBoton(BTN_F2_X1, BTN_F2_Y, BTN_F2_W, BTN_F2_H, "DHT11",  "Temp/Hum");
  dibujarBoton(BTN_F2_X2, BTN_F2_Y, BTN_F2_W, BTN_F2_H, "JUEGOS", "Arcade");

  tft.setTextSize(1);
  tft.setTextColor(sdOk ? COLOR_OK : COLOR_ERROR);
  tft.setCursor(10, 186);
  tft.print(sdOk ? "SD: OK" : "SD: Sin tarjeta");
}

void dibujarArchivos() {
  pantallaActual = ARCHIVOS;
  tft.fillScreen(COLOR_FONDO);
  dibujarHeader(">> Archivos SD");
  dibujarBotonVolver();

  tft.fillRect(BTN_NOTAS_X, BTN_NOTAS_Y, BTN_NOTAS_W, BTN_NOTAS_H, COLOR_BOTON);
  tft.drawRect(BTN_NOTAS_X, BTN_NOTAS_Y, BTN_NOTAS_W, BTN_NOTAS_H, COLOR_BORDE);
  tft.drawRect(BTN_NOTAS_X + 2, BTN_NOTAS_Y + 2, BTN_NOTAS_W - 4, BTN_NOTAS_H - 4, COLOR_BORDE_INT);
  tft.setTextColor(COLOR_TEXTO);
  tft.setTextSize(1);
  tft.setCursor(BTN_NOTAS_X + 8, BTN_NOTAS_Y + 10);
  tft.print("[ NOTAS / notes.txt ]");

  if (!sdOk) {
    tft.setTextColor(COLOR_ERROR);
    tft.setTextSize(1);
    tft.setCursor(15, 55);
    tft.print("Error: SD no detectada");
    return;
  }

  File raiz = SD.open("/");
  if (!raiz) {
    tft.setTextColor(COLOR_TEXTO_BODY);
    tft.setTextSize(1);
    tft.setCursor(15, 55);
    tft.print("No se pudo abrir /");
    return;
  }

  tft.setTextSize(1);
  int posY    = 42;
  int cantidad = 0;

  while (cantidad < 7) {
    File archivo = raiz.openNextFile();
    if (!archivo) break;
    tft.setCursor(15, posY);
    if (archivo.isDirectory()) {
      tft.setTextColor(COLOR_TEXTO_SEC);
      tft.print("[DIR] ");
    } else {
      tft.setTextColor(COLOR_TEXTO_BODY);
    }
    tft.print(archivo.name());
    archivo.close();
    posY += 14;
    cantidad++;
  }
  raiz.close();

  if (cantidad == 0) {
    tft.setTextColor(COLOR_TEXTO_SEC);
    tft.setCursor(15, 55);
    tft.print("Directorio vacio");
  }
}

void dibujarNotas() {
  pantallaActual = NOTAS;
  tft.fillScreen(COLOR_FONDO);
  dibujarHeader(">> Notas (notes.txt)");
  dibujarBotonVolver();

  if (!sdOk) {
    tft.setTextColor(COLOR_ERROR);
    tft.setTextSize(1);
    tft.setCursor(15, 55);
    tft.print("SD no disponible");
    return;
  }

  File nota = SD.open("/notes.txt");
  if (!nota) {
    tft.setTextColor(COLOR_TEXTO_BODY);
    tft.setTextSize(1);
    tft.setCursor(15, 55);
    tft.print("Archivo notes.txt");
    tft.setCursor(15, 68);
    tft.print("no encontrado");
    return;
  }

  tft.setTextColor(COLOR_TEXTO_BODY);
  tft.setTextSize(1);
  int posY  = 42;
  String linea = "";

  while (nota.available() && posY < BTN_VOL_Y - 10) {
    char c = nota.read();
    if (c == '\n' || c == '\r') {
      if (linea.length() > 0) {
        tft.setCursor(10, posY);
        tft.print(linea.substring(0, 36));
        posY += 13;
        linea = "";
      }
    } else {
      linea += c;
    }
  }
  if (linea.length() > 0 && posY < BTN_VOL_Y - 10) {
    tft.setCursor(10, posY);
    tft.print(linea.substring(0, 36));
  }
  nota.close();
}

void dibujarInfo() {
  pantallaActual = INFO;
  tft.fillScreen(COLOR_FONDO);
  dibujarHeader(">> Informacion");
  dibujarBotonVolver();

  tft.setTextSize(1);
  int y = 45;

  auto fila = [&](const char* etiqueta, const char* valor, uint16_t color = COLOR_TEXTO_BODY) {
    tft.setTextColor(COLOR_TEXTO_LABEL);
    tft.setCursor(12, y);  tft.print(etiqueta);
    tft.setTextColor(color);            tft.setCursor(115, y); tft.print(valor);
    y += 16;
  };
  fila("CPU:",        "ESP32 240MHz");
  fila("Pantalla:",   "ILI9341 320x240");
  fila("Touch:",      "XPT2046 SPI");
  fila("SD Card:",    sdOk ? "Conectada" : "No detectada",
                      sdOk ? (uint16_t)COLOR_OK : (uint16_t)COLOR_ERROR);
  fila("Sensor:",     "DHT11 en GPIO 16");
  fila("LED:",        "GPIO 17 (inicio)");
  fila("Buzzer:",     "GPIO 27");
}

void dibujarTemperatura() {
  pantallaActual = TEMPERATURA;
  tft.fillScreen(COLOR_FONDO);
  dibujarHeader(">> Sensor DHT11");
  dibujarBotonVolver();
  float temperatura = dht.readTemperature();
  float humedad     = dht.readHumidity();
  bool  lectura_ok  = !isnan(temperatura) && !isnan(humedad);
  if (!lectura_ok) {
    tft.setTextColor(COLOR_ERROR); tft.setTextSize(1);
    tft.setCursor(15, 80); tft.print("Error: no se pudo leer el DHT11");
    tft.setCursor(15, 96);
    tft.print("Verificar conexion en GPIO 16");
    return;
  }

  tft.drawRect(10, 42, 140, 70, COLOR_BORDE);
  tft.fillRect(11, 43, 138, 68, 0x0000); // Fondo oscuro
  tft.setTextColor(COLOR_TEXTO_LABEL); tft.setTextSize(1);
  tft.setCursor(15, 48); tft.print("TEMPERATURA");
  tft.setTextColor(COLOR_HEADER_TEXTO); tft.setTextSize(3);
  char bufTemp[10]; dtostrf(temperatura, 4, 1, bufTemp);
  tft.setCursor(15, 65); tft.print(bufTemp);
  tft.setTextSize(2); tft.setTextColor(COLOR_BORDE);
  tft.setCursor(95, 65); tft.print("\xF7""C");
  tft.drawRect(165, 42, 145, 70, COLOR_BORDE);
  tft.fillRect(166, 43, 143, 68, 0x0000); // Fondo oscuro
  tft.setTextColor(COLOR_TEXTO_LABEL); tft.setTextSize(1);
  tft.setCursor(170, 48); tft.print("HUMEDAD");
  tft.setTextColor(COLOR_HEADER_TEXTO); tft.setTextSize(3);
  char bufHum[10];
  dtostrf(humedad, 4, 1, bufHum);
  tft.setCursor(170, 65); tft.print(bufHum);
  tft.setTextSize(2); tft.setTextColor(COLOR_BORDE);
  tft.setCursor(255, 65); tft.print("%");

  tft.setTextColor(COLOR_TEXTO_LABEL); tft.setTextSize(1);
  tft.setCursor(10, 122);
  tft.print("Sensor: DHT11  |  GPIO: 16");
  tft.setCursor(10, 136); tft.print("Presiona VOLVER para actualizar");
}

// ===================== JUEGO: UTILIDADES =====================
int celdaPx(int col) { return col * TAMANO_CELDA; }
int celdaPy(int fila) { return fila * TAMANO_CELDA; }

bool puntoEnSerpiente(int col, int fila, int ignorarUltimos) {
  int limite = juego.largo - ignorarUltimos;
  if (limite < 0) limite = 0;
  for (int i = 0; i < limite; i++) {
    if (juego.serpiente[i].x == col && juego.serpiente[i].y == fila) {
      return true;
    }
  }
  return false;
}

int indiceManzanaEn(int col, int fila) {
  for (int i = 0; i < juego.cantManzanas; i++) {
    if (juego.manzanas[i].x == col && juego.manzanas[i].y == fila) {
      return i;
    }
  }
  return -1;
}

bool celdaOcupada(int col, int fila, bool ignorarCola) {
  if (col < 0 || col >= COLS_JUEGO || fila < 0 || fila >= FILAS_JUEGO) {
    return true;
  }
  if (indiceManzanaEn(col, fila) >= 0) return true;
  if (puntoEnSerpiente(col, fila, ignorarCola ? 1 : 0)) return true;
  return false;
}

bool colocarManzanaAleatoria(Punto &m) {
  for (int intento = 0; intento < 500; intento++) {
    m.x = random(0, COLS_JUEGO);
    m.y = random(0, FILAS_JUEGO);
    if (!celdaOcupada(m.x, m.y, true)) {
      return true;
    }
  }
  return false;
}

void generarManzanasIniciales() {
  juego.cantManzanas = 0;
  for (int i = 0; i < CANTIDAD_MANZANAS; i++) {
    Punto m;
    if (colocarManzanaAleatoria(m)) {
      juego.manzanas[juego.cantManzanas++] = m;
    }
  }
}

// ===================== JUEGO: DIBUJO =====================
void dibujarCeldaPasto(int col, int fila) {
  int px = celdaPx(col);
  int py = celdaPy(fila);
  uint16_t color = ((col + fila) % 2 == 0) ? COLOR_PASTO : COLOR_PASTO_ALT;
  tft.fillRect(px, py, TAMANO_CELDA, TAMANO_CELDA, color);
}

void dibujarMapaPasto() {
  for (int f = 0; f < FILAS_JUEGO; f++) {
    for (int c = 0; c < COLS_JUEGO; c++) {
      dibujarCeldaPasto(c, f);
    }
  }
}

void dibujarManzanaCelda(int col, int fila) {
  int px = celdaPx(col);
  int py = celdaPy(fila);
  dibujarCeldaPasto(col, fila);
  tft.fillCircle(px + 4, py + 5, 3, COLOR_MANZANA);
  tft.fillRect(px + 3, py + 1, 2, 2, COLOR_MANZANA_HOJA);
}

void dibujarSerpienteCelda(int col, int fila, bool esCabeza, bool bocaAbierta, Direccion dir) {
  int px = celdaPx(col);
  int py = celdaPy(fila);
  dibujarCeldaPasto(col, fila);

  uint16_t colorCuerpo = esCabeza ? COLOR_SERP_CABEZA : COLOR_SERP_CUERPO;
  tft.fillRect(px + 1, py + 1, TAMANO_CELDA - 2, TAMANO_CELDA - 2, colorCuerpo);

  if (esCabeza) {
    int ojoDx = 2, ojoDy = 2;
    int bocaPx = px + 2, bocaPy = py + 2, bocaW = 4, bocaH = 4;

    switch (dir) {
      case DIR_ARRIBA:
        ojoDx = 2; ojoDy = 1;
        bocaPx = px + 2; bocaPy = py; bocaW = 4; bocaH = 3;
        break;
      case DIR_ABAJO:
        ojoDx = 2; ojoDy = 4;
        bocaPx = px + 2; bocaPy = py + 5; bocaW = 4; bocaH = 3;
        break;
      case DIR_IZQ:
        ojoDx = 1; ojoDy = 2;
        bocaPx = px; bocaPy = py + 2; bocaW = 3; bocaH = 4;
        break;
      case DIR_DER:
        ojoDx = 4; ojoDy = 2;
        bocaPx = px + 5; bocaPy = py + 2; bocaW = 3; bocaH = 4;
        break;
    }

    tft.fillRect(px + ojoDx,     py + ojoDy,     2, 2, COLOR_SERP_OJO);
    tft.fillRect(px + ojoDx + 3, py + ojoDy,     2, 2, COLOR_SERP_OJO);

    if (bocaAbierta) {
      tft.fillRect(bocaPx, bocaPy, bocaW, bocaH, COLOR_SERP_BOCA);
    }
  }
}

void redibujarCelda(int col, int fila) {
  int idxManz = indiceManzanaEn(col, fila);
  bool esCabeza = (juego.largo > 0 &&
                   juego.serpiente[0].x == col &&
                   juego.serpiente[0].y == fila);

  if (esCabeza) {
    dibujarSerpienteCelda(col, fila, true, juego.bocaAbierta || juego.comiendo,
                          juego.direccion);
  } else if (puntoEnSerpiente(col, fila, 0)) {
    dibujarSerpienteCelda(col, fila, false, false, juego.direccion);
  } else if (idxManz >= 0) {
    dibujarManzanaCelda(col, fila);
  } else {
    dibujarCeldaPasto(col, fila);
  }
}

void dibujarPuntajeHUD();

void dibujarControlesTactiles() {
  // Fondo del area de control (parte inferior)
  tft.fillRect(0, 160, ANCHO_PANTALLA, 80, COLOR_FONDO);
  tft.drawFastHLine(0, 160, ANCHO_PANTALLA, COLOR_BORDE);

  // Dibujar 4 botones grandes
  int btnW = 50;
  int btnH = 35;
  int cx = 160;
  int cy = 200;

  // Boton UP
  tft.fillRect(cx - btnW/2, 163, btnW, btnH, COLOR_DPAD);
  tft.drawRect(cx - btnW/2, 163, btnW, btnH, COLOR_DPAD_BORDE);
  tft.fillTriangle(cx, 168, cx - 12, 190, cx + 12, 190, COLOR_BORDE);

  // Boton DOWN
  tft.fillRect(cx - btnW/2, 203, btnW, btnH, COLOR_DPAD);
  tft.drawRect(cx - btnW/2, 203, btnW, btnH, COLOR_DPAD_BORDE);
  tft.fillTriangle(cx, 233, cx - 12, 211, cx + 12, 211, COLOR_BORDE);

  // Boton LEFT
  int leftX = cx - btnW/2 - btnW - 5;
  tft.fillRect(leftX, cy - btnH/2, btnW, btnH, COLOR_DPAD);
  tft.drawRect(leftX, cy - btnH/2, btnW, btnH, COLOR_DPAD_BORDE);
  tft.fillTriangle(leftX + 8, cy, leftX + 30, cy - 12, leftX + 30, cy + 12, COLOR_BORDE);

  // Boton RIGHT
  int rightX = cx + btnW/2 + 5;
  tft.fillRect(rightX, cy - btnH/2, btnW, btnH, COLOR_DPAD);
  tft.drawRect(rightX, cy - btnH/2, btnW, btnH, COLOR_DPAD_BORDE);
  tft.fillTriangle(rightX + btnW - 8, cy, rightX + btnW - 30, cy - 12, rightX + btnW - 30, cy + 12, COLOR_BORDE);

  // Puntaje HUD en la izquierda
  tft.fillRect(5, 175, 70, 45, COLOR_DPAD);
  tft.drawRect(5, 175, 70, 45, COLOR_DPAD_BORDE);
  tft.setTextColor(COLOR_TEXTO_BODY);
  tft.setTextSize(1);
  tft.setCursor(10, 182);
  tft.print("Pts:");
  dibujarPuntajeHUD();
}

void dibujarPuntajeHUD() {
  tft.fillRect(6, 198, 68, 16, COLOR_DPAD);
  tft.setTextColor(COLOR_TEXTO);
  tft.setTextSize(2);
  tft.setCursor(10, 198);
  tft.print(juego.puntaje);
}

void dibujarJuegoCompleto() {
  dibujarMapaPasto();

  for (int i = 0; i < juego.cantManzanas; i++) {
    dibujarManzanaCelda(juego.manzanas[i].x, juego.manzanas[i].y);
  }

  for (int i = juego.largo - 1; i >= 0; i--) {
    bool cabeza = (i == 0);
    dibujarSerpienteCelda(juego.serpiente[i].x, juego.serpiente[i].y,
                          cabeza, false, juego.direccion);
  }

  dibujarControlesTactiles();
}

void dibujarGameOver();

// ===================== JUEGO: LOGICA =====================
void dibujarJuego() {
  pantallaActual = JUEGOS;
  randomSeed(analogRead(0) ^ millis());

  juego.largo = 3;
  juego.direccion = DIR_DER;
  juego.direccionPendiente = DIR_DER;
  juego.puntaje = 0;
  juego.activo = true;
  juego.gameOver = false;
  juego.comiendo = false;
  juego.bocaAbierta = false;
  juego.ultimoMovimiento = millis();
  juego.inicioAnimBoca = 0;
  juego.cantManzanas = 0;
  juego.musicaActiva = true;
  juego.inicioMusica = millis();
  juego.ultimoCambioNota = 0;
  juego.notaMusicaActual = 0;

  int cx = COLS_JUEGO / 2;
  int cy = FILAS_JUEGO / 2;
  for (int i = 0; i < juego.largo; i++) {
    juego.serpiente[i].x = cx - i;
    juego.serpiente[i].y = cy;
  }

  generarManzanasIniciales();

  tft.fillRect(0, 0, ANCHO_PANTALLA, 160, COLOR_PASTO);
  dibujarJuegoCompleto();
}

bool direccionOpuesta(Direccion actual, Direccion nueva) {
  return (actual == DIR_ARRIBA && nueva == DIR_ABAJO) ||
         (actual == DIR_ABAJO && nueva == DIR_ARRIBA) ||
         (actual == DIR_IZQ && nueva == DIR_DER) ||
         (actual == DIR_DER && nueva == DIR_IZQ);
}

void cambiarDireccion(Direccion nueva) {
  if (!direccionOpuesta(juego.direccion, nueva)) {
    juego.direccionPendiente = nueva;
  }
}

void finalizarJuego() {
  juego.activo = false;
  juego.gameOver = true;
  juego.bocaAbierta = true;
  juego.musicaActiva = false;
  noTone(BUZZER);

  Punto cabeza = juego.serpiente[0];
  redibujarCelda(cabeza.x, cabeza.y);

  tone(BUZZER, 400, 300);
  delay(350);
  noTone(BUZZER);

  dibujarGameOver();
}

void moverSerpiente() {
  if (!juego.activo || juego.gameOver) return;

  juego.direccion = juego.direccionPendiente;

  Punto cabeza = juego.serpiente[0];
  Punto nuevaCabeza = cabeza;

  switch (juego.direccion) {
    case DIR_ARRIBA: nuevaCabeza.y--; break;
    case DIR_ABAJO:  nuevaCabeza.y++; break;
    case DIR_IZQ:    nuevaCabeza.x--; break;
    case DIR_DER:    nuevaCabeza.x++; break;
  }

  bool fueraLimites = (nuevaCabeza.x < 0 || nuevaCabeza.x >= COLS_JUEGO ||
                       nuevaCabeza.y < 0 || nuevaCabeza.y >= FILAS_JUEGO);

  bool choqueCuerpo = puntoEnSerpiente(nuevaCabeza.x, nuevaCabeza.y, 1);

  if (fueraLimites || choqueCuerpo) {
    finalizarJuego();
    return;
  }

  int idxManz = indiceManzanaEn(nuevaCabeza.x, nuevaCabeza.y);
  bool comio = (idxManz >= 0);

  Punto cola = juego.serpiente[juego.largo - 1];

  for (int i = juego.largo - 1; i > 0; i--) {
    juego.serpiente[i] = juego.serpiente[i - 1];
  }
  juego.serpiente[0] = nuevaCabeza;

  if (comio) {
    if (juego.largo < MAX_SERPENTE) {
      juego.serpiente[juego.largo] = cola;
      juego.largo++;
    }

    for (int i = idxManz; i < juego.cantManzanas - 1; i++) {
      juego.manzanas[i] = juego.manzanas[i + 1];
    }
    juego.cantManzanas--;

    juego.puntaje++;
    juego.comiendo = true;
    juego.inicioAnimBoca = millis();

  } else {
    redibujarCelda(cola.x, cola.y);
  }

  redibujarCelda(cabeza.x, cabeza.y);
  redibujarCelda(nuevaCabeza.x, nuevaCabeza.y);
  dibujarPuntajeHUD();

  if (comio) {
    Punto nuevaManz;
    if (colocarManzanaAleatoria(nuevaManz)) {
      juego.manzanas[juego.cantManzanas++] = nuevaManz;
      dibujarManzanaCelda(nuevaManz.x, nuevaManz.y);
    }
  }
}

void dibujarGameOver() {
  tft.fillScreen(0x0000);

  tft.setTextColor(COLOR_GAMEOVER);
  tft.setTextSize(2);
  tft.setCursor(18, 50);
  tft.print("jajajaja, por");
  tft.setCursor(30, 72);
  tft.print("pendejo");

  for (int i = 0; i < 40; i++) {
    int bx = 18 + random(0, 220);
    int by = 95 + random(0, 80);
    int gota = random(2, 6);
    tft.fillRect(bx, by, gota, random(8, 28), COLOR_SANGRE);
    tft.fillRect(bx, by + gota, gota / 2 + 1, random(4, 14), COLOR_SANGRE_OSC);
  }

  tft.fillRect(10, 175, 300, 3, COLOR_SANGRE);
  for (int x = 10; x < 310; x += 12) {
    tft.fillRect(x, 178, random(2, 5), random(10, 35), COLOR_SANGRE);
  }

  tft.setTextColor(COLOR_HEADER_TEXTO);
  tft.setTextSize(2);
  tft.setCursor(40, 130);
  tft.print("SCORE: ");
  tft.setTextColor(COLOR_TEXTO_SEC);
  tft.print(juego.puntaje);

  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXTO_LABEL);
  tft.setCursor(55, 160);
  tft.print("manzanas comidas = puntos");

  tft.setTextColor(COLOR_TEXTO_SEC);
  tft.setCursor(70, 210);
  tft.print("Toca pantalla p/ menu");
}

void manejarTouchJuego(int x, int y) {
  if (juego.gameOver) {
    beepBoton();
    dibujarMenu();
    return;
  }

  if (y > 160) {
    int dx = x - 160;
    int dy = y - 200;

    if (abs(dx) > abs(dy)) {
      cambiarDireccion(dx > 0 ? DIR_DER : DIR_IZQ);
    } else {
      cambiarDireccion(dy > 0 ? DIR_ABAJO : DIR_ARRIBA);
    }
    
    if (!juego.musicaActiva) {
      tone(BUZZER, 900, 30);
    }
  }
}

void procesarJuego() {
  unsigned long ahora = millis();

  if (juego.musicaActiva) {
    if (ahora - juego.inicioMusica >= 10000) {
      juego.musicaActiva = false;
      noTone(BUZZER);
    } else {
      if (ahora - juego.ultimoCambioNota >= 150) {
        juego.ultimoCambioNota = ahora;
        if (juego.notaMusicaActual % 2 == 0) {
          int melodia[] = {262, 330, 392, 523, 392, 330};
          int numNotas = sizeof(melodia) / sizeof(melodia[0]);
          tone(BUZZER, melodia[(juego.notaMusicaActual / 2) % numNotas]);
        } else {
          noTone(BUZZER);
        }
        juego.notaMusicaActual++;
      }
    }
  }

  if (juego.comiendo && (ahora - juego.inicioAnimBoca >= ANIM_BOCA_MS)) {
    juego.comiendo = false;
    redibujarCelda(juego.serpiente[0].x, juego.serpiente[0].y);
  }

  if (juego.activo && !juego.gameOver) {
    if (ahora - juego.ultimoMovimiento >= VELOCIDAD_MS) {
      juego.ultimoMovimiento = ahora;
      moverSerpiente();
    }
  }

  int x, y;
  if (obtenerTouch(x, y)) {
    manejarTouchJuego(x, y);
  }
}


// ═══════════════════════════════════════════════════════════
//                  MANEJADORES DE TOQUE
// ═══════════════════════════════════════════════════════════

void manejarMenuPrincipal(int x, int y) {
  if      (enRectangulo(x, y, BTN_F1_X1, BTN_F1_Y, BTN_F1_W, BTN_F1_H)) { beepBoton();
  dibujarArchivos();    }
  else if (enRectangulo(x, y, BTN_F1_X2, BTN_F1_Y, BTN_F1_W, BTN_F1_H)) { beepBoton(); dibujarInfo();
  }
  else if (enRectangulo(x, y, BTN_F2_X1, BTN_F2_Y, BTN_F2_W, BTN_F2_H)) { beepBoton(); dibujarTemperatura();
  }
  else if (enRectangulo(x, y, BTN_F2_X2, BTN_F2_Y, BTN_F2_W, BTN_F2_H)) { beepBoton(); dibujarJuego();
  }
}

void manejarArchivos(int x, int y) {
  if      (enRectangulo(x, y, BTN_NOTAS_X, BTN_NOTAS_Y, BTN_NOTAS_W, BTN_NOTAS_H)) { beepBoton();
  dibujarNotas(); }
  else if (enRectangulo(x, y, BTN_VOL_X,   BTN_VOL_Y,   BTN_VOL_W,   BTN_VOL_H))   { beepBoton();
  dibujarMenu();  }
}

void manejarBotonVolver(int x, int y) {
  if (enRectangulo(x, y, BTN_VOL_X, BTN_VOL_Y, BTN_VOL_W, BTN_VOL_H)) {
    beepBoton();
    dibujarMenu();
  }
}

// ═══════════════════════════════════════════════════════════
//                    COMPONENTES UI
// ═══════════════════════════════════════════════════════════

void dibujarHeader(const char* titulo) {
  tft.fillRect(0, HEADER_Y, ANCHO_PANTALLA, HEADER_H, COLOR_HEADER);
  tft.setTextColor(COLOR_HEADER_TEXTO);
  tft.setTextSize(1);
  tft.setCursor(8, 11);
  tft.print(titulo);
  tft.drawFastHLine(0, HEADER_H, ANCHO_PANTALLA, COLOR_HEADER_LINE);
}

void dibujarBoton(int x, int y, int ancho, int alto,
                  const char* etiqueta, const char* subtitulo) {
  tft.fillRect(x, y, ancho, alto, COLOR_BOTON);
  tft.drawRect(x,     y,     ancho,     alto,     COLOR_BORDE);
  tft.drawRect(x + 2, y + 2, ancho - 4, alto - 4, COLOR_BORDE_INT);

  tft.setTextColor(COLOR_HEADER_TEXTO);
  tft.setTextSize(2);
  int16_t tx = x + (ancho / 2) - (strlen(etiqueta) * 6);
  int16_t ty = y + (alto / 2) - 12;
  tft.setCursor(tx, ty);
  tft.print(etiqueta);

  tft.setTextColor(COLOR_TEXTO_SEC);
  tft.setTextSize(1);
  int16_t sx = x + (ancho / 2) - (strlen(subtitulo) * 3);
  tft.setCursor(sx, ty + 18);
  tft.print(subtitulo);
}

void dibujarBotonVolver() {
  tft.fillRect(BTN_VOL_X, BTN_VOL_Y, BTN_VOL_W, BTN_VOL_H, 0x0000); // Fondo negro puro
  tft.drawRect(BTN_VOL_X, BTN_VOL_Y, BTN_VOL_W, BTN_VOL_H, COLOR_BORDE_VOL);
  tft.setTextColor(COLOR_HEADER_TEXTO);
  tft.setTextSize(1);
  tft.setCursor(BTN_VOL_X + 10, BTN_VOL_Y + 13);
  tft.print("< VOLVER AL MENU");
}