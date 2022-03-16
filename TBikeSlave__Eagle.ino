#include <Arduino.h>
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include <BLEDevice.h>
#include <Adafruit_NeoPixel.h>
//#include "BLEScan.h"
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
#define BatteryService BLEUUID((uint16_t)0x180F)

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
BLECharacteristic* pCharacteristic = NULL;


#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define LED_PIN    15 //Pin para tira led
#define LED_COUNT   26 // ¿Cuantos leds se van a controlar?
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800); //Declaración del objeto strip, que controlará la tira led

void StripTask( void *pvParameters );
String dato = "";
uint32_t stripColor;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("data: ");
    Serial.println((char*)pData);
    dato=(char*)pData;    
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  strip.begin(); //Inicializa el objeto Strip -> tira led
  strip.show(); //Enciende/ muestra la tira led -----> Si todo se ajustó bien, deberá encender la tira led
  stripColor = strip.Color(127, 45, 50);

   xTaskCreatePinnedToCore(
    StripTask
    ,  "StripTask"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
} // End of setup.

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(1000);
}

void StripTask(void *pvParameters){
  (void) pvParameters;
  for (;;){   
     strip.clear();  
     if(!connected){
      searchServer();
     }else{   
       if(dato=="D00100"){
        chaseLeft(strip.Color(255, 0, 0));
       }else if(dato=="D00010"){
        theaterChase(stripColor,50);
       }else if(dato=="D00001"){
        chaseRight(strip.Color(255, 0, 0));
       }else{
        theaterChase(stripColor, 50);
       }  
       
       if(dato == "L00101"){
        strip.setBrightness(64);
       }else if(dato == "L00102"){
        strip.setBrightness(128);
       }else if(dato == "L00103"){
        strip.setBrightness(192);
       }else if(dato == "L00104"){
        strip.setBrightness(255);
       }    

       if(dato == "C10800"){
        stripColor = strip.Color(198, 40, 40);
       }else if(dato == "C20800"){
        stripColor = strip.Color(239, 108, 0);
       }else if(dato == "C30800"){
        stripColor = strip.Color(249, 168, 37);
       }else if(dato == "C40800"){
        stripColor = strip.Color(46, 125, 50);
       }else if(dato == "C50800"){
        stripColor = strip.Color(2, 119, 189);
       }else if(dato == "C60800"){
        stripColor = strip.Color(21, 101, 192);
       }else if(dato == "C70800"){
        stripColor = strip.Color(106, 27, 154);
       }else if(dato == "C80800"){
        stripColor = strip.Color(227, 81, 31);
       }

       if(dato == "G10001"){
        stripColor = strip.Color(255, 0, 0);
       }
     }
  }
}


static void chase(uint32_t c) {
  for(uint16_t i=0; i<LED_COUNT +4; i++) {
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.setPixelColor(i-4, 0); // Erase pixel a few steps back
      strip.show();
      delay(25);
  }
}

static void chaseLeft(uint32_t c) {
  for(uint16_t i=LED_COUNT; i>=LED_COUNT/2; i--) {
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.setPixelColor(i-4, 0); // Erase pixel a few steps back
      strip.show();
      delay(25);
  }
}

static void chaseRight(uint32_t c){
  for(uint16_t i=0; i<=LED_COUNT/2; i++){
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.setPixelColor(i-4, 0); // Erase pixel a few steps back
      strip.show();
      delay(25);
  }
}

static void sampleLight(uint32_t c){
  for(uint16_t i=0; i<=LED_COUNT; i++){
      strip.setPixelColor(i  , c); // Draw new pixel
      strip.show();
      delay(25);
  }
}

void theaterChase(uint32_t c, uint8_t wait){
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}


void rainbow(uint8_t wait){
  uint16_t i, j;
  for(j=0; j<256; j++){
    for(i=0; i<strip.numPixels(); i++){
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void searchServer(){
  for(uint16_t i=0; i< strip.numPixels(); i++) {
    strip.clear();
    strip.setPixelColor(i+1, random(1,254), random(1,254), random(1,254));
    strip.setPixelColor(i, random(1,254), random(1,254), random(1,254));
    strip.show();
    delay(20);
  }
   delay(50);
   for(uint16_t i=strip.numPixels()-2; i>0 ; i--) {
      strip.clear();
      strip.setPixelColor(i, random(1,254), random(1,254), random(1,254));
      strip.setPixelColor(i-1, random(1,254), random(1,254), random(1,254));
      strip.show();
      delay(20);
  }
}

uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
