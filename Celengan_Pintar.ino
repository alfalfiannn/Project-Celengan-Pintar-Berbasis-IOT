
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Kredensial WiFi & Telegram
#define WIFI_SSID "21.41"
#define WIFI_PASSWORD "112345678"
#define BOT_TOKEN "8995747803:AAFaY9uHJ5PT14lcSq5YEX_mVRm9aXa-3BM"
#define CHAT_ID "1463282312"

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Inisialisasi LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Pin Push Button
#define BTN_5K D3
#define BTN_10K D4
#define BTN_15K D7
#define BTN_RESET D8

// Pin Buzzer
#define BUZZER_PIN D0

// Pin Sensor Ultrasonik
#define TRIG_PIN D5
#define ECHO_PIN D6

// Variabel Sistem
unsigned long totalSaldo = 0;
bool isFull = false;
unsigned long lastSensorCheck = 0;
const long sensorInterval = 2000;

// Timer LCD
unsigned long lastLcdUpdate = 0;
bool isTempMessage = false;
const long lcdTempDuration = 2000;

bool lastState5k = HIGH;
bool lastState10k = HIGH;
bool lastState15k = HIGH;
bool lastStateReset = LOW;

// ===================================
// FUNGSI BUZZER
// ===================================
void bunyiBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
}

// ===================================
// FUNGSI LCD
// ===================================
void updateLCD(String baris1, String baris2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(baris1);
  lcd.setCursor(0, 1);
  lcd.print(baris2);
}

// ===================================
// SENSOR ULTRASONIK
// ===================================
long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return 999;

  return duration * 0.034 / 2;
}

// ===================================
// TAMBAH SALDO
// ===================================
void tambahSaldo(unsigned long nominal) {

  totalSaldo += nominal;

  if (!isFull) {
    updateLCD("+Rp " + String(nominal),
              "Total:Rp " + String(totalSaldo));

    lastLcdUpdate = millis();
    isTempMessage = true;
  }

  String pesan = "💰 Dana Masuk: Rp " + String(nominal) + "\n";
  pesan += "💳 Total Saldo saat ini: Rp " + String(totalSaldo);

  bot.sendMessage(CHAT_ID, pesan, "");
}

// ===================================
// RESET SALDO
// ===================================
void resetSaldo() {

  totalSaldo = 0;

  updateLCD("RESET SALDO", "Total:Rp 0");

  lastLcdUpdate = millis();
  isTempMessage = true;

  bot.sendMessage(CHAT_ID,
    "🔄 Saldo celengan telah di-reset menjadi Rp 0.", "");
}

// ===================================
// SETUP
// ===================================
void setup() {

  Serial.begin(115200);

  pinMode(BTN_5K, INPUT_PULLUP);
  pinMode(BTN_10K, INPUT_PULLUP);
  pinMode(BTN_15K, INPUT_PULLUP);

  // Tombol reset mengikuti konfigurasi awal
  pinMode(BTN_RESET, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Wire.begin(D2, D1);

  lcd.init();
  lcd.backlight();

  updateLCD("Menghubungkan", "ke WiFi...");

  // Koneksi WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  client.setInsecure();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  updateLCD("Celengan Pintar", "Sistem Siap!");

  bot.sendMessage(CHAT_ID,
    "✅ Sistem Celengan Pintar Aktif!", "");

  delay(2000);

  updateLCD("Celengan Pintar",
            "Total:Rp " + String(totalSaldo));
}

// ===================================
// LOOP
// ===================================
void loop() {

  // 1. Tombol Rp5.000
  bool currentState5k = digitalRead(BTN_5K);

  if (lastState5k == HIGH && currentState5k == LOW) {
    bunyiBeep();
    tambahSaldo(5000);
    delay(200);
  }

  lastState5k = currentState5k;

  // 2. Tombol Rp10.000
  bool currentState10k = digitalRead(BTN_10K);

  if (lastState10k == HIGH && currentState10k == LOW) {
    bunyiBeep();
    tambahSaldo(10000);
    delay(200);
  }

  lastState10k = currentState10k;

  // 3. Tombol Rp15.000
  bool currentState15k = digitalRead(BTN_15K);

  if (lastState15k == HIGH && currentState15k == LOW) {
    bunyiBeep();
    tambahSaldo(15000);
    delay(200);
  }

  lastState15k = currentState15k;

  // 4. Tombol Reset
  bool currentStateReset = digitalRead(BTN_RESET);

  if (lastStateReset == LOW && currentStateReset == HIGH) {
    bunyiBeep();
    resetSaldo();
    delay(200);
  }

  lastStateReset = currentStateReset;

  // 5. Pengembalian tampilan LCD
  unsigned long currentMillis = millis();

  if (isTempMessage && !isFull) {

    if (currentMillis - lastLcdUpdate >= lcdTempDuration) {

      isTempMessage = false;

      updateLCD("Celengan Pintar",
                "Total:Rp " + String(totalSaldo));
    }
  }

  // 6. Sensor Ultrasonik
  if (currentMillis - lastSensorCheck >= sensorInterval) {

    lastSensorCheck = currentMillis;

    long distance = readDistance();

    if (distance > 0 && distance < 5) {

      if (!isFull) {

        isFull = true;
        isTempMessage = false;

        updateLCD("PERINGATAN!",
                  "Celengan PENUH!");

        bot.sendMessage(CHAT_ID,
          "⚠️ PERINGATAN! Celengan sudah penuh.", "");
      }

    } else {

      if (isFull) {

        isFull = false;

        updateLCD("Celengan Pintar",
                  "Total:Rp " + String(totalSaldo));

        bot.sendMessage(CHAT_ID,
          "✅ Celengan kembali kosong/aman.", "");
      }
    }
  }
}
