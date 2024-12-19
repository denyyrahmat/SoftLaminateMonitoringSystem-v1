/*
 * PERINGATAN!!!!!
 * JANGAN LUPA UNTUK TETAP MEMASUKKAN PROGRAM DARI "ElegantOTA"
 * AGAR DAPAT UPLOAD PROGRAM SECARA WIRELESS!!!!!
 *
// * UNTUK UPLOAD FILE MASUK KE LAMAN "http://<IP_ADDRESS>/update"
 */

#include <Arduino.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include "pitches.h"
#include <EEPROM.h>
#include <Adafruit_VL53L0X.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <PubSubClient.h>

// Wifi cridentials
const char *SSID = "Haast";
const char *SSID_PASS = "qwerty00";
const int MAX_RETRIES = 5;
int RETRY_COUNT = 0;

WebServer server(80);

// Konfigurasi LCD
#define LCD_ADDRESS_1 0X27
#define LCD_ADDRESS_2 0x26
#define LCD_COLUMNS 16
#define LCD_ROWS 2

const int ONE_WIRE_BUS = 27;
const int PB_RESET_PIN = 34;
const int RED_LED_PIN = 4;
const int BLUE_LED_PIN = 23;
const int BUZZER_PIN = 2;
const int XSHUT1 = 32;
const int XSHUT2 = 25;
const int XSHUT3 = 26;

float ROLL1_DIST, ROLL2_DIST, ROLL3_DIST;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress DRY_ZONE_SENSOR = {0x28, 0x1, 0xE3, 0x35, 0x0, 0x0, 0x0, 0x74};
DeviceAddress ROLL_SENSOR = {0x28, 0x4F, 0x4F, 0x39, 0x0, 0x0, 0x0, 0x12};

// Konfigurasi LCD
LiquidCrystal_I2C DIST_LCD(LCD_ADDRESS_1, LCD_COLUMNS, LCD_ROWS);
LiquidCrystal_I2C TEMP_LCD(LCD_ADDRESS_2, LCD_COLUMNS, LCD_ROWS);

// Objek VL53L0X
Adafruit_VL53L0X ROLL1 = Adafruit_VL53L0X();
Adafruit_VL53L0X ROLL2 = Adafruit_VL53L0X();
Adafruit_VL53L0X ROLL3 = Adafruit_VL53L0X();

unsigned long PREVIOUS_MILLIS = 0;
const long BLINK_INTERVAL = 200;

const long TEMP_READ_INTERVAL = 500;
const long DIST_READ_INTERVAL = 500;

unsigned long PREVIOUS_TEMP_MILLIS = 0;
unsigned long PREVIOUS_DIST_MILLIS = 0;

int EEPROM_TEMP_STATUS_ADDRESS = 0;
int EEPROM_DIST_STATUS_ADDRESS = 1;

void saveEEPROMStatus(int status, int address);
void playErrorTone();

