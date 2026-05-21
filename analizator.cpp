#include <Wire.h>
#include <Adafruit_INA219.h>
 
// ==================== CONFIGURARE HARDWARE ====================
#define GATE_PIN 25             // Control direct IRLZ44N
#define SAMPLE_INTERVAL_US 5000 // 5ms între eșantioane 
#define MAX_SAMPLES 300         // Maximul de puncte pentru grafic
 
// ==================== OBIECTE GLOBALE ====================
Adafruit_INA219 ina219;
 
// ==================== VARIABILE MĂSURARE ====================
float voltages[MAX_SAMPLES];
float currents[MAX_SAMPLES];
unsigned long timestamps[MAX_SAMPLES]; 
int sampleCount = 0;
bool isTesting = false;
float calculatedCapacitance = 0; 
 
// Variabile monitorizare live
float prev_volt = -9999;
float prev_curr = -9999;
unsigned long lastTelemetryTime = 0;
 
// ==================== FUNCȚIE CALCUL PRIN INTEGRARE ====================
float calculateCapacitanceIntegral() {
  if (sampleCount < 10) return 0;
 
  float V_start = voltages[0];
  float V_high = V_start * 0.8; 
  float V_low = V_start * 0.4;  
 
  int idx_high = -1;
  int idx_low = -1;
 
  for (int i = 0; i < sampleCount; i++) {
    if (voltages[i] <= V_high && idx_high == -1) idx_high = i;
    if (voltages[i] <= V_low && idx_low == -1) {
      idx_low = i;
      break; 
    }
  }
 
  if (idx_high != -1 && idx_low != -1) {
    float delta_V = voltages[idx_high] - voltages[idx_low];
    float total_charge_mAs = 0; // Sarcina în miliAmperi-secundă
 
    // Adunăm (integrăm) cantitatea de curent din fiecare interval de timp
    for (int i = idx_high; i < idx_low; i++) {
      float dt_sec = (timestamps[i+1] - timestamps[i]) / 1000000.0;
      float avg_current_step = (currents[i] + currents[i+1]) / 2.0; // Media trapezoidală
      total_charge_mAs += (avg_current_step * dt_sec);
    }
 
    // C = Q / V
    float charge_Coulombs = total_charge_mAs / 1000.0;
    float capacitance_F = charge_Coulombs / delta_V;
    return capacitance_F * 1000000.0; // uF
  }
  return 0; 
}
 
// ==================== TELEMETRIE JSON ====================
void sendTelemetry(String status, float voltage, float current, float capacitance_uF) {
  Serial.print("{\"status\":\"" + status + "\"");
  Serial.print(",\"voltage\":" + String(voltage, 3));
  Serial.print(",\"current\":" + String(current, 1));
  Serial.print(",\"capacitance\":" + String(capacitance_uF, 0)); 
  Serial.println("}");
}
 
// ==================== RUTINA DE DESCĂRCARE ====================
void runDischargeTest() {
  if (isTesting) return;
  isTesting = true;
 
  float V_start = ina219.getBusVoltage_V();
  if (V_start < 2.0) { 
    sendTelemetry("ERROR_LOW_VOLTAGE", V_start, 0, 0);
    isTesting = false;
    return;
  }
 
  sendTelemetry("TESTING", V_start, ina219.getCurrent_mA(), 0);
  sampleCount = 0;
 
  // Deschidem tranzistorul la maxim!
  digitalWrite(GATE_PIN, HIGH);
 
  unsigned long startTime = micros();
  unsigned long lastSampleTime = 0;
 
  while (sampleCount < MAX_SAMPLES) {
    unsigned long currentMicros = micros();
 
    if (currentMicros - lastSampleTime >= SAMPLE_INTERVAL_US) {
      float voltage = ina219.getBusVoltage_V();
      float current = ina219.getCurrent_mA();
 
      timestamps[sampleCount] = currentMicros - startTime;
      voltages[sampleCount] = voltage;
      currents[sampleCount] = current;
 
      sampleCount++;
      lastSampleTime = currentMicros;
 
      if (voltage < 0.2) break;
    }
    yield();
  }
 
  // Oprim descărcarea
  digitalWrite(GATE_PIN, LOW);
 
  calculatedCapacitance = calculateCapacitanceIntegral();
 
  if (calculatedCapacitance > 0) {
    Serial.println("\n--- START CURVE DATA ---");
    for (int i = 0; i < sampleCount; i++) {
      Serial.printf("{\"point\":%d,\"time_ms\":%.3f,\"voltage_V\":%.3f,\"current_mA\":%.1f}\n", 
              i, timestamps[i] / 1000.0, voltages[i], currents[i]);
    }
    Serial.println("--- END CURVE DATA ---\n");
    sendTelemetry("FINISHED", ina219.getBusVoltage_V(), 0, calculatedCapacitance);
  } else {
    sendTelemetry("ERROR_CALCULATION", ina219.getBusVoltage_V(), 0, 0);
  }
 
  prev_volt = -9999;
  prev_curr = -9999;
  isTesting = false;
}
 
// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000); // Mărim viteza magistralei I2C
 
  pinMode(GATE_PIN, OUTPUT);
  digitalWrite(GATE_PIN, LOW); // Tranzistorul pornește blocat
 
  if (!ina219.begin()) {
    Serial.println("{\"status\":\"ERROR_INA219\"}");
    while (1) delay(10);
  }
  ina219.setCalibration_32V_2A();
  Serial.println("{\"status\":\"READY\"}");
}
 
// ==================== LOOP ====================
void loop() {
  if (Serial.available() > 0 && !isTesting) {
    char cmd = Serial.read();
    if (cmd == 's' || cmd == 'S') runDischargeTest();
  }
 
  if (!isTesting) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastTelemetryTime > 150) { 
      float volt = ina219.getBusVoltage_V();
      float curr = ina219.getCurrent_mA();
 
      // Am scos filtrul, va trimite date constant spre dashboard
      sendTelemetry("IDLE", volt, curr, calculatedCapacitance);
      lastTelemetryTime = currentMillis;
    }
  }
}
