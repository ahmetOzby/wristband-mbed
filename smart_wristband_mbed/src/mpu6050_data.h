/* Author: Ahmet Ozbay
 * Date: 01.04.2022
 * Purpose: Akıllı bileklik makine öğrenmesi algoritmasında veri seti oluşturması için 
 * MPU6050'den alınan verilerin işlenmiş ve işlenmemiş hallerini, MQTT üzerinden iletmek.
 */

#ifndef MPU6050_DATA_H_
#define MPU6050_DATA_H_

#include "stdint.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Arduino.h>

typedef struct 
{
  double sum_accel_x;
  double sum_accel_y;
  double sum_accel_z;
  double sum_gyro_x;
  double sum_gyro_y;
  double sum_gyro_z;

  float final_pitch;
  float final_roll;

  double sum_pitch;
  double sum_roll;

  double pitch_derivative_sum;
  double roll_derivative_sum;

  float sample_size;
}Mpu6050_Data_T;

typedef union
{
  struct
  {
    int16_t median_accel_x;
    int16_t median_accel_y;
    int16_t median_accel_z;

    int16_t median_gyro_x;
    int16_t median_gyro_y;
    int16_t median_gyro_z;

    int16_t pitch_integral;
    int16_t pitch_derrivative;

    int16_t roll_integral;
    int16_t roll_derrivative;

    uint8_t button_pressed;
  };
  uint8_t tx_buf[21];
}MQTT_Send_Data_T;

const int16_t ACCEL_Z_RAW_OFFSET = 3513;
const int16_t ACCEL_Y_RAW_OFFSET = 850;
const int16_t ACCEL_X_RAW_OFFSET = -750;

const int16_t GYRO_Z_RAW_OFFSET = 163;
const int16_t GYRO_Y_RAW_OFFSET = 3;
const int16_t GYRO_X_RAW_OFFSET = 482;

const float ACCEL_Z_SCALED_OFFSET = 2.1;
const float ACCEL_Y_SCALED_OFFSET = 0.7;
const float ACCEL_X_SCALED_OFFSET = -0.43;

const float GYRO_Z_SCALED_OFFSET = 1.28;
const float GYRO_Y_SCALED_OFFSET = 0.0487;
const float GYRO_X_SCALED_OFFSET = 3.678;
const uint16_t FILTER_SAMPLE_SIZE = 78; //156'dan 78'e çekildi
const byte MPU6050_FILTER_PERIODE = 2;

void mpu_setup(Adafruit_MPU6050 *mpu);  // mpu6050'nin kurulumunu yapar.
void Sum_Raw_Data(Mpu6050_Data_T *mpu6050_data, sensors_event_t a, sensors_event_t g);  // m/s2 ve dps cinsindeki ivmeölçver ve jiroskop verilerini toplar.
void Offset_mpu6050(sensors_event_t *a, sensors_event_t *g);  // m/s2 ve dps birimlerine dönüştürülmüş ham verilere belirlenmiş offsetleri ekler.
void Pitch_Roll_Calculate(Mpu6050_Data_T *mpu6050_data, sensors_event_t a, sensors_event_t g);  // Complimentary filtresi kullanarak "pitch" ve "roll"u hesaplar.
void Calculate_Pitch_Roll_Characteristics(Mpu6050_Data_T *mpu6050_data);    // Hesaplanan "pitch" ve "roll"un integral ve türev toplamını hesaplar.
void Create_MQTT_TX_Package(MQTT_Send_Data_T *mqtt_tx_package, Mpu6050_Data_T mpu6050_data, volatile bool *button_pressed);  // MQTT üzerinden yollanacak nihai verileri çıkartır.
void Print_Mqtt_TX_Pack(MQTT_Send_Data_T mqtt_tx_package);  // Hata ayıklaması için oluşturulmuş bu fonksiyon MQTT paketinin seri monitorden görülebilmesini sağlar.
void Clear_Mpu_Data(Mpu6050_Data_T *mpu6050_data);  // mpu6050 verilerinin işlenip kaydedildiği structure bir sonraki iterasyon için sıfırlanır. (sample_size hariç)
void Print_Raw_MQTT_TX_Pack(MQTT_Send_Data_T mqtt_tx_package);

#endif