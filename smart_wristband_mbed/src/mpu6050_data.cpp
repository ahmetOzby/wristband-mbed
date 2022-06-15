#include "mpu6050_data.h"


/*
 * Ortalama yaya yürüme hızı 1.2 m/s. Ortalama adım uzunluğu 0.64 m ve 0.75 m arasında değişmektedir. 
   Buna göre adım frekansının olabildiğince yüksek çıkması adına ortalama adım uzunluğu 0.75 m alınmıştır.
   Buna göre yayanın ortalama bir adımı atarken geçen süre (0.75 m / 1.2 m/s) 0.625 s olarak bulunmuştur. 
   Buna göre adım frekansı 1.6 Hz'dir. Nyquist teoremine göre örnekleme frekansı minimum (1.6 * 2) 3.2 Hz 
   olmalıdır. Buna uygun olarak alçak geçiren frekans 21 Hz, yüksek geçiren frekans ise 0.63 Hz olarak 
   belirlenmiştir.
 * mpu6050'nin ivmeölçer örnekleme oranı 1 kHz olduğundan 2 ms'de bir sensör okumaları yapılacak. 
   İşlenen ve ham veriler toplanarak (1 / 3.2) ~ 312 ms'de bir ortalaması alınarak makine öğrenmesi için 
   veri seti oluşturulacaktır.
 * 2 ms'de bir okunan ham sensör verileri 156 okuma sonucunda makine öğrenmesine girecek veri setinin özellik 
   matrisinin bir satırı oluşmuş olacaktır.
 * Tamamlayıcı filtrenin 312 ms'nin sonunda 156 örneği olacaktır. 156 örneğin oluşturduğu grafiğin altındaki 
  alan ve her bir noktanın bir öncekiyle türevi makine öğrenmesi veri setine sokulacaktır.
*/


