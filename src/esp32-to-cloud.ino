#include <WiFi.h>
#include <PubSubClient.h>

// Configuracion Wi-Fi
const char* WIFI_SSID = "DIEGO";
const char* WIFI_PASSWORD = "diego123";

// IP PUBLICA de la VM donde corre EMQX
const char* MQTT_SERVER = "191.235.32.38";
const int MQTT_PORT = 1883;

// Usuario y clave creados en EMQX (Access Control -> Authentication)
const char* MQTT_USER = "esp32";
const char* MQTT_PASSWORD = "r00tX!";

const char* MQTT_TOPIC = "ta3/distancia";

const int TRIG_PIN = 25;
const int ECHO_PIN = 26;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long ultimaLectura = 0;
const unsigned long INTERVALO = 10000;

void conectarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Conectando al Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi conectado");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  while (!mqtt.connected()) {
    String id = "ESP32-ultrasonico-" +
                String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.print("Conectando a EMQX...");

    if (mqtt.connect(id.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println(" conectado");
    } else {
      Serial.print(" Error: ");
      Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Tiempo maximo de espera: 30 ms
  unsigned long duracion =
      pulseIn(ECHO_PIN, HIGH, 30000);

  if (duracion == 0) {
    return -1.0;
  }

  // Velocidad del sonido: aproximadamente 0.0343 cm/us
  return duracion * 0.0343 / 2.0;
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  conectarWiFi();

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setBufferSize(256);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!mqtt.connected()) {
    conectarMQTT();
  }

  mqtt.loop();

  if (millis() - ultimaLectura >= INTERVALO) {
    ultimaLectura = millis();

    float distancia = medirDistancia();

    if (distancia < 0) {
      Serial.println("No se recibio el eco del sensor");
      return;
    }

    char mensaje[100];

    snprintf(mensaje, sizeof(mensaje),
      "{\"distancia_cm\":%.2f}",
      distancia);

    Serial.print("Distancia: ");
    Serial.print(distancia);
    Serial.println(" cm");

    if (mqtt.publish(MQTT_TOPIC, mensaje)) {
      Serial.print("MQTT publicado: ");
      Serial.println(mensaje);
    } else {
      Serial.println("Error al publicar");
    }
  }
}