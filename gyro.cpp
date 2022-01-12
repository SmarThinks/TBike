#include "Arduino.h"
#include "Gyro.h"
#include "MPU6050.h"
#include "I2Cdev.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

MPU6050 imuGyro;
unsigned long t_now;
unsigned long last_read_time;
float gyro_angle_x;
float gyro_angle_y;
float angle_x;
float angle_y;
int16_t ax, ay, az, gx, gy, gz;
int16_t gyro_angle_x_l, gyro_angle_y_l;
int16_t angle_x_l, angle_y_l;
int16_t ax_offset, ay_offset, az_offset, gx_offset, gy_offset, gz_offset;
const float CONST_16G = 2048;
const float _CONST_2000 = 16.4;
const float _CONST_G = 9.81;
const float _RADIANS_TO_DEGREES = 180 / 3.14159;
const float _ALPHA = 0.96;
const float _KMPH = 3.6;

inline unsigned long get_last_time() {
  return last_read_time;
}

inline void set_last_time(unsigned long _time) {
 last_read_time = _time;
}

inline float get_delta_time(unsigned long t_now) {
  return (t_now - get_last_time()) / 1000.0;
}

inline int16_t get_last_gyro_angle_x() {
  return gyro_angle_x_l;
}

inline void set_last_gyro_angle_x(int16_t _gyro_angle_x) {
  gyro_angle_x_l = _gyro_angle_x;
}

inline int16_t get_last_gyro_angle_y() {
  return gyro_angle_y_l;
}

inline void set_last_gyro_angle_y(int16_t _gyro_angle_y) {
  gyro_angle_y_l = _gyro_angle_y;
}

inline int16_t get_last_angle_x() {
  return angle_x_l;
}

inline void set_last_angle_x(int16_t _ang_x) {
  angle_x_l = _ang_x;
}

inline int16_t get_last_angle_y() {
  return angle_y_l;
}

inline void set_last_angle_y(int16_t _ang_y) {
  angle_y_l = _ang_y;
}

inline float get_accel_xy(float ax_p, float ay_p) {
  return sqrt(pow(ax_p, 2) + pow(ay_p, 2));
}


Gyro::Gyro(){
  imuGyro.initialize();
  if(!imuGyro.testConnection()){
    imuGyro.initialize();
  }
}

void Gyro::calibration(){
  Serial.println(F("Empezando calibración"));
  int   num_readings = 1000;
  float x_accel = 0;
  float y_accel = 0;
  float z_accel = 0;
  float x_gyro = 0;
  float y_gyro = 0;
  float z_gyro = 0;
  for (int i = 0; i < num_readings; i++) {
    imuGyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);    
    x_accel += ax;
    y_accel += ay;
    z_accel += az;
    x_gyro += gx;
    y_gyro += gy;
    z_gyro += gz;
    delay(10);
  }
  x_accel /= num_readings;
  y_accel /= num_readings;
  z_accel /= num_readings;
  x_gyro /= num_readings;
  y_gyro /= num_readings;
  z_gyro /= num_readings;
  ax_offset = x_accel;
  ay_offset = y_accel;
  az_offset = z_accel;
  gx_offset = x_gyro;
  gy_offset = y_gyro;
  gz_offset = z_gyro;
  Serial.println(F("Done calibration")); 
}

void Gyro::readerImu(){
  for(int i=0;i<10;i++){
      t_now = millis();
      float dt = get_delta_time(t_now);
      imuGyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
      float ax_p = (ax - ax_offset) / CONST_16G;
      float ay_p = (ay - ay_offset) / CONST_16G;
      float az_p = (az / CONST_16G);
  
      float accel_angle_y = atan(-1 * ax_p / sqrt(pow(ay_p, 2) + pow(az_p, 2))) * _RADIANS_TO_DEGREES;
      float accel_angle_x = atan(ay_p / sqrt(pow(ax_p, 2) + pow(az_p, 2))) * _RADIANS_TO_DEGREES;
  
      float gx_p = (gx - gx_offset) / _CONST_2000;
      float gy_p = (gy - gy_offset) / _CONST_2000;
      float gz_p = (gz - gz_offset) / _CONST_2000;
  
      gyro_angle_x = gx_p * dt + get_last_angle_x();
      gyro_angle_y = gy_p * dt + get_last_angle_y();
  
      angle_x = _ALPHA * gyro_angle_x + (1.0 - _ALPHA) * accel_angle_x;
      angle_y = _ALPHA * gyro_angle_y + (1.0 - _ALPHA) * accel_angle_y;
  
      float vel_x = (ax_p * dt * _CONST_G);
      float vel_y = (ay_p * dt * _CONST_G);
      _aprox_vel = round(sqrt(pow(vel_x, 2) + pow(vel_y, 2)) * _KMPH);  
      _imu_data_arr[i] = _aprox_vel;
      Serial.println(_aprox_vel);
    }
    
    set_last_time(t_now);
  
    set_last_gyro_angle_x(gyro_angle_x);
    set_last_gyro_angle_y(gyro_angle_y);
  
    set_last_angle_x(angle_x);
    set_last_angle_y(angle_y);

    autostop();
}
void Gyro::autostop(){
  if(sizeof(_imu_data_arr) > 0){
        for(int j=0;j<10;j++){
      if((_imu_data_arr[0]+3)>=_imu_data_arr[j] && (_imu_data_arr[0]-3)<=_imu_data_arr[j]){
        _imu_counter++;
        if(_imu_counter==10){
          _imu_counter=0;
        }
      }else{
        _imu_counter=0;
        Serial.println("Stop");
        //if(deviceConnected){
          //Serial.println("MPU val: 1");
          //sendBLECodecData("G10001");
        //}
      }
    }
  }
}
