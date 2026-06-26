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
#define ADC_RES 4096
#define READ_TEMP 25
#define TWO_POINT_CALIBRATION 0
#define CAL1_V 1600
#define CAL1_T 25
#define CAL2_V 1300
#define CAL2_T 15
const uint16_t DO_Table[41] = {
  14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
  11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
  9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
  7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410
};

// WiFi
const char* ssid = "Nombre_WIFI";
const char* password = "PASSWORD_wifi";

// MQTT
const char* mqtt_server = "IP_DE_RASPBERRY";
const int mqtt_port = 1883;
const char* mqtt_topic = "etap/etapa/oxigeno";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long mqttTime = 0;
const unsigned long MQTT_INTERVAL = 2000;


int16_t readDO(uint32_t voltage_mv, uint8_t temperature_c) {
#if TWO_POINT_CALIBRATION == 0
  uint16_t V_saturation = (uint32_t)CAL1_V + (uint32_t)35 * temperature_c - (uint32_t)CAL1_T * 35;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#else
  uint16_t V_saturation = (int16_t)((int8_t)temperature_c - CAL2_T) *
                          ((uint16_t)CAL1_V - CAL2_V) /
                          ((uint8_t)CAL1_T - CAL2_T) + CAL2_V;
  return (voltage_mv * DO_Table[temperature_c] / V_saturation);
#endif
}

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
uint16_t voltage_mv = (uint32_t)VREF * adc / ADC_RES;

uint8_t temperature = READ_TEMP;

float oxygen_ugL = readDO(voltage_mv, temperature);
float oxygen_mgL = oxygen_ugL / 1000.0;

Serial.print("ADC: ");
Serial.print(adc);
Serial.print(" | Voltaje: ");
Serial.print(voltage, 3);
Serial.print(" V | Oxigeno: ");
Serial.print(oxygen_mgL, 2);
Serial.println(" mg/L");

if (millis() - mqttTime > MQTT_INTERVAL) {

char payload[200];

snprintf(payload, sizeof(payload),
      "{\"sensor\":\"SEN0237-A\","
      "\"parametro\":\"oxigeno_disuelto\","
      "\"valor\":%.2f,"
      "\"unidad\":\"mg/L\","
      "\"voltaje\":%.3f,"
      "\"unidad_voltaje\":\"V\","
      "\"temperatura\":%d}",
      oxygen_mgL,
      voltage,
      temperature
    );


client.publish(mqtt_topic, payload);

mqttTime = millis();
}

delay(1000);
}


