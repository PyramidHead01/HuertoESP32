#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Definir pines
#define DHTPIN 4       // GPIO4 para el DHT22
#define DHTTYPE DHT22  // Tipo de sensor
#define LDR_PIN 36     // GPIO36 para el LDR
#define SOIL_PIN 34    // GPIO34 para el sensor de humedad del suelo

// Dimensiones de la pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

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
  Serial.print("DHT22 - Temp: "); Serial.print(temperatureDHT); Serial.println("°C");
  Serial.print("DHT22 - Humedad: "); Serial.print(humidityDHT); Serial.println("%");
  Serial.print("Luminosidad (voltios): "); Serial.println(voltageLDR);
  Serial.print("Humedad del suelo: "); Serial.print(soilPercentage); Serial.println("%");
  Serial.println("=========================");

  // Mostrar datos en la OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);

  display.println("DHT22:");
  display.print("Temp: "); display.print(temperatureDHT); display.println(" C");
  display.print("Hum: "); display.print(humidityDHT); display.println(" %");

  display.println("-------------------");
  display.print("Luz: "); display.print(voltageLDR); display.println(" V");
  display.print("H. Suelo: "); display.print(soilPercentage); display.println(" %");

  display.display();
  delay(2000);
}