void mpu_setup(Adafruit_MPU6050 *mpu)
{
   if (!mpu->begin()) 
  {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  // +- 2G kuvveti arası veri alınmıştır. Bu aralık adım tespiti için yeterlidir.
  mpu->setAccelerometerRange(MPU6050_RANGE_2_G);
  Serial.print("Accelerometer range set to: +-2G");
  
  // +- 500 dps arası veri alınmıştır. Bu aralık adım tespiti için yeterlidir.
  mpu->setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: +- 500 deg/s");

  mpu->setFilterBandwidth(MPU6050_BAND_21_HZ);
  mpu->setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  Serial.println("Filter bandwidth set to: 21Hz");

  delay(100);
}

void Sum_Raw_Data(Mpu6050_Data_T *mpu6050_data, sensors_event_t a, sensors_event_t g)
{
  // Verilerin toplamı
  mpu6050_data->sum_accel_x += a.acceleration.x;
  mpu6050_data->sum_accel_y += a.acceleration.y;
  mpu6050_data->sum_accel_z += a.acceleration.z;
  mpu6050_data->sum_gyro_x += g.gyro.x;
  mpu6050_data->sum_gyro_y += g.gyro.y;
  mpu6050_data->sum_gyro_z += g.gyro.z;
}

void Offset_mpu6050(sensors_event_t *a, sensors_event_t *g)
{
  /*
   * Bu fonksiyon önceden belirlenmiş  offset değerlerini alınan ölçeklenmiş sensör verilerine ekler.
   */
    a->acceleration.x += ACCEL_X_SCALED_OFFSET;
    a->acceleration.y += ACCEL_Y_SCALED_OFFSET;
    a->acceleration.z += ACCEL_Z_SCALED_OFFSET;

    g->gyro.x += GYRO_X_SCALED_OFFSET;
    g->gyro.y += GYRO_Y_SCALED_OFFSET;
    g->gyro.z += GYRO_Z_SCALED_OFFSET;
}

// Fonksiyon sonucu "pitch" ve "roll" hesaplanarak sonuçlar structure'a kaydedilir.
void Pitch_Roll_Calculate(Mpu6050_Data_T *mpu6050_data, sensors_event_t a, sensors_event_t g)
{
  static uint64_t dt_time_stamp;
  static float pitch_angle_g {0};
  static float roll_angle_g {0};
  float pitch_angle_a {0};
  float roll_angle_a {0};
  
  float dt = (millis() - dt_time_stamp) / 1000.0f;
  dt_time_stamp = millis();
   
  pitch_angle_g += g.gyro.y * dt; 
  roll_angle_g += g.gyro.x * dt; 

 
  pitch_angle_a = -atan2(a.acceleration.x, a.acceleration.z)/2/3.141592654*360 -5.20;
  roll_angle_a = atan2(a.acceleration.y, a.acceleration.z)/2/3.141592654*360;
  mpu6050_data->final_pitch = pitch_angle_g * 0.3f + pitch_angle_a * 0.7f;
  mpu6050_data->final_roll = roll_angle_g * 0.3f + roll_angle_a * 0.7f;
}

// Hesaplanan "pitch" ve "roll" değerlerinin türev toplamları ve integrali hesaplanır.
void Calculate_Pitch_Roll_Characteristics(Mpu6050_Data_T *mpu6050_data)
{
  static float temp_final_pitch {0};
  static float temp_final_roll {0};
  
  mpu6050_data->sum_pitch += mpu6050_data->final_pitch;
  mpu6050_data->sum_roll += mpu6050_data->final_roll;

  
  mpu6050_data->pitch_derivative_sum += (mpu6050_data->final_pitch - temp_final_pitch);
  mpu6050_data->roll_derivative_sum += (mpu6050_data->final_roll - temp_final_roll);
  temp_final_pitch = mpu6050_data->final_pitch;
  temp_final_roll = mpu6050_data->final_roll;
}

/* Bu fonksiyon "sample_size"a uygun periyodda çağrılmalıdır. İşlenen ve ham verilerin geçen 
 * zamandaki ortalaması alınarak MQTT tx paketine eklenir.
*/
void Create_MQTT_TX_Package(MQTT_Send_Data_T *mqtt_tx_package, Mpu6050_Data_T mpu6050_data)
{
  mqtt_tx_package->median_accel_x = int16_t(mpu6050_data.sum_accel_x / mpu6050_data.sample_size * 100);
  mqtt_tx_package->median_accel_y = int16_t(mpu6050_data.sum_accel_y / mpu6050_data.sample_size * 100);
  mqtt_tx_package->median_accel_z = int16_t(mpu6050_data.sum_accel_z / mpu6050_data.sample_size * 100);
  mqtt_tx_package->median_gyro_x = int16_t(mpu6050_data.sum_gyro_x / mpu6050_data.sample_size * 100);
  mqtt_tx_package->median_gyro_y = int16_t(mpu6050_data.sum_gyro_y / mpu6050_data.sample_size * 100);
  mqtt_tx_package->median_gyro_z = int16_t(mpu6050_data.sum_gyro_z / mpu6050_data.sample_size * 100);

  //mqtt_tx_package->pitch_integral = mpu6050_data.sum_pitch / mpu6050_data.sample_size * 100;
  //mqtt_tx_package->roll_integral = mpu6050_data.sum_roll / mpu6050_data.sample_size * 100;


  mqtt_tx_package->pitch_derrivative = mpu6050_data.pitch_derivative_sum / mpu6050_data.sample_size * 100;
  mqtt_tx_package->roll_derrivative = mpu6050_data.roll_derivative_sum / mpu6050_data.sample_size * 100;

  /*if(*button_pressed == true)
  {
    mqtt_tx_package->button_pressed = 1;
    *button_pressed = false;
  }

  else
  {
    mqtt_tx_package->button_pressed = 0;
  }*/
}

// Oluşturulan MQTT TX paketinin hata ayıklama için yazdırma fonksiyonu.
void Print_Mqtt_TX_Pack(MQTT_Send_Data_T mqtt_tx_package)
{
  Serial.print("Median Accel X: ");
  Serial.print(mqtt_tx_package.median_accel_x);
  
  Serial.print("  Median Accel Y: ");
  Serial.print(mqtt_tx_package.median_accel_y);
  
  Serial.print("  Median Accel Z: ");
  Serial.print(mqtt_tx_package.median_accel_z);
  
  Serial.print("  Median Gyro X: ");
  Serial.print(mqtt_tx_package.median_gyro_x);
  
  Serial.print("  Median Gyro Y: ");
  Serial.print(mqtt_tx_package.median_gyro_y);
  
  Serial.print("  Median Gyro Z: ");
  Serial.println(mqtt_tx_package.median_gyro_z);
  
  /*Serial.print("Pitch integral Rate: ");
  Serial.print(mqtt_tx_package.pitch_integral);
  
  Serial.print("  Roll integral Rate: ");
  Serial.print(mqtt_tx_package.roll_integral);*/
 

  Serial.print("  Pitch Derrivative Rate: ");
  Serial.print(mqtt_tx_package.pitch_derrivative);
  
  Serial.print("Roll Derrivative Rate: ");
  Serial.println(mqtt_tx_package.roll_derrivative);
}

// MQTT paketinin iletilmesinin ardından hesaplama verileri sıfırlanır.
void Clear_Mpu_Data(Mpu6050_Data_T *mpu6050_data)
{
  memset(mpu6050_data, 0, sizeof(*mpu6050_data));
  mpu6050_data->sample_size = FILTER_SAMPLE_SIZE; // Bu assignment yapılmazsa 0'a bölüm sonucu mikrodenetleyici Hard Fault'a düşebilir.
}

void Print_Raw_MQTT_TX_Pack(MQTT_Send_Data_T mqtt_tx_package)
{
  for (size_t i = 0; i < sizeof(mqtt_tx_package.tx_buf)  ; i++)
  {
    Serial.print(i);
    Serial.print(".: ");
    Serial.print(mqtt_tx_package.tx_buf[i]);
    Serial.print("  ");
    if(i % 5 == 0 && i != 0)
    {
      Serial.println();
    }
  }
}