/*Código de funcionamiento (modificar los valores en negrita, correspondientes al Wifi y 
a la etapa en la que se coloca el sensor usa: agua_bruta, floculacion, acondicionado,control_calidad):
*/
#include <WiFi.h>
#include <PubSubClient.h>

/*
  Proyecto: Sistemas de telemetría multisensorial para monitoreo de la calidad del agua.
  DFRobot SEN0169-V2 pH Meter Pro
  Board: FireBeetle 2 ESP32-E
  Signal pin: A2 / GPIO34
  MQTT topic: etap/etapa/ph
*/

#define PH_PIN 34
#define OFFSET 0.00
#define SAMPLING_INTERVAL 20
#define PRINT_INTERVAL 800
#define MQTT_INTERVAL 2000
#define ARRAY_LENGTH 40

// WiFi
const char* ssid = "TU_WIFI";
const char* password = "TU_PASSWORD";

// MQTT Raspberry Pi
const char* mqtt_server = "IP_DE_TU_RASPBERRY";
const int mqtt_port = 1883;
const char* mqtt_topic = "etap/etapa/ph";

WiFiClient espClient;
PubSubClient client(espClient);

int pHArray[ARRAY_LENGTH];
int pHArrayIndex = 0;

unsigned long samplingTime = 0;
unsigned long printTime = 0;
unsigned long mqttTime = 0;

float voltage = 0.0;
float pHValue = 0.0;

void setup_wifi() {
  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT... ");

    if (client.connect("FireBeetle_pH")) {
      Serial.println("conectado");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5s");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(PH_PIN, ADC_11db);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);

  Serial.println("DFRobot SEN0169-V2 pH meter - ESP32 MQTT");
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }

  client.loop();

  if (millis() - samplingTime > SAMPLING_INTERVAL) {
    pHArray[pHArrayIndex++] = analogRead(PH_PIN);

    if (pHArrayIndex >= ARRAY_LENGTH) {
      pHArrayIndex = 0;
    }

    float averageADC = averageArray(pHArray, ARRAY_LENGTH);

    voltage = averageADC * 3.3 / 4095.0;
    pHValue = 3.5 * voltage + OFFSET;

    samplingTime = millis();
  }

  if (millis() - printTime > PRINT_INTERVAL) {
    Serial.print("ADC promedio: ");
    Serial.print(averageArray(pHArray, ARRAY_LENGTH), 0);

    Serial.print(" | Voltaje: ");
    Serial.print(voltage, 3);

    Serial.print(" V | pH: ");
    Serial.println(pHValue, 2);

    printTime = millis();
  }

  if (millis() - mqttTime > MQTT_INTERVAL) {
    char payload[120];

    snprintf(payload, sizeof(payload),
             "{\"sensor\":\"SEN0169-V2\",\"parametro\":\"pH\",\"valor\":%.2f,\"unidad\":\"pH\",\"voltaje\":%.3f}",
             pHValue,
             voltage);

    client.publish(mqtt_topic, payload);

    Serial.print("MQTT enviado a ");
    Serial.print(mqtt_topic);
    Serial.print(": ");
    Serial.println(payload);

    mqttTime = millis();
  }
}

double averageArray(int* arr, int number) {
  if (number <= 0) return 0;

  long amount = 0;

  if (number < 5) {
    for (int i = 0; i < number; i++) {
      amount += arr[i];
    }
    return (double)amount / number;
  }

  int minVal, maxVal;

  if (arr[0] < arr[1]) {
    minVal = arr[0];
    maxVal = arr[1];
  } else {
    minVal = arr[1];
    maxVal = arr[0];
  }

  for (int i = 2; i < number; i++) {
    if (arr[i] < minVal) {
      amount += minVal;
      minVal = arr[i];
    } else if (arr[i] > maxVal) {
      amount += maxVal;
      maxVal = arr[i];
    } else {
      amount += arr[i];
    }
  }

  return (double)amount / (number - 2);
}
