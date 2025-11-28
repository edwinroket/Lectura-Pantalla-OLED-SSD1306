#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ---------- OLED 128x32 ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET   -1
#define I2C_ADDR     0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- HX711 ----------
#define HX_DT   14  // D5 (GPIO14)
#define HX_SCK  12  // D6 (GPIO12)
HX711 scale;

// ---------- Calibración ----------
float CALIBRATION_FACTOR = -21.983999; // AJUSTAR

// ---------- MQTT + WiFi ----------
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";
const char* mqtt_server = "broker.hivemq.com";
const int   mqtt_port = 1883;
const char* mqtt_topic = "/peso/talca/00";

WiFiClient espClient;
PubSubClient client(espClient);

// ---------- Funciones ----------
void showMsg(const String& l1, const String& l2="") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(l1);
  display.setCursor(0,16);
  display.println(l2);
  display.display();
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Intentando conectar MQTT...");
    String clientId = "ESP8266-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT conectado!");
    } else {
      Serial.print("Fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Reintentando en 3s...");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // I2C manual
  Wire.begin(4, 5);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDR)) {
    Serial.println("No se pudo iniciar la OLED");
  } else {
    showMsg("Balanza IoT", "Iniciando...");
  }

  // WiFi
  WiFi.begin(ssid, password);
  showMsg("Conectando WiFi", ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  showMsg("WiFi Conectado!", WiFi.localIP().toString());
  Serial.println("WiFi OK");

  // MQTT
  client.setServer(mqtt_server, mqtt_port);

  // HX711
  scale.begin(HX_DT, HX_SCK);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();

  if (!scale.wait_ready_timeout(2000)) {
    showMsg("HX711 sin señal", "Revisa VCC/DT/SCK");
    Serial.println("HX711 no listo");
  } else {
    showMsg("Listo", "Coloca objeto...");
    Serial.println("Balanza lista.");
  }
}

void loop() {
  // Mantener MQTT conectado
  if (!client.connected()) reconnectMQTT();
  client.loop();

  if (scale.is_ready()) {
    float weight = scale.get_units(1);
    float weight_kg = weight / 1000;

    // Mostrar en Serial
    Serial.print("Peso: ");
    Serial.print(weight_kg, 2);
    Serial.println(" Kg");

    // Publicar MQTT
    char buffer[10];
    dtostrf(weight_kg, 1, 2, buffer);
    client.publish(mqtt_topic, buffer);

    // OLED
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("Peso actual");
    display.setTextSize(2);
    display.setCursor(6,14);
    display.print(weight_kg, 2);
    display.print(" kg");
    display.display();

  } else {
    showMsg("HX711 sin señal", "Revisa conexiones");
    Serial.println("Error HX711");
  }

  delay(500);
}
