#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pRxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

bool dataDump = false;
uint8_t data[] = "This is my entire data string this could be of any lenght let see what happens";
byte pIndex = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
        dataDump = true;
      }
    }
};


void setup() {
  Serial.begin(9600);

  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  // pTxCharacteristic = pService->createCharacteristic(
	// 									CHARACTERISTIC_UUID_TX,
	// 									BLECharacteristic::PROPERTY_NOTIFY
	// 								);
                      
  //pTxCharacteristic->addDescriptor(new BLE2902());

  pRxCharacteristic = pService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX,
											BLECharacteristic::PROPERTY_WRITE | 
                      BLECharacteristic::PROPERTY_NOTIFY
										);

  pRxCharacteristic->setCallbacks(new MyCallbacks());
  //pRxCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  if (dataDump){
    // Get real buffer size based off what's left in data to send.
      byte buffer_size = 20;
      uint8_t buffer[buffer_size];
  
      // Load in the next buffer of bytes
      for (int i=0;i<buffer_size;i++){
        if (pIndex+i < sizeof(data)){
          buffer[i] = data[pIndex+i];
        }else{
          buffer[i] = 0;
        }
      }
  
      pIndex += buffer_size;
      if (pIndex >= sizeof(data)){
        pIndex = 0;
        dataDump = false;
      }
      
      Serial.println((char*)buffer);
      
      if (deviceConnected) {
          pRxCharacteristic->setValue(buffer, buffer_size);
          pRxCharacteristic->notify();
          txValue++;
    		delay(10); // bluetooth stack will go into congestion, if too many packets are sent
    	}
  
      delay(1000);
    }
    
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
		// do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}