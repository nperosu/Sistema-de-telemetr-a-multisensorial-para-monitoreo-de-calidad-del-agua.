#include <WiFi.h>
#include <PubSubClient.h>

/*
  Proyecto: Sistemas de telemetría multisensorial para monitoreo de la calidad del agua.
  DFRobot SEN0507 Sensor liquido tubo
  Board: FireBeetle 2 ESP32-E
  Signal pin: A2 / GPIO34
  MQTT topic: etap/etapa/ph
*/

#define LEVEL_PIN 2

// WiFi
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";

// MQTT
const char* mqtt_server = "IP_DE_TU_RASPBERRY";
const int mqtt_port = 1883;
const char* mqtt_topic = "etap/etapa/nivel_tubo";

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

    if (client.connect("FireBeetle_NivelTubo")) {
      Serial.println("MQTT conectado");
    } else {
      delay(5000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(LEVEL_PIN, INPUT);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {

  if (!client.connected()) {
    reconnect_mqtt();
  }

  client.loop();

  bool liquidDetected =
      (digitalRead(LEVEL_PIN) == LOW);

  if (liquidDetected) {
    Serial.println("LIQUIDO DETECTADO");
  } else {
    Serial.println("SIN LIQUIDO");
  }

  if (millis() - mqttTime > MQTT_INTERVAL) {

    char payload[200];

    snprintf(payload, sizeof(payload),
             "{\"sensor\":\"SEN0509\","
             "\"parametro\":\"nivel_tubo\","
             "\"estado\":%d,"
             "\"descripcion\":\"%s\"}",
             liquidDetected ? 1 : 0,
             liquidDetected ?
             "LIQUIDO_DETECTADO" :
             "SIN_LIQUIDO");

    client.publish(mqtt_topic, payload);

    mqttTime = millis();
  }

  delay(500);
}