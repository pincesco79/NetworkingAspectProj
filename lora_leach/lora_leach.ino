
#include "Arduino.h"
#include "LoRa_E220.h"

#define battery_pin A0
#define analog_sensor_pin A1
#define comm_status_pin 23
#define channel 23

LoRa_E220 loraMod(17, 16, &Serial2, 15, 21, 19, UART_BPS_RATE_9600); //  esp32 RX <-- e220 TX, esp32 TX --> e220 RX AUX M0 M1

struct DataPacket {
  int senderId;    // ID del nodo che invia il pacchetto
  String type;     // Tipo di messaggio (ACK, DATA)
  String data;     // Dati trasmessi (solo per messaggi DATA)
  float battery;   // Livello di batteria
};

bool isClusterHead = false;
bool isBaseStation = false; // Imposta a true per il nodo stazione base
float batteryVoltage = 0.0;
int currentRound = 0;

void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);

void setup() {
  pinMode(comm_status_pin,OUTPUT);
  digitalWrite(comm_status_pin,HIGH);
  setSerial1();
  loraMod.begin();
  printLoraConfig(loraMod);

  batteryVoltage = readBatteryVoltage();
  //networkDiscovery();
  //
  //sendBroadcastMsg(loraMod);

 }

void loop() {

  currentRound++;
  // 1. Lettura della tensione della batteria e calcolo della probabilità
  float batteryVoltage = readBatteryVoltage();
  int threshold = calculateClusterHeadProbability(batteryVoltage);

  // 2. *********** Decisione di diventare capo-cluster
  if (!isBaseStation && random(0, 100) < threshold) {
    isClusterHead = true;
    Serial.println("Sono diventato capo-cluster");
    sendClusterHeadAnnouncement();
    
  // 3. ********* Cluster Set-up Phase ***********
  // During this phase, all cluster-head nodes must keep their receivers on and get RSSI level
    if (e220ttl.available()>1) {
    ResponseContainer rc = e220ttl.receiveMessageRSSI();
    // Is something goes wrong print error
    if (rc.status.code!=1){
      Serial.println(rc.status.getResponseDescription());
    }else{
      // Print the data received
      Serial.println(rc.status.getResponseDescription());
      String message = rc.data;
      Serial.println("Dati aggregati ricevuti: " + message);
      // SEND ACK LoRaSerial.println("ACK");
	  }
  }

  if (!isBaseStation && random(0, 100) < threshold) {
    isClusterHead = true;
    Serial.println("Sono diventato capo-cluster");
    sendClusterHeadAnnouncement();
  }



  // 4. Ricezione dati o invio dati a seconda del ruolo
  if (isBaseStation) {
    receiveAggregatedData();
  } else if (isClusterHead) {
    if (receiveMemberData()) {
      sendAggregatedDataToBaseStation();
    } else {
      Serial.println("Timeout nella ricezione dei dati dai membri");
    }
  } else {
    if (!sendDataToClusterHead()) {
      Serial.println("Errore nell'invio dei dati al capo-cluster");
    }
  }

  // 5. Modalità sleep per risparmio energetico
  //enterLightSleep(sleepDuration);

  // 5. Torna alla modalità WOR per ricevere i dati
  //setLoRaModeWOR();
  //Serial.println("Modulo LoRa impostato in modalità WOR");
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

setConfigurationBroadcastRSSI() {
  bool status = false;
  ResponseStructContainer c;
	c = e220ttl.getConfiguration();
	// It's important get configuration pointer before all other operation
	Configuration configuration = *(Configuration*) c.data;
	Serial.println(c.status.getResponseDescription());
	Serial.println(c.status.code);

//----------------------- BROADCAST MESSAGE RSSI 1 -----------------------
	configuration.ADDL = 0x01;
	configuration.ADDH = 0x00;
	configuration.CHAN = 23;

	configuration.SPED.uartBaudRate = UART_BPS_9600;
	configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
	configuration.SPED.uartParity = MODE_00_8N1;

	configuration.OPTION.subPacketSetting = SPS_200_00;
	configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
	configuration.OPTION.transmissionPower = POWER_22;

	configuration.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
	configuration.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
	configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
	configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;

	ResponseStatus rs = e220ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
	Serial.println(rs.getResponseDescription());
	Serial.println(rs.code);

	c = e220ttl.getConfiguration();
	// It's important get configuration pointer before all other operation
	configuration = *(Configuration*) c.data;
  status = c.status.getResponseDescription();
	Serial.println(status);
	if (Serial.println(c.status.code)) {
    status = true;
    printParameters(configuration);
  } else {
    status = false;
  }
	c.close();
  return status;
}

setConfigurationSenderRSSI() {
  bool status = false;
  ResponseStructContainer c;
	c = e220ttl.getConfiguration();
	// It's important get configuration pointer before all other operation
	Configuration configuration = *(Configuration*) c.data;
	Serial.println(c.status.getResponseDescription());
	Serial.println(c.status.code);

//	----------------------- FIXED SENDER RSSI -----------------------
	configuration.ADDL = 0x01;
	configuration.ADDH = 0x00;

	configuration.CHAN = 23;

	configuration.SPED.uartBaudRate = UART_BPS_9600;
	configuration.SPED.airDataRate = AIR_DATA_RATE_010_24;
	configuration.SPED.uartParity = MODE_00_8N1;

	configuration.OPTION.subPacketSetting = SPS_200_00;
	configuration.OPTION.RSSIAmbientNoise = RSSI_AMBIENT_NOISE_DISABLED;
	configuration.OPTION.transmissionPower = POWER_22;

	configuration.TRANSMISSION_MODE.enableRSSI = RSSI_ENABLED;
	configuration.TRANSMISSION_MODE.fixedTransmission = FT_FIXED_TRANSMISSION;
	configuration.TRANSMISSION_MODE.enableLBT = LBT_DISABLED;
	configuration.TRANSMISSION_MODE.WORPeriod = WOR_2000_011;
//
	ResponseStatus rs = e220ttl.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
	Serial.println(rs.getResponseDescription());
	Serial.println(rs.code);

	c = e220ttl.getConfiguration();
	// It's important get configuration pointer before all other operation
	configuration = *(Configuration*) c.data;
  status = c.status.getResponseDescription();
	Serial.println(status);
	if (Serial.println(c.status.code)) {
    status = true;
    printParameters(configuration);
  } else {
    status = false;
  }
	c.close();
  return status;
}


void printModuleInformation(struct ModuleInformation moduleInformation) {
	Serial.println("----------------------------------------");
	Serial.print(F("HEAD: "));  Serial.print(moduleInformation.COMMAND, HEX);Serial.print(" ");Serial.print(moduleInformation.STARTING_ADDRESS, HEX);Serial.print(" ");Serial.println(moduleInformation.LENGHT, DEC);

	Serial.print(F("Model no.: "));  Serial.println(moduleInformation.model, HEX);
	Serial.print(F("Version  : "));  Serial.println(moduleInformation.version, HEX);
	Serial.print(F("Features : "));  Serial.println(moduleInformation.features, HEX);
	Serial.println("----------------------------------------");
}

float readBatteryVoltage() { //Ok
  int rawValue = analogRead(battery_pin);
  float voltage = (rawValue / 4095.0) * 3.3 * 2; // Formula per calcolare tensione
  return voltage;
}

String readSensor() {//OK
  float temp = random(20, 30); // Simula temperatura tra 20 e 30 °C
  return String(temp);
}

void receiveAggregatedData() { //OK
  if (e220ttl.available()>1) {
    ResponseContainer rc = e220ttl.receiveMessageRSSI();
    // Is something goes wrong print error
    if (rc.status.code!=1){
      Serial.println(rc.status.getResponseDescription());
    }else{
      // Print the data received
      Serial.println(rc.status.getResponseDescription());
      String message = rc.data;
      Serial.println("Dati aggregati ricevuti: " + message);
      // SEND ACK LoRaSerial.println("ACK");
	  }
}

// Funzione per inviare dati aggregati alla stazione base
void sendAggregatedDataToBaseStation() { 
  //*************************
  //struct Message message = { "TEMP", ROOM, 0 };
  //message.temperature = Serial.parseFloat();
  // Send message
	ResponseStatus rs = e220ttl.sendFixedMessage(0, DESTINATION_ADDL, 23, &message, sizeof(Message));
	// Check If there is some problem of succesfully send
	Serial.println(rs.getResponseDescription());
  }

//*******************************************
  String message = "AGG_DATA:" + String(random(0, 100));
  LoRaSerial.println(message);
  Serial.println("Dati aggregati inviati alla stazione base");
}

void sendClusterHeadAnnouncement() { //OK
  String message = "CH:" + String(ESP.getEfuseMac());
  //LoRaSerial.println(message);
  ResponseStatus rs = e220ttl.sendBroadcastFixedMessage(23, message);
  // Check If there is some problem of succesfully send
  if (rs.getResponseDescription()) {
    Serial.println("Annuncio capo-cluster inviato");
  }
  //Serial.println(rs.getResponseDescription());
}

bool sendDataToClusterHead() {
  String message = "DATA:" + String(analogRead(analog_sensor_pin));
  //LoRaSerial.println(message);
  
  Serial.println("Dati inviati al capo-cluster");

  // Verifica della ricezione dell'ACK
  unsigned long startTime = millis();
  while (millis() - startTime < timeoutDuration) {
    if (LoRaSerial.available()) {
      String response = LoRaSerial.readStringUntil('\n');
      if (response.startsWith("ACK")) {
        Serial.println("ACK ricevuto dal capo-cluster");
        return true;
      }
    }
  }
  return false; // Timeout
}

// Funzione per ricevere dati dai membri del cluster
bool receiveMemberData() {
  unsigned long startTime = millis();
  while (millis() - startTime < timeoutDuration) {
    if (LoRaSerial.available()) {
      String message = LoRaSerial.readStringUntil('\n');
      Serial.println("Dati ricevuti dai membri: " + message);
      return true;
    }
  }
  return false; // Timeout
}

int calculateClusterHeadProbability(float voltage) {
  if (voltage > maxBatteryVoltage) voltage = maxBatteryVoltage;
  if (voltage < minBatteryVoltage) voltage = minBatteryVoltage;

  // Calcola la percentuale di carica (0-100%)
  int batteryLevel = ((voltage - minBatteryVoltage) / (maxBatteryVoltage - minBatteryVoltage)) * 100;
  Serial.print("Livello batteria (%): ");
  Serial.println(batteryLevel);
  return batteryLevel; // Utilizza la percentuale come probabilità
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