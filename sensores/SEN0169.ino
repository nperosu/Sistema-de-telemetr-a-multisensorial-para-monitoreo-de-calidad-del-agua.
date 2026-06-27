/*Código de funcionamiento (modificar los valores en negrita, correspondientes al Wifi, 
a la etapa en la que se coloca el sensor usa: agua_bruta, floculacion, acondicionado,control_calidad; y valores de offset y slope usando el codigo de 
testeo(ver pdf manual del sensor)):
*/

#include <WiFi.h>
#include <PubSubClient.h>

/*
  Proyecto: Sistemas de telemetría multisensorial para monitoreo de la calidad del agua.
  Sensor: DFRobot SEN0169 pH
  Board: FireBeetle 2 ESP32-E
  Signal pin: A2 / GPIO34
  MQTT topic: etap/etapa/ph
*/

#define PH_PIN 34
#define VREF 3.3
#define ADC_RES 4095.0

// Sustituye estos valores por los obtenidos en la calibración
const float PH_SLOPE = -5.700000;
const float PH_OFFSET = 21.340000;

// WiFi
const char* ssid = "Nombre_WIFI";
const char* password = "PASSWORD_wifi";

// MQTT
const char* mqtt_server = "IP_DE_RASPBERRY";
const int mqtt_port = 1883;
const char* mqtt_topic = "etapa/etapa/ph";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long mqttTime = 0;
const unsigned long MQTT_INTERVAL = 2000;

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
    Serial.print("Conectando MQTT... ");

    if (client.connect("ElegooESP32_PH")) {
      Serial.println("MQTT conectado");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);

  analogReadResolution(12);
  analogSetPinAttenuation(PH_PIN, ADC_11db);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);

  Serial.println("DFRobot SEN0169 pH - Elegoo ESP32");
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }

  client.loop();

  float voltage = readAverageVoltage();
  float phValue = PH_SLOPE * voltage + PH_OFFSET;

  Serial.print("Voltaje: ");
  Serial.print(voltage, 3);
  Serial.print(" V | pH: ");
  Serial.println(phValue, 2);

  if (millis() - mqttTime > MQTT_INTERVAL) {
    char payload[256];

    snprintf(payload, sizeof(payload),
             "{\"sensor\":\"SEN0169\","
             "\"parametro\":\"ph\","
             "\"valor\":%.2f,"
             "\"unidad\":\"pH\","
             "\"voltaje\":%.3f}",
             phValue,
             voltage);

    bool enviado = client.publish(mqtt_topic, payload);

    if (enviado) {
      Serial.print("MQTT enviado: ");
      Serial.println(payload);
    } else {
      Serial.println("ERROR al publicar MQTT");
    }

    mqttTime = millis();
  }

  delay(1000);
}

float readAverageVoltage() {
  long adcSum = 0;

  for (int i = 0; i < 30; i++) {
    adcSum += analogRead(PH_PIN);
    delay(10);
  }

  float adcAverage = adcSum / 30.0;
  float voltage = adcAverage * VREF / ADC_RES;

  return voltage;
}
