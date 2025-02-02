#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>

// Definir pines
#define DHTPIN 4       // GPIO4 para el DHT22
#define DHTTYPE DHT22  // Tipo de sensor
#define LDR_PIN 36     // GPIO36 para el LDR
#define SOIL_PIN 34    // GPIO34 para el sensor de humedad del suelo

// Dimensiones de la pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_BMP280 bmp; // I2C

// Instancia del sensor DHT y pantalla OLED
DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  dht.begin();

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
  display.println("Sistema listo!");
  display.display();
  delay(2000);
}

void loop() {
  // Leer sensores
  float temperatureDHT = dht.readTemperature();
  float humidityDHT = dht.readHumidity();

  int ldrValue = analogRead(LDR_PIN);
  float voltageLDR = ldrValue * (3.3 / 4095.0);

  int soilValue = analogRead(SOIL_PIN);
  float soilPercentage = map(soilValue, 4095, 1000, 0, 100); // Ajustar según calibración

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

  // Mostrar datos en la OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);

  display.print("Temp: "); display.print((temperatureDHT+bmp.readTemperature())/2); display.println("C");

  display.print("Hum: "); display.print(humidityDHT); display.println(" %");

  //BMP
  display.print(F("Presion: "));display.print(bmp.readPressure());display.println(" Pa");
  display.print(F("Altitud: "));display.print(bmp.readAltitude(1013.25)); display.println(" m");

  //display.println("-------------------");
  display.print("Luz: "); display.print(voltageLDR); display.println(" V");
  display.print("H. Suelo: "); display.print(soilPercentage); display.println(" %");

  display.display();
  delay(2000);
}