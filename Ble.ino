#include <Wire.h> //Biblioteca para I2C
#include "DHT.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


BLECharacteristic *characteristicTX; //através desse objeto iremos enviar dados para o client

bool deviceConnected = false; //controle de dispositivo conectado

int pinLed = 18;
int pinTerm = 19;
DHT dht(pinTerm, DHT11);

#define SERVICE_UUID   "ab0828b1-198e-4351-b779-901fa0e0371e"
#define CHARACTERISTIC_UUID_RX  "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX  "0972EF8C-7613-4075-AD52-756F33D4DA91"


//callback para eventos das características
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  
    void onWrite(BLECharacteristic *characteristic) {
      
      //retorna ponteiro para o registrador contendo o valor atual da caracteristica
      std::string rxValue = characteristic->getValue();
      
      //verifica se existe dados (tamanho maior que zero)
      if (rxValue.length() > 0) {
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        
        Serial.println();
        if (rxValue.find("L1") != -1) {
          digitalWrite(pinLed, HIGH);
        }
        
        else if (rxValue.find("L0") != -1) {
          digitalWrite(pinLed, LOW);
        }
        
      }
    }//onWrite
    
};

//callback para receber os eventos de conexão de dispositivos
class ServerCallbacks: public BLEServerCallbacks {
  
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
    
};

void setup() {
  
  Serial.begin(9600);
  Serial.println("DHTxx test!");
  dht.begin();
  pinMode(pinLed, OUTPUT);

  // Create the BLE Device
  BLEDevice::init("ESP32-BLE"); // nome do dispositivo bluetooth
  // Create the BLE Server

  BLEServer *server = BLEDevice::createServer(); // cria um BLE server

  server -> setCallbacks(new ServerCallbacks()); // seta o callback do server

  // Create the BLE Service
  BLEService *service = server -> createService(SERVICE_UUID);

  // Create a BLE Characteristic para envio de dados
  characteristicTX = service -> createCharacteristic (
                       CHARACTERISTIC_UUID_TX,
                       BLECharacteristic::PROPERTY_NOTIFY
                     );
                     
  characteristicTX -> addDescriptor(new BLE2902());

  // Create a BLE Characteristic para recebimento de dados
  BLECharacteristic *characteristic = service -> createCharacteristic(
                                        CHARACTERISTIC_UUID_RX,
                                        BLECharacteristic::PROPERTY_WRITE
                                      );
  characteristic -> setCallbacks(new CharacteristicCallbacks());

  // Start the service
  service -> start();
  
  // Start advertising (descoberta do ESP32)
  server -> getAdvertising() -> start();
  
}

void loop() {
  
  //se existe algum dispositivo conectado
  if (deviceConnected) {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    Serial.print("Umidade: ");
    Serial.print(humidity);
    Serial.print(" %\t");
    Serial.print("Temperatura: ");
    Serial.print(temperature);
    Serial.println(" *C");

    char txString[8];
    dtostrf(temperature, 2, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer
    characteristicTX -> setValue(txString); //seta o valor que a caracteristica notificará (enviar)
    characteristicTX -> notify(); // Envia o valor para o smartphone
  }
  delay(3000);
}
