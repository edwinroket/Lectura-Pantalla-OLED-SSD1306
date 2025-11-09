#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

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
float CALIBRATION_FACTOR = 9.121500; // <-- AJUSTAR ESTE VALOR

// ---------- Función para mostrar mensajes en OLED ----------
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

void setup() {
  Serial.begin(115200);

  // I2C manual: SDA=GPIO4, SCL=GPIO5
  Wire.begin(4, 5);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDR)) {
    Serial.println("No se pudo iniciar la OLED");
  } else {
    showMsg("Balanza IoT", "Iniciando...");
  }

  // HX711
  scale.begin(HX_DT, HX_SCK);
  scale.set_scale(CALIBRATION_FACTOR); 
  scale.tare(); // Pone a cero la balanza sin peso

  if (!scale.wait_ready_timeout(2000)) {
    showMsg("HX711 sin señal", "Revisa VCC/DT/SCK");
    Serial.println("HX711 no listo");
  } else {
    showMsg("Listo", "Coloca objeto...");
    Serial.println("Balanza lista. Coloca un objeto para pesar.");
  }
}

void loop() {
  if (scale.is_ready()) {
    // Lectura individual sin promediar
    float weight = scale.get_units(1); // Solo 1 lectura
    float weight_kg = weight / 1000; // Convertir a kilos
    
    // Mostrar en Serial
    Serial.print("Peso: ");
    Serial.print(weight_kg, 2);
    Serial.println(" Kg");
    
    // Mostrar en OLED
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
    Serial.println("Error: HX711 no está listo.");
  }
  
  delay(500); // Refresca cada medio segundo
}