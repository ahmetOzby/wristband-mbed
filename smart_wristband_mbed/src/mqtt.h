/* Author: Ahmet Ozbay
 * Date: 01.04.2022
 * Purpose: Akıllı bileklik makine öğrenmesi algoritmasında veri seti oluşturması için 
 * MPU6050'den alınan verilerin işlenmiş ve işlenmemiş hallerini, MQTT üzerinden iletmek.
 */

#ifndef MQTT_H_
#define MQTT_H_
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

extern const char* data_topic;

void wifi_setup(void);  // ESP8266 için WiFi ağı ile internete bağlanma kurulumunu tamamlar.
void mqtt_setup(PubSubClient *client);  // İnternete bağlantı sağlandıktan sonra ".cpp" dosyasındaki konfigürasyonlara göre MQTT broker'ına bağlanır.

#endif