#include "key.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "esp_eap_client.h"

WiFiClientSecure espClient = WiFiClientSecure();
PubSubClient client(espClient);

const char* sub_topic = "response/action";
const char* pub_topic = "kachow/pub";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println(F("WiFi is connected!"));
  Serial.println(F("IP address set: "));
  Serial.println(WiFi.localIP());

  espClient.setCACert(AWS_CERT_CA);
  espClient.setCertificate(AWS_CERT_CRT);
  espClient.setPrivateKey(AWS_CERT_PRIVATE);
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(1000);
  }
  if (!client.connected()) {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  if (client.subscribe(sub_topic)) {
    Serial.println("Subscribed successfully!");
  } else {
      Serial.println("Subscription failed!");
  }
  client.setCallback(callback);
  client.subscribe(sub_topic);
  Serial.println("AWS IoT Connected!");
}

void callback(char* topic, byte* payload, unsigned int length) {
    payload[length] = '\0';
    String message = String((char*)payload);
    
    Serial.println(message);
}

void loop() {
  // put your main code here, to run repeatedly:
  client.loop();
}
