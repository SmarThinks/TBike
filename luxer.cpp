#include "Arduino.h"
#include "Luxer.h"

Luxer::Luxer(int lux_pin){
  _lux_pin = lux_pin;
}

void Luxer::calibration(){
  const int lux_num_readings = 100;
  for(int i = 0; i <= lux_num_readings; i++){
    float lux_aux_read = analogRead(_lux_pin);
    _lux_reference += lux_aux_read;
    delay(10);
  }
  _lux_reference = _lux_reference / lux_num_readings;

  Serial.print(_lux_reference);
}