void setup()
{
  DIST_LCD.init();
  DIST_LCD.backlight();
  TEMP_LCD.init();
  TEMP_LCD.backlight();

  delay(1000);

  Serial.begin(115200);

  // Wifi and OTA setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, SSID_PASS);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED && RETRY_COUNT < MAX_RETRIES)
  {
    delay(500);
    Serial.print(".");
    DIST_LCD.clear();
    DIST_LCD.setCursor(1, 0);
    DIST_LCD.print("Connecting to");
    DIST_LCD.setCursor(5, 1);
    DIST_LCD.print("Wifi");

    TEMP_LCD.clear();
    TEMP_LCD.setCursor(1, 0);
    TEMP_LCD.print("Connecting to");
    TEMP_LCD.setCursor(5, 1);
    TEMP_LCD.print("Wifi");

    RETRY_COUNT++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(SSID);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    DIST_LCD.clear();
    DIST_LCD.setCursor(6, 0);
    DIST_LCD.print("Wifi");
    DIST_LCD.setCursor(3, 1);
    DIST_LCD.print("Connected");

    TEMP_LCD.clear();
    TEMP_LCD.setCursor(6, 0);
    TEMP_LCD.print("Wifi");
    TEMP_LCD.setCursor(3, 1);
    TEMP_LCD.print("Connected");

    server.on("/", []()
              { server.send(200, "text/plain", "To upload files go to the URL \"<YOUR_IP>/update\""); });

    ElegantOTA.begin(&server);
    server.begin();
    Serial.println("HTTP server started\n=================");

    pinMode(PB_RESET_PIN, INPUT_PULLUP);
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(XSHUT1, OUTPUT);
    pinMode(XSHUT2, OUTPUT);
    pinMode(XSHUT3, OUTPUT);

    // Matikan semua sensor VL53L0X
    digitalWrite(XSHUT1, LOW);
    digitalWrite(XSHUT2, LOW);
    digitalWrite(XSHUT3, LOW);
    delay(5000); // Waktu tunggu untuk memastikan sensor benar-benar mati

    // Inisialisasi sensor 'ROLL1'
    Serial.println("Mengaktifkan Sensor 1...");
    digitalWrite(XSHUT1, HIGH);
    delay(100);
    if (!ROLL1.begin(0x30))
    { // Set alamat ke 0x30
      Serial.println("Sensor 1 gagal diinisialisasi!");
      while (1)
        ;
    }
    Serial.println("Sensor 1 berhasil diinisialisasi dengan alamat 0x30.");

    // Inisialisasi sensor 'ROLL2'
    Serial.println("Mengaktifkan Sensor 2...");
    digitalWrite(XSHUT2, HIGH);
    delay(100);
    if (!ROLL2.begin(0x31))
    { // Set alamat ke 0x31
      Serial.println("Sensor 2 gagal diinisialisasi!");
      while (1)
        ;
    }
    Serial.println("Sensor 2 berhasil diinisialisasi dengan alamat 0x31.");

    // Inisialisasi sensor 'ROLL3'
    Serial.println("Mengaktifkan Sensor 3...");
    digitalWrite(XSHUT3, HIGH);
    delay(100);
    if (!ROLL3.begin(0x32))
    { // Set alamat ke 0x32
      Serial.println("Sensor 3 gagal diinisialisasi!");
      while (1)
        ;
    }
    Serial.println("Sensor 3 berhasil diinisialisasi dengan alamat 0x32.");

    Serial.println("Semua sensor berhasil diinisialisasi!");

    // Inisialisasi EEPROM
    EEPROM.begin(512);
    int TEMP_STATUS = EEPROM.read(EEPROM_TEMP_STATUS_ADDRESS);
    int DIST_STATUS = EEPROM.read(EEPROM_DIST_STATUS_ADDRESS);

    Wire.begin();

    sensors.begin();
  }
  else
  {
    Serial.print("Connection failed");

    DIST_LCD.clear();
    DIST_LCD.setCursor(4, 0);
    DIST_LCD.print("Connection");
    DIST_LCD.setCursor(4, 1);
    DIST_LCD.print("Failed");

    TEMP_LCD.clear();
    TEMP_LCD.setCursor(4, 0);
    TEMP_LCD.print("Connection");
    TEMP_LCD.setCursor(4, 1);
    TEMP_LCD.print("Failed");
  }
}

