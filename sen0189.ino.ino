/*
modificar los valores correspondientes al Wifi y a la etapa en la que se coloca 
el sensor; usa: agua_bruta, floculacion, acondicionado,control_calidad
*/

#include <WiFi.h>
#include <PubSubClient.h>

/*
  Proyecto: Sistemas de telemetría multisensorial para monitoreo de la calidad del agua.
  DFRobot SEN0189 Turbidez
  Board: FireBeetle 2 ESP32-E
  Signal pin: A2 / GPIO34
  MQTT topic: etap/etapa/ph
*/


#define TURBIDITY_PIN 34

// WiFi
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";

// MQTT
const char* mqtt_server = "IP_DE_TU_RASPBERRY";
const int mqtt_port = 1883;
const char* mqtt_topic = "etap/etapa/turbidez";

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
if (client.connect("FireBeetle_Turbidez")) {
Serial.println("MQTT conectado");
} else {
delay(5000);
}
}
}

void setup() {
Serial.begin(115200);

analogReadResolution(12);
analogSetPinAttenuation(TURBIDITY_PIN, ADC_11db);

setup_wifi();

client.setServer(mqtt_server, mqtt_port);
}

void loop() {

if (!client.connected()) {
reconnect_mqtt();
}

client.loop();

int adc = analogRead(TURBIDITY_PIN);

float vESP32 = adc * 3.3 / 4095.0;

// Divisor 10k / 20k
float vSensor = vESP32 * 1.5;

float ntu;

if (vSensor >= 4.2) {
ntu = 0;
}
else {
ntu = -1120.4 * vSensor * vSensor
+ 5742.3 * vSensor
- 4352.9;

if (ntu < 0) ntu = 0;
}

Serial.print("Voltaje: ");
Serial.print(vSensor, 3);

Serial.print(" V | NTU: ");
Serial.println(ntu, 1);

if (millis() - mqttTime > MQTT_INTERVAL) {

char payload[200];

snprintf(payload, sizeof(payload),
"{\"sensor\":\"SEN0189\","
"\"parametro\":\"turbidez\","
"\"ntu\":%.1f,"
"\"voltaje\":%.3f,"
"\"unidad\":\"NTU\"}",
ntu,
vSensor);

client.publish(mqtt_topic, payload);

mqttTime = millis();
}

delay(500);
}