#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// --- Configuración WiFi ---
const char* ssid = "FLIA_DIAZ";       
const char* password = "77777777";  

// --- Dirección del servidor PHP ---
const char* serverUrl = "http://192.168.100.40/recibir_datos.php";

// --- Definición de pines ---
#define DHTPIN 4        // Pin del DHT22
#define DHTTYPE DHT22
#define LM393_PIN 26     // Pin digital del sensor de ruido
#define MQ2_PIN 33       // Pin analógico del sensor de gas MQ-2

// --- Configuración del GPS ---
#define RXD2 16   // RX del ESP32 conectado al TX del GPS
#define TXD2 17   // TX del ESP32 conectado al RX del GPS

HardwareSerial gpsSerial(2);   // UART2 para GPS
TinyGPSPlus gps;

// --- Inicialización del DHT22 ---
DHT dht(DHTPIN, DHTTYPE);

// --- Setup ---
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(LM393_PIN, INPUT);
  pinMode(MQ2_PIN, INPUT);

  // --- Inicio GPS ---
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("=== Lectura de sensores + GPS con ESP32 ===");

  // --- Conexión WiFi ---
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n Conectado al WiFi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
  delay(1000);
}

// --- Función para leer coordenadas GPS ---
void leerGPS(float &latitud, float &longitud) {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid()) {
    latitud = gps.location.lat();
    longitud = gps.location.lng();
  } else {
    latitud = 0.0;
    longitud = 0.0;
  }
}

// --- Loop principal ---
void loop() {
  // --- Lectura del GPS ---
  float latitud = 0.0, longitud = 0.0;
  leerGPS(latitud, longitud);

  if (latitud == 0.0 && longitud == 0.0) {
    Serial.println(" Esperando señal GPS...");
  } else {
    Serial.print(" Latitud: ");
    Serial.print(latitud, 6);
    Serial.print(" | Longitud: ");
    Serial.println(longitud, 6);
  }

  // --- Lecturas del DHT22 ---
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();

  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println("Error al leer el sensor DHT22");
  } else {
    Serial.print(" Temperatura: ");
    Serial.print(temperatura, 1);
    Serial.println(" °C");

    Serial.print(" Humedad: ");
    Serial.print(humedad, 1);
    Serial.println(" %");
  }

  // --- Lectura del sensor de ruido ---
  int estadoRuido = digitalRead(LM393_PIN);
  bool hayRuido = (estadoRuido == LOW);

  if (hayRuido) {
    Serial.println(" Ruido detectado");
  } else {
    Serial.println(" Ambiente silencioso");
  }

  // --- Lectura del sensor de gas ---
  int valorGas = analogRead(MQ2_PIN);
  float voltajeGas = (valorGas / 4095.0) * 3.3;

  String nivelGas;
  if (voltajeGas < 0.5) {
    nivelGas = "Aire limpio o muy poco gas";
  } else if (voltajeGas < 1.5) {
    nivelGas = "Baja concentración de gas";
  } else if (voltajeGas < 2.5) {
    nivelGas = "Concentración moderada de gas";
  } else {
    nivelGas = " Alta concentración de gas";
  }

  Serial.print(" Sensor gas: ");
  Serial.print(voltajeGas, 2);
  Serial.print(" V → ");
  Serial.println(nivelGas);

  // --- Línea resumen ---
  Serial.print("DATA;");
  Serial.print(temperatura, 1); Serial.print(";");
  Serial.print(humedad, 1); Serial.print(";");
  Serial.print(voltajeGas, 2); Serial.print(";");
  Serial.print(hayRuido ? "1" : "0"); Serial.print(";");
  Serial.print(latitud, 6); Serial.print(";");
  Serial.println(longitud, 6);
  Serial.println("------------------------------------");

  // --- Envío de datos al servidor ---
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"temperatura\": " + String(temperatura, 1) +
                      ", \"humedad\": " + String(humedad, 1) +
                      ", \"voltaje_gas\": " + String(voltajeGas, 2) +
                      ", \"ruido\": " + String(hayRuido ? 1 : 0) +
                      ", \"latitud\": " + String(latitud, 6) +
                      ", \"longitud\": " + String(longitud, 6) + "}";

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      Serial.print(" Datos enviados. Respuesta del servidor: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("❌ Error al enviar datos: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("⚠️ WiFi desconectado");
  }

  delay(3000);
}
