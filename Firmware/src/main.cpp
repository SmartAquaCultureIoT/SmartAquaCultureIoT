#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

// Definisi Pin
#define PH_PIN 34
#define TURBIDITY_PIN 35
#define SERVO_PIN 18
#define BUTTON_PIN 4
#define LED_PIN 2

Servo servoPakan;
unsigned long previousMillis = 0;
const long interval = 2000; // Interval telemetri JSON 2 detik
uint32_t seq = 0;

// Variabel status tombol (Non-blocking debouncing)
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);
  
  pinMode(PH_PIN, INPUT);
  pinMode(TURBIDITY_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  
  servoPakan.attach(SERVO_PIN);
  servoPakan.write(0); // Posisi awal wadah pakan tertutup
  
  Serial.println("Sistem Smart Aquaculture Bengkulu Dimulai...");
}

void loop() {
  unsigned long currentMillis = millis();

  // 1. Eksekusi Tombol Manual (Pembuktian Non-Blocking Instan)
  bool reading = digitalRead(BUTTON_PIN);
  if (reading != lastButtonState) {
    lastDebounceTime = currentMillis;
  }
  
  if ((currentMillis - lastDebounceTime) > debounceDelay) {
    if (reading == LOW) { // Tombol ditekan
      servoPakan.write(90); // Buka wadah pakan
      Serial.println("Pakan manual diberikan! (Respons instan tanpa terhalang delay)");
    } else {
      servoPakan.write(0);  // Tutup kembali
    }
  }
  lastButtonState = reading;

  // 2. Serialisasi JSON & Telemetri Kualitas Air setiap 2 detik
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    seq++;

    // Membaca sensor (Simulasi Potensiometer 0-4095)
    int raw_ph = analogRead(PH_PIN);
    int raw_turbidity = analogRead(TURBIDITY_PIN);

    // Konversi nilai ADC ke satuan nyata
    float ph_value = (raw_ph / 4095.0) * 14.0; // Rentang pH 0-14
    float turbidity_ntu = (raw_turbidity / 4095.0) * 1000.0; // Misal rentang 0-1000 NTU

    // Logika bisnis peringatan (Air Bahaya)
    bool alertStatus = false;
    if (ph_value < 6.5 || ph_value > 8.5 || turbidity_ntu > 400.0) {
      alertStatus = true;
      digitalWrite(LED_PIN, HIGH); // Nyalakan LED peringatan
    } else {
      digitalWrite(LED_PIN, LOW);
    }

    // Membangun Payload JSON Standar
    StaticJsonDocument<256> doc;
    doc["device_id"] = "ESP32_AQUA_BKL_01";
    doc["seq"] = seq;
    doc["uptime_ms"] = currentMillis;
    doc["ph_value"] = ph_value;
    doc["turbidity_ntu"] = turbidity_ntu;
    doc["alert"] = alertStatus;

    // Output JSON telemetri pada Serial Monitor
    serializeJson(doc, Serial);
    Serial.println();
  }
}