#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

// TELEGRAM
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "secrets.h"
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Tiempo NTP para saber la hora
#include "time.h"
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;         // España GMT+1
const int daylightOffset_sec = 3600;     // +1h en verano

// Pines
#define DHTPIN 4      // DHT22
#define DHTTYPE DHT22 // Tipo de sensor
#define LDR_PIN 36    // LDR
#define SOIL_PIN 34   // Sensor de humedad del suelo
#define BUTTON_PIN 33 // GPIO33
#define BATT_PIN 35   // Leer estado bateria
#define LED_VERDE    16
#define LED_AMARILLO 17
#define LED_ROJO     18

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool oledActive = false;
unsigned long oledOnTime = 0;

// Sensores
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

// Rango de horas de funcionamiento (18:00 a 23:59)
const int activeHourStart = 6;
const int activeHourEnd = 23;

// Datos guardados
float tempFinal = 0;
float humidityDHT = 0;
float pressure = 0;
float altitude = 0;
float voltageLDR = 0;
float soilPercentage = 0;
int porcBateria;



float leerVoltajeBateria() {
  int raw = analogRead(BATT_PIN);
  float voltaje = raw * (3.3 / 4095.0); // Convierte a voltaje ADC
  voltaje = voltaje * 2; // Porque el divisor de voltaje divide entre 2
  return voltaje;
}

int porcentajeBateria(float voltaje) {
  // Ajusta los valores para tu batería LiPo de 1 celda (3.0V = 0%, 4.2V = 100%)
  if (voltaje >= 4.2) return 100;
  if (voltaje <= 3.0) return 0;
  return (int)((voltaje - 3.0) * 100.0 / (4.2 - 3.0));
}

void mostrarEstadoBateria(int porcentaje) {
  
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);

  if (porcentaje >= 75) {
    Serial.println("LED VERDE");
    digitalWrite(LED_VERDE, HIGH);
  } else if (porcentaje >= 30) {
    Serial.println("LED AMARILLO");
    digitalWrite(LED_AMARILLO, HIGH);
  } else {
    Serial.println("LED ROJO");
    digitalWrite(LED_ROJO, HIGH);
  }
}

void mostrarDatosEnOLED(){

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE); // ← NECESARIO
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Temp: "); display.print(tempFinal); display.println(" C");
  display.print("Hum: "); display.print(humidityDHT); display.println(" %");
  display.print("Presion: "); display.print(pressure); display.println(" Pa");
  display.print("Altitud: "); display.print(altitude); display.println(" m");
  display.print("Luz: "); display.print(voltageLDR); display.println(" V");
  display.print("Suelo: "); display.print(soilPercentage); display.println(" %");
  display.print("Bateria: "); display.print(porcBateria); display.println(" %");
  display.print("OLED se apagara en 30 seg");
  display.display();
  delay(30000); // 30 segundos de OLED encendida
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Despertado por botón. Mostrando OLED.");

    delay(100);  // <-- Dale tiempo a I2C

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println("No se encontró la pantalla OLED");
      while (1);
    }
    else{
      Serial.println("Encontrado la pantalla OLED");
    }

    bateria();
    mostrarEstadoBateria(porcBateria);

    medirSensores(false);
    mostrarDatosEnOLED();
    entrarEnDeepSleep();
    return;
  }
  
  // Conexión WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Obtener hora del servidor NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Error al obtener la hora");
    entrarEnDeepSleep();
  }

  int horaActual = timeinfo.tm_hour;
  Serial.printf("Hora actual: %02d:%02d\n", horaActual, timeinfo.tm_min);

  medirSensores(true);
  enviarTelegram();

  mostrarEstadoBateria(porcBateria);

  entrarEnDeepSleep();

}

void loop() {
  // No se usa, todo ocurre en setup
}

void bateria(){
  float voltBateria = leerVoltajeBateria();
  porcBateria = porcentajeBateria(voltBateria);
}

void BMP(){
  if (!bmp.begin()) {
    Serial.println("No se encontró el BMP280");
    while (1) delay(10);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
}

void medirSensores(bool calcularBateria) {
  
  // Para que los sensores esten correctamente listos
  delay(3000);
  
  // Preparar BMP y DHT
  BMP();
  dht.begin();
  
  // Temperatura
  float tempDHT = dht.readTemperature();
  float tempBMP = bmp.readTemperature();
  if (!isnan(tempDHT) && !isnan(tempBMP)) {
    tempFinal = (tempDHT + tempBMP) / 2;
  } else if (!isnan(tempDHT)) {
    Serial.println("BMP Fallo al iniciarse");
    tempFinal = tempDHT;
  } else if (!isnan(tempBMP)) {
    Serial.println("DHT Fallo al iniciarse");
    tempFinal = tempBMP;
  } else {
    Serial.println("DHT y BMP Fallaron al iniciarse");
    tempFinal = NAN;  // Ambos fallaron
  }
  
  // Humeadad
  humidityDHT = dht.readHumidity();
  
  // Presion
  pressure = bmp.readPressure();

  // Altitud
  altitude = bmp.readAltitude(1013.25);

  // LDR
  voltageLDR = analogRead(LDR_PIN) * (3.3 / 4095.0);
  
  // Humedad suelo
  int soilValue = analogRead(SOIL_PIN);
  soilPercentage = map(soilValue, 4095, 1000, 0, 100);
  soilPercentage = constrain(soilPercentage, 0, 100);
  
  // Bateria
  if (calcularBateria){
    bateria();
  }

  Serial.println("Lectura completa.");
}

void enviarTelegram() {
  String mensaje = "🌿 *Reporte Huerto:*\n";
  mensaje += "🌡️ Temp: " + String(tempFinal) + " °C\n";
  mensaje += "💧 Humedad: " + String(humidityDHT) + " %\n";
  mensaje += "📈 Presion: " + String(pressure) + " Pa\n";
  mensaje += "🗻 Altitud: " + String(altitude) + " m\n";
  mensaje += "🔆 Luz: " + String(voltageLDR) + " V\n";
  mensaje += "🌱 Suelo: " + String(soilPercentage) + " %\n";
  mensaje += "🔋 Bateria: " + String(porcBateria) + " %\n";

  bool ok = bot.sendMessage(CHAT_ID, mensaje, "Markdown");
  if (ok) {
    Serial.println("Mensaje Telegram enviado");
  } else {
    Serial.println("Error al enviar por Telegram");
  }
}



void entrarEnDeepSleep() {

  //digitalWrite(LED_VERDE, LOW);
  //digitalWrite(LED_AMARILLO, LOW);
  //digitalWrite(LED_ROJO, LOW);

  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0); // 0 = LOW activ
  Serial.println("Entrando en Deep Sleep por 1 hora...");
  esp_sleep_enable_timer_wakeup(3600000000ULL); // 1 hora en microsegundos
  esp_deep_sleep_start();
}
