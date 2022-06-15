#include <NTPClient.h>  
#include <WiFiUdp.h>          
#include "src/tft.h"
#include "src/mqtt.h"
#include "src/mpu6050_data.h"
#include "MAX30100_PulseOximeter.h"
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
Adafruit_MPU6050 mpu;
Mpu6050_Data_T mpu6050_data;
sensors_event_t a, g, temp;
PulseOximeter pox;
float BPM, SpO2;

MQTT_Send_Data_T mqtt_tx_package;
void setup(void) 
{
  Serial.begin(115200);
  mpu_setup(&mpu);
  mpu6050_data.sample_size = FILTER_SAMPLE_SIZE;
  WiFi.begin("Ahmet", "11111111");
  client.setServer("broker.mqttdashboard.com", 1883);
  clock_gui_init();
  
  if (!pox.begin())                               //Oximetre sensör haberleşmesi başlatılır
  {
       Serial.println("FAILED");
       for(;;);
  }
  else
  {
       Serial.println("SUCCESS");
       pox.setOnBeatDetectedCallback(onBeatDetected);       //Kalp atımı algılandığında çağrılacak fonksiyon seçilir.
  }

  //pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);   
  // time init

  task_timer._1_sec_timer = millis() + _1_sec_in_ms;
  task_timer._1_hour_timer = millis() + _10_sec_in_ms;
  task_timer._10_sec_timer = millis() + _10_sec_in_ms;
}

void loop() 
{
  //
  client.loop();
  //
  pox.update();
  if (task_timer._200_ms_timer < millis()) 
  {
    //Serial.println("200ms task initated");

    BPM = pox.getHeartRate();
    SpO2 = pox.getSpO2();
    Serial.print("SpO2 : ");
    Serial.println(SpO2);
    Serial.print("Heart Rate : ");
    Serial.println(BPM);
    if(WiFi.status() == WL_CONNECTED && client.connected() == true)
    {
      //pox.shutdown();
      uint8_t arr[4] = {SpO2, BPM, temp.temperature, 0};
      client.publish("s_wristband", (const uint8_t *)arr, 4); 
      //pox.begin();
    }
    task_timer._200_ms_timer += _200_ms;
  }

  
  if (task_timer._1_sec_timer < millis()) 
  {
    Serial.println("1 sec task initiated");
    Serial.println(WiFi.status());
    start_clock();
    task_timer._1_sec_timer += _1_sec_in_ms;
  }

  if (task_timer._10_sec_timer < millis()) 
  {
    if(WiFi.status() == WL_CONNECTED && client.connected() == false)
    {
      pox.shutdown();
      client.connect("ESP8266Ahmet");
      Serial.print("mqttye bağlan komutu çalıştırıldı.");
      pox.begin();
    }
    task_timer._10_sec_timer += _10_sec_in_ms;
  }

  if (task_timer._1_hour_timer < millis()) 
  {
    Serial.println("1 hour task initiated");
    task_timer._1_hour_timer += _1_hour_in_ms;
    if(WiFi.status() == WL_CONNECTED)
    {
      pox.shutdown();
      timeClient.setTimeOffset(10800);                //GMT ayarı yapılır. (Türkiye için GMT+3)
      timeClient.begin();
      timeClient.update();
      String date = timeClient.getFormattedTime();
      Serial.println(date);
      ss = timeClient.getSeconds();
      mm = timeClient.getMinutes();
      hh = timeClient.getHours();
      initial = true;
      pox.begin();
    }
  }
  static uint64_t filter_time_stamp;
  if((millis() - filter_time_stamp) >= MPU6050_FILTER_PERIODE)      // TODO: Bu yapı yerine timer interrupt kullanılabilir.
  {
    static uint16_t sample_ctr {0};
    sample_ctr++;
    //TODO: Filter functions
    
    mpu.getEvent(&a, &g, &temp);   // Ham mpu6050 verileri I2C üzerinden alınır. Bu veriler m/s2 birimi cinsine dönüştürülür.
    Offset_mpu6050(&a, &g);        // Sensörün üretiminden kaynaklanan eklenmesi gereken offset değerleri alınan verilere eklenir.
    Sum_Raw_Data(&mpu6050_data, a, g);     // m/s2 birimi cinsine dönüştürülmüş ham verileri toplar.
    Pitch_Roll_Calculate(&mpu6050_data, a, g);  // Complimentary filtresi kullanılarak "pitch" ve "roll" hesabı yapılır.
    Calculate_Pitch_Roll_Characteristics(&mpu6050_data);  // Hesaplanan "pitch" ve "roll" açılarının türevler toplamı ve integrali hesaplanır.
    

    /*Serial.print("Temperature: ");
    Serial.print(temp.temperature);
    Serial.println(" degC");*/

    if(sample_ctr >= mpu6050_data.sample_size)
    {
      Create_MQTT_TX_Package(&mqtt_tx_package, mpu6050_data);   // MQTT üzerinden yollanacak structure doldurulur.
      //Print_Mqtt_TX_Pack(mqtt_tx_package);         // MQTT paketi seri monitor üzerinden gözlemlenebilir.
      //Print_Raw_MQTT_TX_Pack(mqtt_tx_package);       // Anlamlandırılmamış MQTT paketi seri monitörden görülebilir.
      client.publish("step_wristband", (const uint8_t *)mqtt_tx_package.tx_buf, sizeof(mqtt_tx_package.tx_buf));       // MQTT üzerinden iletim.
      Clear_Mpu_Data(&mpu6050_data);                            // MPU6050 verilerinin tutulduğu structure "sample_size" hariç sıfırlanır.

      
      Serial.println(g.gyro.x);
      Serial.println(g.gyro.y);
      Serial.println(g.gyro.z);
      Serial.println(a.gyro.x);
      Serial.println(a.gyro.y);
      Serial.println(a.gyro.z);
      sample_ctr = 0;       
    }
    
    filter_time_stamp = millis();
  }
}



void onBeatDetected()
{
    BPM = pox.getHeartRate();
    SpO2 = pox.getSpO2();
    Serial.print("SpO2 : ");
    Serial.println(SpO2);
    Serial.print("Heart Rate : ");
    Serial.println(BPM);        
}
