/***************************************************************
 * NODE 3: Rain Sensor
 ***************************************************************/
#include "Arduino.h"
#include "LoRa_E220.h"
#include <Pangodream_18650_CL.h>


// ------------------- PINS AND DEFINITIONS -------------------
#define RAIN_PWR 2  
#define RAIN_PIN 5  

#define ADC_PIN 4     
#define CONV_FACTOR 3.1
#define READS 20

#define AUX_PIN 15


Pangodream_18650_CL battery(ADC_PIN, CONV_FACTOR, READS);

int getBatteryChargeLevel(float voltage) {
  const float VOLTAGE_MIN = 2.4;
  const float VOLTAGE_MAX = 3.7;
  if (voltage >= VOLTAGE_MAX) return 100;
  if (voltage <= VOLTAGE_MIN) return 0;
  return (voltage / VOLTAGE_MAX) * 100;
}

// ------------------- STATES AND CONFIG -----------------------
enum NodeState {
  SEARCHING_CH,
  CLUSTER_HEAD,
  MEMBER,
  WAITING_COOLDOWN
};

const int dry = 2400;
const int wet = 900;
int rainValue;

// IMPORTANT: NODE 3
const uint8_t NODE_ID = 3; 
const uint8_t TOTAL_NODES = 3;


LoRa_E220 e220ttl(17, 16, &Serial2, AUX_PIN, 21, 19, UART_BPS_RATE_9600);
//LoRa_E220(byte txE220pin, byte rxE220pin, HardwareSerial* serial, byte auxPin, UART_BPS_RATE bpsRate, uint32_t serialConfig = SERIAL_8N1);
volatile bool interruptExecuted = false;

const int ledPin = 23;

struct receivedData {
  int humidity;
  float temperature;
  float soilMoisture;
  bool rain;
  float batteryLevel;
};

receivedData currentData = {0, 0, 0, false, 0};
NodeState currentState = SEARCHING_CH;
    
unsigned long lastClusterHeadTime = 0; 
const unsigned long cooldownTime = 60000; 
const unsigned long roundLengthTime = 60000; 
unsigned long memberCheckTime = 25000;

void loraSetup() {
 if (!e220ttl.begin()) {
    Serial.println("Error initializing E220 module");
    while (1);
  }
  Serial.print("E220 module initialized successfully. Node ID: ");
  Serial.println(NODE_ID);
  e220ttl.setMode(MODE_2_WOR_RECEIVER);

  Serial.println("Start sleep!");
  delay(100);
  attachInterrupt(digitalPinToInterrupt(AUX_PIN), loraWakeUp, FALLING);
}

void loraWakeUp() {
  interruptExecuted = true;
  //detachInterrupt(digitalPinToInterrupt(AUX_PIN));
}

void leachSetupPhase() {
  leachCHAdvPhase();
  leachClusterSetupPhase();
  leachBroadCastSchedule();
}

void leachCHAdvPhase() {
  if (e220ttl.available()>1) {
    Serial.println("Message arrived: ");
      // read the String message
    ResponseContainer rc = e220ttl.receiveMessage();
    String message = rc.data;
    //Serial.println(rc.status.getResponseDescription());
    Serial.print(message);
    e220ttl.setMode(MODE_0_NORMAL); // change to normal mode
    delay(1000);
    e220ttl.sendBroadcastFixedMessage(23, "We have received the message!");
    e220ttl.setMode(MODE_2_WOR_RECEIVER); // change back to WOR receiver mode
    interruptExecuted = false;
  }

  if(interruptExecuted) {
    //Serial.println("WakeUp Callback, AUX pin go LOW and start receive message!");
    Serial.flush();
    //attachInterrupt(digitalPinToInterrupt(AUX_PIN), wakeUp, FALLING);
    interruptExecuted = false;
  }
}

void leachClusterSetupPhase(){

}

void leachBroadCastSchedule(){

}

void setup() {
  Serial.begin(9600);
  delay(2000);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  pinMode(RAIN_PWR, OUTPUT);
  pinMode(RAIN_PIN, INPUT);

  loraSetup();
  randomSeed(analogRead(0) * NODE_ID);
}



void loop() {
  currentState = SEARCHING_CH;  
  leachSetupPhase();
  //e220ttl.sendMessage(&receivedData,sizeof(receivedData));
  //e220ttl.sendBroadcastFixedMessage(23, &currentData, sizeof(currentData));

}

