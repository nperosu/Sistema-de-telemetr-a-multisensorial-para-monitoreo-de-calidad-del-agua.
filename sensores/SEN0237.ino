/*Código de funcionamiento modificar correspondientes al Wifi y a la etapa en la que se coloca el 
sensor usa: agua_bruta, floculacion, acondicionado,control_calidad):
*/
#include <WiFi.h>
#include <PubSubClient.h>

/*
  Proyecto: Sistemas de telemetría multisensorial para monitoreo de la calidad del agua.
  DFRobot SEN0237 Oxigeno disuelto
  Board: FireBeetle 2 ESP32-E
  Signal pin: A2 / GPIO34
  MQTT topic: etap/etapa/oxigeno
*/

#define DO_PIN 34

// WiFi
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";

// MQTT
const char* mqtt_server = "IP_DE_TU_RASPBERRY";
const int mqtt_port = 1883;
const char* mqtt_topic = "etap/etapa/oxigeno";

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

if (client.connect("FireBeetle_DO")) {
Serial.println("MQTT conectado");
} else {
delay(5000);
}
}
}

void setup() {

Serial.begin(115200);

analogReadResolution(12);
analogSetPinAttenuation(DO_PIN, ADC_11db);

setup_wifi();

client.setServer(mqtt_server, mqtt_port);
}

void loop() {

if (!client.connected()) {
reconnect_mqtt();
}

client.loop();

int adc = analogRead(DO_PIN);

float voltage = adc * 3.3 / 4095.0;

Serial.print("ADC: ");
Serial.print(adc);

Serial.print(" | Voltaje: ");
Serial.print(voltage, 3);
Serial.println(" V");

if (millis() - mqttTime > MQTT_INTERVAL) {

char payload[200];

snprintf(payload, sizeof(payload),
"{\"sensor\":\"SEN0237-A\","
"\"parametro\":\"oxigeno_disuelto\","
"\"voltaje\":%.3f,"
"\"unidad\":\"V\"}",
voltage);

client.publish(mqtt_topic, payload);

mqttTime = millis();
}

delay(1000);
}




