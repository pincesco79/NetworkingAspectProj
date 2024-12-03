
#include "Arduino.h"
#include "LoRa_E220.h"

#define battery_pin A0
#define comm_status_pin 23

LoRa_E220 loraMod(17, 16, &Serial2, 15, 21, 19, UART_BPS_RATE_9600); //  esp32 RX <-- e220 TX, esp32 TX --> e220 RX AUX M0 M1

struct DataPacket {
  int senderId;    // ID del nodo che invia il pacchetto
  String type;     // Tipo di messaggio (ADV, REQ, DATA)
  String metadata; // Metadati relativi al dato
  String data;     // Dati trasmessi (solo per messaggi DATA)
  float battery;   // Livello di batteria
};

float batteryVoltage = 0.0;

void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);

void setup() {
  pinMode(comm_status_pin,OUTPUT);
  digitalWrite(comm_status_pin,HIGH);
  setSerial1();
  loraMod.begin();
  printLoraConfig(loraMod);

  batteryVoltage = readBattery();
  //networkDiscovery();
  //
  //sendBroadcastMsg(loraMod);

 }

void loop() {
  //sensorValueRead();
  //sensorValueSend();
  //
}

void setSerial1() {
  Serial.begin(9600);
	while(!Serial){};
	delay(500);
	Serial.println();
}

void printLoraConfig(LoRa_E220 e220ttl) {
  ResponseStructContainer c;
	c = e220ttl.getConfiguration();
	// // It's important get configuration pointer before all other operation
	Configuration configuration = *(Configuration*) c.data;
	Serial.println(c.status.getResponseDescription());
	Serial.println(c.status.code);
	printParameters(configuration);
  
}

void printParameters(struct Configuration configuration) {
	Serial.println("----------------------------------------");
	Serial.print(F("HEAD : "));  Serial.print(configuration.COMMAND, HEX);Serial.print(" ");Serial.print(configuration.STARTING_ADDRESS, HEX);Serial.print(" ");Serial.println(configuration.LENGHT, HEX);
	Serial.println(F(" "));
	Serial.print(F("AddH : "));  Serial.println(configuration.ADDH, HEX);
	Serial.print(F("AddL : "));  Serial.println(configuration.ADDL, HEX);
	Serial.println(F(" "));
	Serial.print(F("Chan : "));  Serial.print(configuration.CHAN, DEC); Serial.print(" -> "); Serial.println(configuration.getChannelDescription());
	Serial.println(F(" "));
	Serial.print(F("SpeedParityBit     : "));  Serial.print(configuration.SPED.uartParity, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getUARTParityDescription());
	Serial.print(F("SpeedUARTDatte     : "));  Serial.print(configuration.SPED.uartBaudRate, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getUARTBaudRateDescription());
	Serial.print(F("SpeedAirDataRate   : "));  Serial.print(configuration.SPED.airDataRate, BIN);Serial.print(" -> "); Serial.println(configuration.SPED.getAirDataRateDescription());
	Serial.println(F(" "));
	Serial.print(F("OptionSubPacketSett: "));  Serial.print(configuration.OPTION.subPacketSetting, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getSubPacketSetting());
	Serial.print(F("OptionTranPower    : "));  Serial.print(configuration.OPTION.transmissionPower, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getTransmissionPowerDescription());
	Serial.print(F("OptionRSSIAmbientNo: "));  Serial.print(configuration.OPTION.RSSIAmbientNoise, BIN);Serial.print(" -> "); Serial.println(configuration.OPTION.getRSSIAmbientNoiseEnable());
	Serial.println(F(" "));
	Serial.print(F("TransModeWORPeriod : "));  Serial.print(configuration.TRANSMISSION_MODE.WORPeriod, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getWORPeriodByParamsDescription());
	Serial.print(F("TransModeEnableLBT : "));  Serial.print(configuration.TRANSMISSION_MODE.enableLBT, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getLBTEnableByteDescription());
	Serial.print(F("TransModeEnableRSSI: "));  Serial.print(configuration.TRANSMISSION_MODE.enableRSSI, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getRSSIEnableByteDescription());
	Serial.print(F("TransModeFixedTrans: "));  Serial.print(configuration.TRANSMISSION_MODE.fixedTransmission, BIN);Serial.print(" -> "); Serial.println(configuration.TRANSMISSION_MODE.getFixedTransmissionDescription());


	Serial.println("----------------------------------------");
}

void printModuleInformation(struct ModuleInformation moduleInformation) {
	Serial.println("----------------------------------------");
	Serial.print(F("HEAD: "));  Serial.print(moduleInformation.COMMAND, HEX);Serial.print(" ");Serial.print(moduleInformation.STARTING_ADDRESS, HEX);Serial.print(" ");Serial.println(moduleInformation.LENGHT, DEC);

	Serial.print(F("Model no.: "));  Serial.println(moduleInformation.model, HEX);
	Serial.print(F("Version  : "));  Serial.println(moduleInformation.version, HEX);
	Serial.print(F("Features : "));  Serial.println(moduleInformation.features, HEX);
	Serial.println("----------------------------------------");
}

float readBattery() {
  int rawValue = analogRead(battery_pin);
  float voltage = (rawValue / 4095.0) * 3.3 * 2; // Formula per calcolare tensione
  return voltage;
}

String readSensor() {
  float temp = random(20, 30); // Simula temperatura tra 20 e 30 °C
  return String(temp);
}

/*DataPacket createPacket(String type, String metadata, String data) {
  DataPacket packet;
  packet.senderId = NODE_ID;
  packet.type = type;
  packet.metadata = metadata;
  packet.data = data;
  packet.battery = batteryVoltage;
  return packet;
}*/