void loop()
{
  unsigned long CURRENT_MILLIS = millis();

  VL53L0X_RangingMeasurementData_t measure;

  // Logika Tombol Reset
  if (digitalRead(PB_RESET_PIN) == LOW)
  {
    Serial.println("Resetting ESP32...");
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    tone(BUZZER_PIN, NOTE_D1, 500);
    delay(500);
    digitalWrite(RED_LED_PIN, LOW);
    noTone(BUZZER_PIN);
    ESP.restart();
  }

  bool allSensorsSuccess = true; // Variabel untuk melacak jika semua sensor berhasil

  // Pembacaan Suhu
  if (CURRENT_MILLIS - PREVIOUS_TEMP_MILLIS >= TEMP_READ_INTERVAL)
  {
    PREVIOUS_TEMP_MILLIS = CURRENT_MILLIS;

    sensors.requestTemperatures();
    float DRY_ZONE_TEMP = sensors.getTempC(DRY_ZONE_SENSOR);
    float ROLL_TEMP = sensors.getTempC(ROLL_SENSOR);

    bool TEMP_READ_SUCCESS = (DRY_ZONE_TEMP != DEVICE_DISCONNECTED_C && ROLL_TEMP != DEVICE_DISCONNECTED_C);
    // bool TEMP_READ_SUCCESS = (ROLL_TEMP != DEVICE_DISCONNECTED_C);
    if (TEMP_READ_SUCCESS)
    {
      char DRY_ZONE_TEMP_STR[6], ROLL_TEMP_STR[6];
      dtostrf(DRY_ZONE_TEMP, 4, 1, DRY_ZONE_TEMP_STR);
      dtostrf(ROLL_TEMP, 4, 1, ROLL_TEMP_STR);

      TEMP_LCD.clear();
      TEMP_LCD.setCursor(0, 0);
      TEMP_LCD.print("DRY ZONE");
      TEMP_LCD.setCursor(0, 1);
      TEMP_LCD.print(DRY_ZONE_TEMP_STR);
      TEMP_LCD.print(" C");

      TEMP_LCD.setCursor(10, 0);
      TEMP_LCD.print("ROLL");
      TEMP_LCD.setCursor(10, 1);
      TEMP_LCD.print(ROLL_TEMP_STR);
      TEMP_LCD.print(" C");

      saveEEPROMStatus(1, EEPROM_TEMP_STATUS_ADDRESS);
    }
    else
    {
      allSensorsSuccess = false;       // Tandai kegagalan jika pembacaan suhu gagal
      digitalWrite(BLUE_LED_PIN, LOW); // Matikan LED BIRU jika ada sensor gagal
      TEMP_LCD.clear();
      TEMP_LCD.setCursor(2, 0);
      TEMP_LCD.print("TEMPERATURE");
      TEMP_LCD.setCursor(5, 1);
      TEMP_LCD.print("ERROR");
      playErrorTone();
      saveEEPROMStatus(0, EEPROM_TEMP_STATUS_ADDRESS);
    }
  }

  // Pembacaan Jarak
  if (CURRENT_MILLIS - PREVIOUS_DIST_MILLIS >= DIST_READ_INTERVAL)
  {
    PREVIOUS_DIST_MILLIS = CURRENT_MILLIS;

    bool ALL_DIST_READ_SUCCESS = true;
    // ROLL 1
    ROLL1.rangingTest(&measure, false); // Parameter "false" untuk mode single-shot
    Serial.print("ROLL 1: ");
    if (measure.RangeStatus != 4)
    { // Status 4 berarti tidak ada objek yang terdeteksi
      ROLL1_DIST = measure.RangeMilliMeter;
      Serial.print(ROLL1_DIST);
      Serial.println(" mm");
    }
    else
    {
      Serial.println("Out of range");
      ALL_DIST_READ_SUCCESS = false;
    }

    // ROLL 2
    ROLL2.rangingTest(&measure, false);
    Serial.print("ROLL 2: ");
    if (measure.RangeStatus != 4)
    {
      ROLL2_DIST = measure.RangeMilliMeter;
      Serial.print(ROLL2_DIST);
      Serial.println(" mm");
    }
    else
    {
      Serial.println("Out of range");
      ALL_DIST_READ_SUCCESS = false;
    }

    // ROLL 3
    ROLL3.rangingTest(&measure, false);
    Serial.print("ROLL 3: ");
    if (measure.RangeStatus != 4)
    {
      ROLL3_DIST = measure.RangeMilliMeter;
      Serial.print(ROLL3_DIST);
      Serial.println(" mm");
    }
    else
    {
      Serial.println("Out of range");
      ALL_DIST_READ_SUCCESS = false;
    }

    if (ALL_DIST_READ_SUCCESS)
    {
      // Format nilai jarak ke string
      char DISTANCE_MM1_STR[10];
      char DISTANCE_MM2_STR[10];
      char DISTANCE_MM3_STR[10];
      itoa(ROLL1_DIST, DISTANCE_MM1_STR, 10);
      itoa(ROLL2_DIST, DISTANCE_MM2_STR, 10);
      itoa(ROLL3_DIST, DISTANCE_MM3_STR, 10);

      // Menampilkan nilai jarak pada LCD
      DIST_LCD.clear();
      DIST_LCD.setCursor(0, 0);
      DIST_LCD.print("R1:");
      DIST_LCD.setCursor(0, 1);
      DIST_LCD.print(DISTANCE_MM1_STR);
      DIST_LCD.print(" mm");

      DIST_LCD.setCursor(6, 0);
      DIST_LCD.print("R2:");
      DIST_LCD.setCursor(6, 1);
      DIST_LCD.print(DISTANCE_MM2_STR);
      DIST_LCD.print(" mm");

      DIST_LCD.setCursor(12, 0);
      DIST_LCD.print("R3:");
      DIST_LCD.setCursor(12, 1);
      DIST_LCD.print(DISTANCE_MM3_STR);
      DIST_LCD.print(" mm");

      saveEEPROMStatus(1, EEPROM_DIST_STATUS_ADDRESS);
    }
    else
    {
      allSensorsSuccess = false;       // Tandai kegagalan jika pembacaan jarak gagal
      digitalWrite(BLUE_LED_PIN, LOW); // Matikan LED BIRU jika ada sensor gagal
      DIST_LCD.clear();
      DIST_LCD.setCursor(4, 0);
      DIST_LCD.print("DISTANCE");
      DIST_LCD.setCursor(5, 1);
      DIST_LCD.print("ERROR");
      playErrorTone();
      saveEEPROMStatus(0, EEPROM_DIST_STATUS_ADDRESS);
    }
  }

  // Kontrol LED BIRU
  if (allSensorsSuccess)
  { // Hanya nyalakan LED BIRU jika semua sensor berhasil
    if (CURRENT_MILLIS - PREVIOUS_MILLIS >= BLINK_INTERVAL)
    {
      PREVIOUS_MILLIS = CURRENT_MILLIS;
      digitalWrite(BLUE_LED_PIN, !digitalRead(BLUE_LED_PIN)); // Toggl LED BIRU
    }
  }
  else
  {
    digitalWrite(BLUE_LED_PIN, LOW); // Matikan LED BIRU jika ada sensor gagal
  }

  // OTA handle
  server.handleClient();
  ElegantOTA.loop();
}

void saveEEPROMStatus(int status, int address)
{
  if (EEPROM.read(address) != status)
  {
    EEPROM.write(address, status);
    EEPROM.commit();
  }
}

void playErrorTone()
{
  tone(BUZZER_PIN, NOTE_G2, 1000);

  digitalWrite(RED_LED_PIN, HIGH);
  delay(1000);
  digitalWrite(RED_LED_PIN, LOW);
  delay(200);
}