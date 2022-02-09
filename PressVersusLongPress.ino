#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif


#include <AceButton.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "MPU6050.h"
#include "Wire.h"
#include "I2Cdev.h"

using namespace ace_button;
const int MODE_BUTTON_PIN = 26;
const int CHANGE_BUTTON_PIN = 25;
const int DEMO_BUTTON_PIN = 23;
const int POWER_SYS_LED = 4;
const int DIR_LEFT_LED = 14;
const int DIR_RIGHT_LED = 13;
AceButton centerButton(MODE_BUTTON_PIN);
AceButton rightButton(CHANGE_BUTTON_PIN);
AceButton leftButton(DEMO_BUTTON_PIN);

int state = 1;
int counter = 0;
bool isEditting = false;
unsigned long tiempo;
#define DELAY 10000
String direction = "";
bool isBlinking = false;
bool isDirActive = false;

const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

String rx_brigthness = "";

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String _rxValue;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const float CONST_16G = 2048;
const float _CONST_2000 = 16.4;
const float _CONST_G = 9.81;
const float _RADIANS_TO_DEGREES = 180 / 3.14159;
const float _ALPHA = 0.96;
const float _KMPH = 3.6;
float datos[10];
int cont;
MPU6050 accelgyro;
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
int16_t temperature;
float vel;


void LuxBrigTask( void *pvParameters );
void AutoStopTask( void *pvParameters );
void BatStatusTask( void *pvParameters );
void BLEControlTask( void *pvParameters );
void sendBLECodecData(String _dato);

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        _rxValue = "";

        for (int i = 0; i < rxValue.length(); i++) {
          if(i<=6){
            _rxValue+=rxValue[i];      
          }                
        }
        Serial.print(_rxValue);
        sendBLECodecData(_rxValue);
      }
    }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
      ledcWrite(ledChannel, 1500);
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      ledcWrite(ledChannel, 0);
    }
};

void handleEvent(AceButton* button, uint8_t eventType,
    uint8_t /*buttonState*/) {
  uint8_t pin = button->getPin();

  if (pin == MODE_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventReleased:         
          state++;
          if (state > 2) state = 1;
          Serial.println(F("Mode Button: Pressed"));
          Serial.print(F("New state: "));
          Serial.println(state);
//          if(state == 1){
//            isEditting = !isEditting;
//            isEditting? isBlinking = true, digitalWrite(POWER_SYS_LED, HIGH) : digitalWrite(POWER_SYS_LED, LOW);
//          }
        break;
      case AceButton::kEventLongPressed: 
//        isEditting = !isEditting;
//        Serial.println(isEditting);
//        if(state >= 1){
           unsigned long intCh1Rise = micros();
           tiempo = (micros() - intCh1Rise) / 1000;
          if(millis()-tiempo>3000){            
            digitalWrite(POWER_SYS_LED, LOW);
            delay(1000);
            esp_deep_sleep_start();
          }
//        }         
        break;
    }
  } else if (pin == CHANGE_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventPressed:
      case AceButton::kEventRepeatPressed:
//        isBlinking = false;
//        if (isEditting) {
          if (eventType == AceButton::kEventPressed) {
            Serial.println(F("Right Pressed"));
            digitalWrite(DIR_LEFT_LED, LOW);
            digitalWrite(DIR_RIGHT_LED, HIGH);
            isDirActive = true;
            tiempo = millis();
            direction = "Right";
            _rxValue = "D00001";
            sendBLECodecData(_rxValue);
          } else {
            //Serial.println(F("Change Button: Repeat Pressed"));
            digitalWrite(DIR_RIGHT_LED, LOW);
//            isDirActive = false;
          }
     //   }
        break;
        
      case AceButton::kEventLongReleased:
        break;
      case AceButton::kEventDoubleClicked:
//        isBlinking = false;
//        if (isEditting) {
          Serial.println(F("Change Button 2: Repeat"));
           digitalWrite(DIR_RIGHT_LED, LOW);
           isDirActive = false;
           _rxValue = "D00010";
          sendBLECodecData(_rxValue);
        //}
        break;
    }
  } else if (pin == DEMO_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventPressed:
      case AceButton::kEventRepeatPressed:
//        isBlinking = false;
//        if (isEditting) {
          if (eventType == AceButton::kEventPressed) {
            Serial.println(F("Change Button 2: Pressed"));
            digitalWrite(DIR_LEFT_LED, HIGH);
            digitalWrite(DIR_RIGHT_LED, LOW); 
            isDirActive = true;
            tiempo = millis();
            direction = "Left";
            _rxValue = "D00100";
            sendBLECodecData(_rxValue);
          } else {
           // Serial.println(F("Change Button 2: Repeat Pressed"));
             digitalWrite(DIR_LEFT_LED, LOW);
             isDirActive = false;
          }
       // }
        break;

      case AceButton::kEventLongReleased:
        break;
      case AceButton::kEventDoubleClicked:
        isBlinking = false;
//        if (isEditting) {
          Serial.println(F("Change Button 2: Repeat"));
          digitalWrite(DIR_LEFT_LED, LOW);
          isDirActive = false;
          _rxValue = "D00010";
          sendBLECodecData(_rxValue);
//        }
        break;
      
    }
  }
}

