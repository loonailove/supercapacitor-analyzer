#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <Adafruit_INA219.h>

TFT_eSPI tft = TFT_eSPI();
Adafruit_INA219 ina219;

// Folosim DOAR culorile standard TFT_eSPI - garantat corecte
// Culori custom definite corect pentru RGB565
#define COL_BG        TFT_BLACK
#define COL_HEADER_BG 0x0821    // gri-albastru foarte inchis
#define COL_DIVIDER   0x2945    // albastru-gri
#define COL_TITLE     0x5D9B    // albastru deschis

#define COL_U_ICON    TFT_DARKGREEN
#define COL_U_VAL     TFT_GREEN
#define COL_U_LABEL   0x2704    // verde inchis

#define COL_I_ICON    TFT_NAVY
#define COL_I_VAL     TFT_CYAN
#define COL_I_LABEL   0x000D    // albastru inchis

#define COL_P_ICON    0x6200    // maro-portocaliu inchis
#define COL_P_VAL     TFT_YELLOW
#define COL_P_LABEL   0x4200

#define COL_BAR_BG    0x1082
#define COL_FOOTER    0x2104

float prev_volt = -9999;
float prev_curr = -9999;
float prev_pow  = -9999;
bool  dotOn     = false;

// Layout pentru 480x320 landscape
// Header: 0-35
// Row U:  50-105  (icon 50-98, label 50, val 65, bara 96)
// Sep:    110
// Row I:  118-173 (icon 118-166, label 118, val 133, bara 164)
// Sep:    178
// Row P:  186-241 (icon 186-234, label 186, val 201, bara 232)
// Footer: 294-319

#define SCR_W   320
#define SCR_H   480

#define HDR_H   36
#define FTR_Y   454
#define FTR_H   26

#define ICON_X  12
#define ICON_W  52
#define ICON_H  52

#define CONTENT_X  76
#define CONTENT_W  256   // 480 - 76 - 12

#define ROW1_Y  44
#define ROW2_Y  118
#define ROW3_Y  192

#define BAR_H   6
#define BAR_R   3

