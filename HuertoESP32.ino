#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

//TELEGRAM
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include "secrets.h"
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);


// Definir pines
#define DHTPIN 4       // GPIO4 para el DHT22
#define DHTTYPE DHT22  // Tipo de sensor
#define LDR_PIN 36     // GPIO36 para el LDR
#define SOIL_PIN 34    // GPIO34 para el sensor de humedad del suelo
#define BUTTON_PIN 15  // GPIO15


// Dimensiones de la pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// Para encender apagar la oled
unsigned long oledOnTime = 0;
bool oledActive = false;

Adafruit_BMP280 bmp; // I2C

// Instancia del sensor DHT y pantalla OLED
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  dht.begin();

  //CONECTARSE A WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected"); 

  //LANZANDO MENSAJE DE TELEGRAM
  bool success = bot.sendMessage(CHAT_ID, "Hello World!");
  if (success) {
    Serial.println("Mensaje enviado!");
  } else {
    Serial.println("Error al enviar mensaje");
  }

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Botón con pull-up interno

  // Inicializar OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error: No se encontró la pantalla OLED");
    while (1);
  }

  //BMP
  while ( !Serial ) delay(100);   // wait for native usb
  unsigned status;
  status = bmp.begin();
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }
  /* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  // Mensaje inicial en la OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("Bienvenido Sr.Hugo");
  display.println("LISTO archivos");
  display.display();
  delay(2000);
  display.println("LISTO duendes magicos");
  display.display();
  delay(2000);
  display.println("LISTO reservas de orin");
  display.display();
  delay(2000);
  display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void loop() {
  // Leer sensores
  float temperatureDHT = dht.readTemperature();
  float humidityDHT = dht.readHumidity();

  int ldrValue = analogRead(LDR_PIN);
  float voltageLDR = ldrValue * (3.3 / 4095.0);

  int soilValue = analogRead(SOIL_PIN);
  float soilPercentage = map(soilValue, 4095, 1000, 0, 100); // Ajustar según calibración
  soilPercentage = constrain(soilPercentage, 0, 100); // Limitar el valor a un rango de 0-100 para evitar lecturas mayores a 100

  // Mostrar en el monitor serie
  Serial.println("=== Datos del Huerto ===");
  Serial.print("Temp: "); Serial.print((temperatureDHT+bmp.readTemperature())/2); Serial.println("°C");
  //DHT
  //Serial.print("DHT22 - Temp: "); Serial.print(temperatureDHT); Serial.println("°C");
  Serial.print("DHT22 - Humedad: "); Serial.print(humidityDHT); Serial.println("%");
  //BMP
  //Serial.print(F("Temperature = "));Serial.print(bmp.readTemperature());Serial.println(" *C");
  Serial.print(F("Pressure = "));Serial.print(bmp.readPressure());Serial.println(" Pa");
  Serial.print(F("Approx altitude = "));Serial.print(bmp.readAltitude(1013.25)); Serial.println(" m");
  //LDR
  Serial.print("Luminosidad (voltios): "); Serial.println(voltageLDR);
  Serial.print("Humedad del suelo: "); Serial.print(soilPercentage); Serial.println("%");
  Serial.println("=========================");

  //OLED
  // Si se presiona el botón, activamos la pantalla por 1 minuto
  if (digitalRead(BUTTON_PIN) == LOW && !oledActive) {
    oledActive = true;
    oledOnTime = millis();
    display.ssd1306_command(SSD1306_DISPLAYON);  // Encender OLED
  }
  // Si está activa y ya pasó un minuto, la apagamos
  if (oledActive && (millis() - oledOnTime > 30000)) {
    oledActive = false;
    display.clearDisplay();
    display.display();
    display.ssd1306_command(SSD1306_DISPLAYOFF);  // Apagar OLED
  }
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Botón presionado!");
  } else {
    Serial.println("Botón suelto");
  }
  // Solo mostrar info en la OLED si está activa
  if (oledActive) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);

    display.print("Temp: "); display.print((temperatureDHT + bmp.readTemperature()) / 2); display.println("C");

    display.print("Hum: "); display.print(humidityDHT); display.println(" %");

    display.print(F("Presion: ")); display.print(bmp.readPressure()); display.println(" Pa");
    display.print(F("Altitud: ")); display.print(bmp.readAltitude(1013.25)); display.println(" m");

    display.print("Luz: "); display.print(voltageLDR); display.println(" V");
    display.print("H. Suelo: "); display.print(soilPercentage); display.println(" %");

    display.display();
  }
  delay(2000);

}