void timingAutoOff(){
  if(millis()-tiempo>DELAY && isDirActive){
    tiempo=0;
    isDirActive = false;
    _rxValue = "D00010";
  sendBLECodecData(_rxValue);
    if(direction == "Right"){
      digitalWrite(DIR_RIGHT_LED, LOW);
    }else if(direction == "Left"){
      digitalWrite(DIR_LEFT_LED, LOW);
    }    
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  while (! Serial);
  Serial.println(F("setup(): begin"));

  Serial.println(F("Starting BLE Server"));
  // Create the BLE Device
  BLEDevice::init("ESP32");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();

    
  Serial.println(F("Enable GPIOs!"));  
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12,0);
  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(CHANGE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DEMO_BUTTON_PIN, INPUT_PULLUP);
  pinMode(POWER_SYS_LED, OUTPUT);
  pinMode(DIR_LEFT_LED, OUTPUT);
  pinMode(DIR_RIGHT_LED, OUTPUT);

  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(POWER_SYS_LED, ledChannel);
  
  Serial.println(F("Configure Buttons envents handler!")); 
   
  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);
   
  Serial.println(F("Starting MPU6050"));
  Wire.begin();
  accelgyro.initialize();
  if(!accelgyro.testConnection()){
    accelgyro.initialize();
  }
  accelgyro.setFullScaleAccelRange(0x03);
  accelgyro.setFullScaleGyroRange(0x03);
  _stopCalibration();
  Serial.println(F("Starting tasks!"));
  xTaskCreatePinnedToCore(
    LuxBrigTask
    ,  "LuxBrigTask"
    ,  2048
    ,  NULL
    ,  1
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(
    BatStatusTask
    ,  "BatStatusTask"
    ,  1024
    ,  NULL
    ,  1
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(
    AutoStopTask
    ,  "AutoStopTask"
    ,  2048
    ,  NULL
    ,  1
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

 // 
  Serial.println(F("setup(): ready"));
}

void sendBLECodecData(String _dato){
  if(deviceConnected){
   pCharacteristic->setValue(_dato.c_str());
   pCharacteristic->notify();
  }
}


void _stopCalibration() {
  int   num_readings = 100;
  float x_accel = 0;
  float y_accel = 0;
  float z_accel = 0;
  float x_gyro = 0;
  float y_gyro = 0;
  float z_gyro = 0;
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  for (int i = 0; i < num_readings; i++) {
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);    
    x_accel += ax;
    y_accel += ay;
    z_accel += az;
    x_gyro += gx;
    y_gyro += gy;
    z_gyro += gz;
    Serial.println("calibre");
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
}

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

void loop() {
  centerButton.check();
  rightButton.check();
  leftButton.check();
  timingAutoOff();
  if(!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  if(deviceConnected && !oldDeviceConnected) {
     oldDeviceConnected = deviceConnected;
  }
}

void _autoStop(){
  //if(sizeof(datos) > 0){
    for(int j=0;j<10;j++){
      if((datos[0]+3)>=datos[j] && (datos[0]-3)<=datos[j]){
        cont++;
        if(cont==10){
          cont=0;
        }
      }else{
        cont=0;
        Serial.println("1: get permanent red");
        if(deviceConnected){
          Serial.println("MPU val: 1");
          sendBLECodecData("G10001");
        }
      }
    }
}


void LuxBrigTask(void *pvParameters){
  (void) pvParameters;
  for (;;){
//    uint32_t lum = lux_brigthness.getFullLuminosity();
//    uint16_t ir, full;
//    ir = lum >> 16;
//    full = lum & 0xFFFF;
//    String lux_code_value;
//    unsigned int lux_value = lux_brigthness.calculateLux(full, ir);
//    if(lux_value < 200){
//      lux_code_value = "L00101";
//    }else if(lux_value >= 200 && lux_value <400){
//      lux_code_value = "L00102";      
//    }else if(lux_value >= 400 && lux_value <1200){
//      lux_code_value = "L00103";
//    }else if(lux_value >= 1200){
//      lux_code_value = "L00104";
//    }
//
//    if(deviceConnected && (rx_brigthness != lux_code_value)){
//      Serial.print("Brigthness val :");
//      Serial.println(rx_brigthness);
//      sendBLECodecData(rx_brigthness);
//    }
//    rx_brigthness = lux_code_value;
//    vTaskDelay(500);
  }
}

void BatStatusTask(void *pvParameters){
  (void) pvParameters;
  const int battPin = 35;
  float battValue = 0.0;
  for (;;){
    battValue = analogRead(battPin) * (6.9/4095.00);
    vTaskDelay(500);
    Serial.println(battValue);
  }
}


void AutoStopTask(void *pvParameters){
  (void) pvParameters;
  for (;;){
     for(int i=0;i<10;i++){
      t_now = millis();
      float dt = get_delta_time(t_now);
      accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
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
      vel = round(sqrt(pow(vel_x, 2) + pow(vel_y, 2)) * _KMPH);  
      datos[i]=vel; 
    }
    
    set_last_time(t_now);
  
    set_last_gyro_angle_x(gyro_angle_x);
    set_last_gyro_angle_y(gyro_angle_y);
  
    set_last_angle_x(angle_x);
    set_last_angle_y(angle_y);
 
    _autoStop();
    vTaskDelay(500);
  }
}