void drawStaticUI() {
  // Fundal complet negru - GARANTAT
  tft.fillScreen(TFT_BLACK);
  
  // ── HEADER ──────────────────────────────────────
  tft.fillRect(0, 0, SCR_W, HDR_H, COL_HEADER_BG);
  tft.drawFastHLine(0, HDR_H, SCR_W, COL_DIVIDER);
  
  // Puncte decorative stanga
  tft.fillCircle(16, HDR_H/2, 5, TFT_DARKGREEN);
  tft.fillCircle(30, HDR_H/2, 5, TFT_NAVY);
  
  // Titlu
  tft.setTextDatum(ML_DATUM);  // Middle-Left
  tft.setTextColor(COL_TITLE, COL_HEADER_BG);
  tft.setTextSize(1);
  tft.drawString("ANALIZATOR   INA219", 50, HDR_H/2);
  
  // ── SEPARATOARE INTRE RANDURI ──────────────────
  tft.drawFastHLine(CONTENT_X, ROW1_Y + ICON_H + 8, CONTENT_W, COL_DIVIDER);
  tft.drawFastHLine(CONTENT_X, ROW2_Y + ICON_H + 8, CONTENT_W, COL_DIVIDER);
  
  // ── ICOANE ─────────────────────────────────────
  // U - verde
  tft.fillRoundRect(ICON_X, ROW1_Y, ICON_W, ICON_H, 8, COL_U_ICON);
  tft.setTextColor(TFT_WHITE, COL_U_ICON);
  tft.setTextDatum(MC_DATUM);  // Middle-Center
  tft.setTextSize(3);
  tft.drawString("U", ICON_X + ICON_W/2, ROW1_Y + ICON_H/2);

  // I - albastru
  tft.fillRoundRect(ICON_X, ROW2_Y, ICON_W, ICON_H, 8, COL_I_ICON);
  tft.setTextColor(TFT_WHITE, COL_I_ICON);
  tft.drawString("I", ICON_X + ICON_W/2, ROW2_Y + ICON_H/2);

  // P - portocaliu
  tft.fillRoundRect(ICON_X, ROW3_Y, ICON_W, ICON_H, 8, COL_P_ICON);
  tft.setTextColor(TFT_WHITE, COL_P_ICON);
  tft.drawString("P", ICON_X + ICON_W/2, ROW3_Y + ICON_H/2);

  // ── ETICHETE (mici, deasupra valorii) ──────────
  tft.setTextDatum(ML_DATUM);
  tft.setTextSize(1);
  
  tft.setTextColor(COL_U_LABEL, TFT_BLACK);
  tft.drawString("TENSIUNE", CONTENT_X, ROW1_Y + 8);
  
  tft.setTextColor(COL_I_LABEL, TFT_BLACK);
  tft.drawString("CURENT", CONTENT_X, ROW2_Y + 8);
  
  tft.setTextColor(COL_P_LABEL, TFT_BLACK);
  tft.drawString("PUTERE", CONTENT_X, ROW3_Y + 8);

  // ── FUNDAL BARE ─────────────────────────────────
  tft.fillRoundRect(CONTENT_X, ROW1_Y + ICON_H - BAR_H - 2, CONTENT_W, BAR_H, BAR_R, COL_BAR_BG);
  tft.fillRoundRect(CONTENT_X, ROW2_Y + ICON_H - BAR_H - 2, CONTENT_W, BAR_H, BAR_R, COL_BAR_BG);
  tft.fillRoundRect(CONTENT_X, ROW3_Y + ICON_H - BAR_H - 2, CONTENT_W, BAR_H, BAR_R, COL_BAR_BG);

  // ── FOOTER ──────────────────────────────────────
  tft.fillRect(0, FTR_Y, SCR_W, FTR_H, COL_HEADER_BG);
  tft.drawFastHLine(0, FTR_Y, SCR_W, COL_DIVIDER);
  tft.setTextDatum(ML_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(COL_FOOTER, COL_HEADER_BG);
  tft.drawString("ESP32   I2C 0x40", 12, FTR_Y + FTR_H/2);
  tft.setTextDatum(MR_DATUM);
  tft.drawString("200ms REFRESH", SCR_W - 12, FTR_Y + FTR_H/2);
}

void drawBar(int rowY, float pct, uint16_t color) {
  int barY = rowY + ICON_H - BAR_H - 2;
  if (pct < 0) pct = 0;
  if (pct > 1) pct = 1;
  int filled = (int)(pct * CONTENT_W);
  if (filled > 0)
    tft.fillRoundRect(CONTENT_X, barY, filled, BAR_H, BAR_R, color);
  if (filled < CONTENT_W)
    tft.fillRect(CONTENT_X + filled, barY, CONTENT_W - filled, BAR_H, COL_BAR_BG);
}

void drawValue(int rowY, String val, uint16_t color) {
  // Sterge zona valorii
  tft.fillRect(CONTENT_X, rowY + 18, CONTENT_W, 26, TFT_BLACK);
  tft.setTextDatum(ML_DATUM);
  tft.setTextSize(2);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawString(val, CONTENT_X, rowY + 30);
}

void blinkDot() {
  tft.fillCircle(SCR_W - 20, HDR_H/2, 5, dotOn ? TFT_GREEN : COL_HEADER_BG);
  dotOn = !dotOn;
}

void setup() {
  Serial.begin(115200);
  tft.init();
  
  // IMPORTANT: Încearcă rotația 1 sau 3 pentru landscape
  // Dacă tot e greșit, schimbă în 3
  tft.setRotation(1);
  
  tft.fillScreen(TFT_BLACK);

  if (!ina219.begin()) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("EROARE: INA219 negasit!", SCR_W/2, SCR_H/2 - 20);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Verifica SDA(21) & SCL(22)", SCR_W/2, SCR_H/2 + 10);
    while (1) delay(10);
  }

  drawStaticUI();
  Serial.println("Pornit!");
}

void loop() {
  float volt = ina219.getBusVoltage_V();
  float curr = ina219.getCurrent_mA();
  float powr = ina219.getPower_mW();

  if (abs(volt - prev_volt) > 0.001) {
    drawValue(ROW1_Y, String(volt, 3) + " V ", COL_U_VAL);
    drawBar(ROW1_Y, volt / 5.0, TFT_GREEN);
    prev_volt = volt;
  }

  if (abs(curr - prev_curr) > 0.05) {
    drawValue(ROW2_Y, String(curr, 2) + " mA", COL_I_VAL);
    drawBar(ROW2_Y, abs(curr) / 500.0, TFT_CYAN);
    prev_curr = curr;
  }

  if (abs(powr - prev_pow) > 0.1) {
    drawValue(ROW3_Y, String(powr, 1) + " mW", COL_P_VAL);
    drawBar(ROW3_Y, powr / 2500.0, TFT_YELLOW);
    prev_pow = powr;
  }

  blinkDot();

  Serial.print("V:"); Serial.print(volt,3);
  Serial.print(" I:"); Serial.print(curr,2);
  Serial.print(" P:"); Serial.println(powr,1);

  delay(200);
}