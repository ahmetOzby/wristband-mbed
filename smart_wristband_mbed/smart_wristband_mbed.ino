#include <NTPClient.h>  
#include <WiFiUdp.h>          
#include "src/tft.h"
#include "src/mqtt.h"
#define _1_sec_in_ms  1000
#define _200_ms       200
#define _1_hour_in_ms 360000
#define _10_sec_in_ms 10000

extern uint8_t ss;
extern uint8_t mm;
extern uint8_t hh;
extern bool initial;

typedef struct
{
  uint32_t _200_ms_timer;
  uint32_t _1_sec_timer;
  uint64_t _10_sec_timer;
  uint64_t _1_hour_timer;
}task_timer_t;

task_timer_t task_timer;
WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;                                                       //UDP nesnesi oluşturulur.
NTPClient timeClient(ntpUDP, "tr.pool.ntp.org");                     //NTP'yi kullanacak UDP nesnesi seçilir.
void setup(void) 
{
  Serial.begin(9600);
  WiFi.begin("Ahmet", "11111111");
  client.setServer("broker.mqttdashboard.com", 1883);
  clock_gui_init();
  // time init

  task_timer._1_sec_timer = millis() + _1_sec_in_ms;
  task_timer._1_hour_timer = millis() + _10_sec_in_ms;
  task_timer._1_hour_timer = millis() + _10_sec_in_ms;
}

void loop() 
{
  if (task_timer._200_ms_timer < millis()) 
  {
    Serial.println("200ms task initated");
    if(WiFi.status() == WL_CONNECTED && client.connected() == true)
    {
      client.publish("s_wristband", (const uint8_t *)"1?aZ", 4); 
    }
    task_timer._200_ms_timer += _200_ms;
  }

  
  if (task_timer._1_sec_timer < millis()) 
  {
    Serial.println("1 sec task initated");
    Serial.println(WiFi.status());
    start_clock();
    task_timer._1_sec_timer += _1_sec_in_ms;
  }

  if (task_timer._10_sec_timer < millis()) 
  {
    if(WiFi.status() == WL_CONNECTED && client.connected() == false)
    {
      client.connect("ESP8266Ahmet");
      Serial.print("mqttye bağlan komutu çalıştırıldı.");
    }
    task_timer._10_sec_timer += _10_sec_in_ms;
  }

  if (task_timer._1_hour_timer < millis()) 
  {
    Serial.println("1 hour task initated");
    task_timer._1_hour_timer += _1_hour_in_ms;
    if(WiFi.status() == WL_CONNECTED)
    {
      timeClient.setTimeOffset(10800);                //GMT ayarı yapılır. (Türkiye için GMT+3)
      timeClient.begin();
      timeClient.update();
      String date = timeClient.getFormattedTime();
      Serial.println(date);
      ss = timeClient.getSeconds();
      mm = timeClient.getMinutes();
      hh = timeClient.getHours();
      initial = true;
    }
  }
}
