#include "mqtt.h"


const char* ssid = "Ahmet"; // WiFi ağının adı
const char* password =  "11111111"; // WiFi şifresi
const char* mqttServer = "broker.mqttdashboard.com";  // Broker IP
const int mqttPort = 1883;  // Port
const char* mqttUser = "otfxknod";  // MQTT kullanıcı adı
const char* mqttPassword = "nSuUc1dDLygF";  // MQTT şifresi
const char* data_topic = "s_wristband";  // Topic adı
const char* clientID = "ESP8266Ahmet";        // Client numarası

void wifi_setup(void)
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
}


void mqtt_setup(PubSubClient *client)
{
  client->setServer(mqttServer, mqttPort);
 
  while (!client->connected()) 
  {
    Serial.println("Connecting to MQTT...");
 
    if (client->connect(clientID)) 
    {
 
      Serial.println("connected");  
 
    } 
    else 
    {
 
      Serial.print("failed with state ");
      Serial.print(client->state());
      delay(2000);
    }
  }
}

