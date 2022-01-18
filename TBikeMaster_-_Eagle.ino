#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif

#include <AceButton.h>
#include "MPU6050.h"
#include "I2Cdev.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include "Wire.h"
#endif

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include "Gyro.h"
#include "Luxer.h"

Gyro eagle_gyro;
Luxer eagle_luxer(15);

const int CENTER_BUTTON_PIN = 12;
const int RIGTH_BUTTON_PIN = 14;
const int LEFT_BUTTON_PIN = 13;
const int POWER_SYS_LED = 26;
const int DIR_LEFT_LED = 25;
const int DIR_RIGHT_LED = 27;

using namespace ace_button;
AceButton centerButton(CENTER_BUTTON_PIN);
AceButton rightButton(RIGTH_BUTTON_PIN);
AceButton leftButton(LEFT_BUTTON_PIN);

//void LuxBrigTask( void *pvParameters );

int state = 1;
bool isDirActive = false;
String _direction;
unsigned long tiempo;
#define DELAY 10000

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
String _rxValue;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

void handleEvent(AceButton* button, uint8_t eventType,
    uint8_t /*buttonState*/) {
  uint8_t pin = button->getPin();

  if (pin == CENTER_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventPressed:         
          state++;
          if (state > 2) state = 1;
          Serial.println(F("Mode Button: Pressed"));
          Serial.print(F("New state: "));
          Serial.println(state);
        break;
      case AceButton::kEventLongPressed:
          unsigned long intCh1Rise = micros();
          tiempo = (micros() - intCh1Rise) / 1000;
          if(millis()-tiempo>3000){            
            digitalWrite(POWER_SYS_LED, LOW);
            delay(1000);
            esp_deep_sleep_start();
          }        
        break;
    }
  } else if (pin == LEFT_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventPressed:
      case AceButton::kEventRepeatPressed:
        
          if (eventType == AceButton::kEventPressed) {
            Serial.println(F("Left Pressed"));
            digitalWrite(DIR_LEFT_LED, HIGH);
            digitalWrite(DIR_RIGHT_LED, LOW);
            isDirActive = true;
            tiempo = millis();
            _direction = "Left";
            //_rxValue = "D00001";
            //sendBLECodecData(_rxValue);
          } else {
            Serial.println(F("Change Button: Repeat Pressed"));
            digitalWrite(DIR_LEFT_LED, LOW);
            //isDirActive = false;
          }
        break;
        
      case AceButton::kEventLongReleased:
        break;
      case AceButton::kEventDoubleClicked:
      
          Serial.println(F("Change Button 2: Repeat"));
           digitalWrite(DIR_LEFT_LED, LOW);
           isDirActive = false;
           //_rxValue = "D00010";
          //sendBLECodecData(_rxValue);
        break;
    }
  } else if (pin == RIGTH_BUTTON_PIN) {
    switch (eventType) {
      case AceButton::kEventPressed:
      case AceButton::kEventRepeatPressed:
          if (eventType == AceButton::kEventPressed) {
            Serial.println(F("Change Button 2: Pressed"));
            digitalWrite(DIR_LEFT_LED, LOW);
            digitalWrite(DIR_RIGHT_LED, HIGH); 
            isDirActive = true;
            tiempo = millis();
            _direction = "Right";
            //_rxValue = "D00100";
            //sendBLECodecData(_rxValue);
          } else {
            Serial.println(F("Change Button 2: Repeat Pressed"));
             digitalWrite(DIR_RIGHT_LED, LOW);
             isDirActive = false;
          }
        break;

      case AceButton::kEventLongReleased:
        break;
      case AceButton::kEventDoubleClicked:
          Serial.println(F("Change Button 2: Repeat"));
          digitalWrite(DIR_RIGHT_LED, LOW);
          isDirActive = false;
          //_rxValue = "D00010";
          //sendBLECodecData(_rxValue);
        break;    
    }
  }
}
void setup() {
  delay(1000);
  Serial.begin(38400);//Configuracion de baudios
  while (! Serial);//Espera un dato en el puerto serial
  Serial.println(F("setup(): begin"));
  Serial.println(F("Enable GPIOs!"));  
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_12,0); // Pin para despertar el ESP
  pinMode(CENTER_BUTTON_PIN, INPUT_PULLUP// Configuracion pulsador centro
  pinMode(RIGTH_BUTTON_PIN, INPUT_PULLUP);// Configuracion pulsador derecho
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);// Configuracion pulsador izquierdo
  pinMode(POWER_SYS_LED, OUTPUT); // Configuracion led centro
  pinMode(DIR_LEFT_LED, OUTPUT);// Configuracion direccional izquierda
  pinMode(DIR_RIGHT_LED, OUTPUT);// Configuracion direccional derecha

  Serial.println(F("Configure Buttons envents handler!"));   
  ButtonConfig* buttonConfig = ButtonConfig::getSystemButtonConfig(); //Creacion de objeto ButtonConfig
  buttonConfig->setEventHandler(handleEvent);//Creacion de eventos
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);//configuracion pulsador cuando se mantiene presionado
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);//configuracion pulsador cuando se oprime 2 veces con un tiempo prolongado entre pulsos
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);//configuracion pulsador cuandose oprime 2 veces en corto tiempo
  buttonConfig->setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);//Suprime multiples pulsos

  Serial.println(F("Starting TEMP6000"));
  eagle_luxer.calibration();
  Serial.println(F("Starting MPU6050"));
   #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
      Wire.begin();
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
      Fastwire::setup(400, true);
  #endif
  eagle_gyro.calibration();  
  Serial.println(F("End setup()"));
  Serial.println(F("Enjoy light"));
  digitalWrite(POWER_SYS_LED, HIGH);
}

void timingAutoOff(){                     //Tiempo de apagado automatico direccionales en 5 seg
  if(millis()-tiempo>DELAY && isDirActive){
    tiempo=0;
    isDirActive = false;
    //_rxValue = "D00010";
    //sendBLECodecData(_rxValue);
    if(_direction == "Right"){
      digitalWrite(DIR_RIGHT_LED, LOW);
    }else if(_direction == "Left"){
      digitalWrite(DIR_LEFT_LED, LOW);
    }    
  }
}

/*void LuxBrigTask(void *pvParameters){
  (void) pvParameters;  
  for (;;){    
  const int luxPin = 15;
  float luxRead, luxValue;
  String lux_code_value;
  
    luxRead = analogRead(luxPin);
    luxValue = luxRead / 4095.0; 
    luxValue = pow(luxValue, 2.0);
    vTaskDelay(500);
    if(luxValue < 0.3){
      lux_code_value = "L00101";
    }else if(luxValue >= 0.3 && luxValue <0.6){
      lux_code_value = "L00102";      
    }else if(luxValue >= 0.6 && luxValue <0.9){
      lux_code_value = "L00103";
    }else if(luxValue >= 0.9){
      lux_code_value = "L00104";
    }
    rx_brigthness = lux_code_value;  
  }
}*/

void loop() {
  centerButton.check();
  rightButton.check();
  leftButton.check();
  timingAutoOff();
  eagle_gyro.readerImu();
}
