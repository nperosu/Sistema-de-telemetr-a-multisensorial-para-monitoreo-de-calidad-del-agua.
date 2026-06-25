/*Código de funcionamiento modificar correspondientes al Wifi y a la etapa en la que se coloca el 
sensor usa: agua_bruta, floculacion, acondicionado,control_calidad):
*/
#include <WiFi.h>
#include <PubSubClient.h>

/*
  Proyecto: Sistemas de telemetría multisensorial para monitoreo de la calidad del agua.
  DFRobot SEN0244 TDS
  Board: FireBeetle 2 ESP32-E
  Signal pin: A2 / GPIO34
  MQTT topic: etap/etapa/ph
*/


#define TDS_PIN 34
#define VREF 3.3
#define ADC_RES 4095.0

float temperature = 25.0;

// WiFi
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";

// MQTT
const char* mqtt_server = "IP_DE_TU_RASPBERRY";
const int mqtt_port = 1883;
const char* mqtt_topic = "etapa/etapa/tds";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long mqttTime = 0;
const unsigned long MQTT_INTERVAL = 2000;

void setup_wifi() {
WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
delay(500);
}

Serial.print("IP: ");
Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
while (!client.connected()) {

if (client.connect("FireBeetle_TDS")) {
Serial.println("MQTT conectado");
} else {
delay(5000);
}
}
}

void setup() {

Serial.begin(115200);

analogReadResolution(12);
analogSetPinAttenuation(TDS_PIN, ADC_11db);

setup_wifi();

client.setServer(mqtt_server, mqtt_port);

Serial.println("SEN0244 TDS Sensor - FireBeetle ESP32");
}

void loop() {

if (!client.connected()) {
reconnect_mqtt();
}

client.loop();

int adcValue = analogRead(TDS_PIN);

float voltage = adcValue * VREF / ADC_RES;

// Compensación por temperatura
float compensationCoefficient =
1.0 + 0.02 * (temperature - 25.0);

float compensationVoltage =
voltage / compensationCoefficient;

// Fórmula DFRobot
float tdsValue =
(133.42 * compensationVoltage * compensationVoltage * compensationVoltage
- 255.86 * compensationVoltage * compensationVoltage
+ 857.39 * compensationVoltage) * 0.5;

Serial.print("ADC: ");
Serial.print(adcValue);

Serial.print(" | Voltaje: ");
Serial.print(voltage, 3);

Serial.print(" V | TDS: ");
Serial.print(tdsValue, 0);
Serial.println(" ppm");

if (millis() - mqttTime > MQTT_INTERVAL) {

char payload[256];

snprintf(payload, sizeof(payload),
"{\"sensor\":\"SEN0244\","
"\"parametro\":\"tds\","
"\"valor\":%.0f,"
"\"unidad\":\"ppm\","
"\"voltaje\":%.3f,"
"\"temperatura\":%.1f}",
tdsValue,
voltage,
temperature);

client.publish(mqtt_topic, payload);

mqttTime = millis();
}

delay(1000);
}
