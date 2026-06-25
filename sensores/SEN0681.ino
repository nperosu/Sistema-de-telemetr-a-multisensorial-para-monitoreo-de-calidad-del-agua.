#include <WiFi.h>
#include <PubSubClient.h>
#include <ModbusMaster.h>

/*
  Proyecto: Sistemas de telemetría multisensorial para monitoreo de la calidad del agua.
  DFRobot SEN0681 Oxigeno, salinidad, temperatura...
  Board: Edge101
  Signal pin: Modbus
  MQTT topic: etap/etapa/xxx
*/

#define SENSOR_ID 1
#define RS485_BAUD 4800

// WiFi
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";

// MQTT
const char* mqtt_server = "IP_DE_TU_RASPBERRY";
const int mqtt_port = 1883;

ModbusMaster node;
WiFiClient espClient;
PubSubClient client(espClient);

float regsToFloat(uint16_t highReg, uint16_t lowReg) {
uint32_t raw = ((uint32_t)highReg << 16) | lowReg;
float value;
memcpy(&value, &raw, sizeof(value));
return value;
}

void setup_wifi() {
WiFi.begin(ssid, password);

Serial.print("Conectando WiFi");
while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}

Serial.println();
Serial.print("IP: ");
Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
while (!client.connected()) {
if (client.connect("DFR0886_SEN0681")) {
Serial.println("MQTT conectado");
} else {
delay(5000);
}
}
}

void publicarMQTT(const char* topic, const char* sensor, const char* parametro, float valor, const char* unidad) {
char payload[180];

snprintf(payload, sizeof(payload),
"{\"sensor\":\"%s\",\"parametro\":\"%s\",\"valor\":%.2f,\"unidad\":\"%s\"}",
sensor, parametro, valor, unidad);

client.publish(topic, payload);
}

void setup() {
Serial.begin(115200);

setup_wifi();
client.setServer(mqtt_server, mqtt_port);

Serial2.begin(RS485_BAUD, SERIAL_8N1);
node.begin(SENSOR_ID, Serial2);

Serial.println("Lectura SEN0681 - Oxigeno Disuelto RS485 MQTT");
}

void loop() {
if (!client.connected()) {
reconnect_mqtt();
}

client.loop();

uint8_t result;

result = node.readHoldingRegisters(0x0000, 6);

if (result == node.ku8MBSuccess) {
float saturacion = regsToFloat(
node.getResponseBuffer(0),
node.getResponseBuffer(1)
) * 100.0;

float oxigeno = regsToFloat(
node.getResponseBuffer(2),
node.getResponseBuffer(3)
);

float temperatura = regsToFloat(
node.getResponseBuffer(4),
node.getResponseBuffer(5)
);

float salinidad = 0.0;
result = node.readHoldingRegisters(0x1020, 1);
if (result == node.ku8MBSuccess) {
salinidad = node.getResponseBuffer(0);
}

delay(200);

float presion = 0.0;
result = node.readHoldingRegisters(0x1022, 1);
if (result == node.ku8MBSuccess) {
presion = node.getResponseBuffer(0) / 100.0;
}

Serial.println("-----------------------------");
Serial.print("Oxigeno disuelto: ");
Serial.print(oxigeno, 2);
Serial.println(" mg/L");

Serial.print("Saturacion: ");
Serial.print(saturacion, 1);
Serial.println(" %");

Serial.print("Temperatura: ");
Serial.print(temperatura, 1);
Serial.println(" °C");

Serial.print("Salinidad: ");
Serial.print(salinidad, 1);
Serial.println(" ppt");

Serial.print("Presion atmosferica: ");
Serial.print(presion, 2);
Serial.println(" kPa");

publicarMQTT("etap/etapa/oxigeno_optico", "SEN0681", "oxigeno_disuelto", oxigeno, "mg/L");
publicarMQTT("etap/etapa/saturacion", "SEN0681", "saturacion_oxigeno", saturacion, "%");
publicarMQTT("etap/etapa/temperatura", "SEN0681", "temperatura", temperatura, "°C");
publicarMQTT("etap/etapa/salinidad", "SEN0681", "salinidad", salinidad, "ppt");
publicarMQTT("etap/etapa/presion", "SEN0681", "presion_atmosferica", presion, "kPa");
} 
else {
Serial.print("Error Modbus: ");
Serial.println(result);
}

delay(2000);
}

