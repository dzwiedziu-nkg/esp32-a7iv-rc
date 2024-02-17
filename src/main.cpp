#include <Arduino.h>
#include <BLEDevice.h>

// Please enter your camera bluetooth name. My Sony A7IV name is:
#define bleServerName "ILCE-7M4"

uint8_t PRESS_TO_FOCUS[] = {0x01, 0x07};
uint8_t TAKE_PICTURE[] = {0x01, 0x09};
uint8_t SHUTTER_RELEASED[] = {0x01, 0x06};
uint8_t HOLD_FOCUS[] = {0x01, 0x08};

static BLEAddress *pServerAddress;
static boolean doPairing = false;
static boolean doConnect = false;
static boolean connected = false;
BLERemoteCharacteristic* remoteCommand;
BLERemoteCharacteristic* remoteNotify;

// helper function for print debug
void printHex(uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (data[i] < 0x10) {
      Serial.print("0x0");
    } else {
      Serial.print("0x");
    }
    Serial.print(data[i], HEX);
    if (i < length - 1) {
      Serial.print(" ");
    }
  }
}

static void commandNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("Received from command channel: ");
  printHex(pData, length);
  if (isNotify) {
    Serial.println (" - notify");
  } else {
    Serial.println (" - not notify");
  }
}

static void notifyNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  Serial.print("Received from notify channel: ");
  printHex(pData, length);
  if (isNotify) {
    Serial.println (" - notify");
  } else {
    Serial.println (" - not notify");
  }
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE: something found... ");
    Serial.println(advertisedDevice.getName().c_str());
    if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      Serial.println("Camera found. Connecting!");
      Serial.print("Payload: ");
      printHex(advertisedDevice.getPayload(), advertisedDevice.getPayloadLength());
      Serial.println("");
      auto data = advertisedDevice.getPayload();
      for (size_t i = 1; i < advertisedDevice.getPayloadLength(); i++) {
        if (data[i-1] == 0x22) {
          if ((data[i] & 0x40) == 0x40 && (data[i] & 0x02) == 0x02) {
            Serial.println("Camera is ready to paring");
            doPairing = true;
          } else {
            doConnect = true;
            Serial.println("Camera is not ready to paring, but try to connect");
          }
        }
      }
    }
  }
};

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connected");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected");
  }
};

// Conn
bool connectToServer(BLEAddress pAddress) {
   BLEClient* pClient = BLEDevice::createClient();
   pClient->setClientCallbacks(new MyClientCallback());
 
  // Connect to the remove BLE Server.
  if (pClient->connect(pAddress)) {
    Serial.println(" - Connected to server");
    doPairing = false;
    doConnect = false;

    BLERemoteService* pRemoteService = pClient->getService("8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF");
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID");
      return false;
    }

    remoteCommand = pRemoteService->getCharacteristic(BLEUUID((uint16_t)0xFF01));
    remoteNotify = pRemoteService->getCharacteristic(BLEUUID((uint16_t)0xFF02));

    if (remoteCommand == nullptr) {
      Serial.println("Failed to find our characteristic command");
      return false;
    }

    if (remoteNotify == nullptr) {
      Serial.println("Failed to find our characteristic notify");
      return false;
    }
    Serial.println("Camera BLE service and characteristic found");

    remoteCommand->registerForNotify(commandNotifyCallback);
    remoteNotify->registerForNotify(notifyNotifyCallback);

    connected = true;

    return true;
  } else {
    Serial.println(" - fail to BLE connect");
    return false;
  }

}

// Accept any pair request from Camera
class MySecurityCallbacks: public BLESecurityCallbacks {

  uint32_t onPassKeyRequest(){
    Serial.println("PassKeyRequest");
    return 123456;
  }

  void onPassKeyNotify(uint32_t pass_key){
    Serial.print("The passkey Notify number:");
    Serial.println(pass_key);
  }

  bool onSecurityRequest(){
    Serial.println("SecurityRequest");
    return true;
  }

  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
    Serial.println("Authentication Complete");
    if(cmpl.success){
      Serial.println("Pairing success");
    }else{
      Serial.println("Pairing failed");
    }
  }

  bool onConfirmPIN(uint32_t pin) {
    return true;
  }
};


void pairOrConnect() {
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  Serial.println("BLE: Looking for camera");
  pBLEScan->start(30);
  Serial.println("BLE: end of searching");
}


void setup() {
  Serial.begin(115200);  // default boot baudrate
  esp_log_level_set("*", ESP_LOG_VERBOSE); // verbose logs

  //Init BLE device
  BLEDevice::init("nkg RC demo v0.1");
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new MySecurityCallbacks());
 
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  pairOrConnect();
}

void loop() {
  if (doPairing || doConnect) {
    connectToServer(*pServerAddress);
  }

  // take next photo by 10s
  delay(10000);

  if (connected) {
    Serial.println("BLE: take photo");

    remoteCommand->writeValue(PRESS_TO_FOCUS, 2, true);
    delay(100);
    remoteCommand->writeValue(TAKE_PICTURE, 2, true);
    delay(100);
    remoteCommand->writeValue(SHUTTER_RELEASED, 2, true);
    delay(100);
    remoteCommand->writeValue(HOLD_FOCUS, 2, true);
    delay(100);
  } else {
    pairOrConnect();
  }
}
