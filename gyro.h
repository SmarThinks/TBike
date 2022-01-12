#include "Arduino.h"

#ifndef Gyro_h
#define Gyro_h
class Gyro{
  public:
    Gyro();
    void calibration();
    void readerImu();
    void autostop();
    void sendDataImu();
  

  private:
    float _aprox_vel;
    float _imu_data_arr[10];
    int _imu_counter;
  
};
#endif
