#include "Arduino.h"

#ifndef Luxer_h
#define Luxer_h
class Luxer{
  public:
    Luxer(int _lux_pin);
    void calibration();
    void readerLux();
    void sendDataLux();
  

  private:
    int _lux_pin;
    float _lux_reference;
    float _lux_value;
    float _lux_reader;
    String lux_code_data;
  
};
#endif
