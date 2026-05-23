#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLESecurity.h>
#include "DHTesp.h" 

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

#define SERVICE_UUID  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define PUMP1_RELAY_PIN 26 
#define PUMP2_RELAY_PIN 25 
#define LAMP_RELAY_PIN 27 
#define CONECTION_LED 4
#define BUZZER_STATE 5
#define GasSensorPin 34
#define DHTPIN 32
#define SOLI_SENSOR_PIN 33
#define TRIG 14
#define ECHO 13

DHTesp dht; 

bool alertOnDisconnect = false;

int chk;
float hum; 
float temp; 

void AlertState(int length){
  for(int i = 0; i < length; i++){
    digitalWrite(BUZZER_STATE, HIGH);
    delay(200);
    digitalWrite(BUZZER_STATE, LOW);
    delay(200); 
  }
}

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("✅ Device Connected");
    digitalWrite(CONECTION_LED, HIGH);
    AlertState(2);
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("❌ Device Disconnected");
    digitalWrite(CONECTION_LED, LOW); 
    BLEDevice::startAdvertising();
    alertOnDisconnect = true; 
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    
    String value = pChar->getValue();
  
       if(value == "PUMP1_ON"){
        digitalWrite(PUMP1_RELAY_PIN,LOW);
        digitalWrite(BUZZER_STATE,HIGH);
                return ;
       }else if(value =="PUMP1_OFF"){
        digitalWrite(PUMP1_RELAY_PIN,HIGH);
        digitalWrite(BUZZER_STATE,LOW);
                return ;
       };

       if(value == "PUMP2_ON"){
        digitalWrite(PUMP2_RELAY_PIN,LOW);
        digitalWrite(BUZZER_STATE,HIGH);
        return ;
       }else if(value =="PUMP2_OFF"){
        digitalWrite(PUMP2_RELAY_PIN,HIGH);
        digitalWrite(BUZZER_STATE,LOW);
                return ;
       };

       if(value == "LAMP_ON"){
        digitalWrite(LAMP_RELAY_PIN,LOW);
        digitalWrite(BUZZER_STATE,HIGH);
        return ;
       }else if(value =="LAMP_OFF"){
        digitalWrite(LAMP_RELAY_PIN,HIGH);
        digitalWrite(BUZZER_STATE,LOW);
                return ;
       };


    if (value.length() > 0) {
      int num = value.toInt();

      Serial.print("📥 Received: ");
      Serial.println(num);

      switch(num) {
        case 1:
          Serial.println("🌱 طماطم");
          digitalWrite(PUMP1_RELAY_PIN, HIGH);
          break;
        case 2:
          Serial.println("🥔 بطاطس");
          digitalWrite(PUMP1_RELAY_PIN, LOW);
          break;
        case 3:
          Serial.println("🧅 بصل");
          break;
        case 4:
          Serial.println("🥬 خس");
          break;
        case 5:
          Serial.println("🥕 جزر");
          break;
        case 6:
          Serial.println("🌶️ فلفل");
          break;
        default:
          Serial.println("❌ Unknown");
      }
    }
  }
};


void setup() {
  Serial.begin(115200);

pinMode(PUMP1_RELAY_PIN, OUTPUT);
pinMode(PUMP2_RELAY_PIN, OUTPUT);
pinMode(LAMP_RELAY_PIN, OUTPUT);
  pinMode(CONECTION_LED, OUTPUT);
  pinMode(BUZZER_STATE, OUTPUT);
  digitalWrite(PUMP1_RELAY_PIN, HIGH);       
  digitalWrite(PUMP2_RELAY_PIN, HIGH);       
  digitalWrite(LAMP_RELAY_PIN, HIGH);       
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  dht.setup(DHTPIN, DHTesp::DHT22);

  BLEDevice::init("ESP32_BLE");

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_NO_BOND);

  BLEDevice::setPower(ESP_PWR_LVL_P9);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  BLEDevice::startAdvertising();

  Serial.println("🚀 ESP32 Ready...");
}

void loop() {

digitalWrite(TRIG, LOW);
delayMicroseconds(5);

digitalWrite(TRIG, HIGH);
delayMicroseconds(10);
digitalWrite(TRIG, LOW);

float distance = -1;
int waterPercent = 0;

long duration = pulseIn(ECHO, HIGH, 7000);

if(duration == 0){
  Serial.println("⚠️ No echo received");
} else {
  distance = duration * 0.034 / 2;


if(distance <= 5){
    waterPercent = 100;
}
else if(distance <= 10){
    waterPercent = 75;
}
else if(distance <= 15){
    waterPercent = 70;
}
else if(distance <= 20){
    waterPercent = 60;
}
else if(distance <= 25){
    waterPercent = 50;
}
else if(distance <= 30){
    waterPercent = 40;
}
else if(distance <= 35){
    waterPercent = 30;
}
else if(distance <= 40){
    waterPercent = 20;
}
else{
    waterPercent = 10;
}

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Serial.println(" waterPercent",waterPercent);
}


  if(alertOnDisconnect){
    AlertState(4);    
    alertOnDisconnect = false;
  }

  int soli = analogRead(SOLI_SENSOR_PIN);

    Serial.println("soli => ");
    Serial.println(soli);

  int isGas = analogRead(GasSensorPin);
  int gas = 600;
  if(isGas > gas){
    Serial.println("Gas detected");

    if(deviceConnected && pCharacteristic != NULL){
      uint32_t val = isGas;
      pCharacteristic->setValue(val);
      pCharacteristic->notify();
    }
    
    
    digitalWrite(BUZZER_STATE, HIGH);
    delay(1000);
    digitalWrite(BUZZER_STATE, LOW);
    delay(1000); 
    digitalWrite(BUZZER_STATE, HIGH);
    delay(1000);
    digitalWrite(BUZZER_STATE, LOW);
    delay(1000); 
  }

    

  Serial.println("Gas value => ");
  Serial.println(isGas);
  Serial.println("waterPercent value => ");
  Serial.println(waterPercent);

    hum = dht.getHumidity();
  temp = dht.getTemperature();

if(deviceConnected && pCharacteristic != NULL){
    
    String sensorData = 
"T:" + String(temp, 1) + 
",H:" + String(hum, 1) + 
",G:" + String(isGas) +
",S:" + String(soli) +
",D:" + String(distance, 1)+
",W:" + String(waterPercent);
    
    pCharacteristic->setValue(sensorData.c_str());
    pCharacteristic->notify();
    
    Serial.print("Sent to App => ");
    Serial.println(sensorData);
  }

//  digitalWrite(PUMP1_RELAY_PIN, HIGH);
//     delay(1000);
//     digitalWrite(PUMP1_RELAY_PIN, LOW);

  delay(2000); 
}
