//
//  MAIN
//
//
//  20231231-1000
//     
//


// Chosen board
#define HELTEC_WIFI_LORA_32_V2            // Board selected

//
// LIBRARIES
//

#include "Arduino.h"
//#include "Time.h"
#include "TimeLib.h"
//#include "hwcrypto.h"
#ifdef HELTEC_WIFI_LORA_32_V2
#include "heltec.h"
#endif
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "FS.h"
#include "HVDN_config.h"
#include "HVDN_api.h"
#include "HVDN_logo.h"


//
// DEFINES
//
#define WIFI_POLL_DELAY 500
#define WIFI_POLL_TRIES 20
#define BAND 911250000                    // you can set band here directly, ( 868E6,915E6 )

//
// VARIABLES
//

boolean MyRX_Reset;                       // A trigger to restart LoRa Radio

//Set Servers
const uint16_t WEB_PORT = 8000;
const uint16_t WS_PORT = 8888;

//Task handles
TaskHandle_t TaskTRX;
TaskHandle_t TaskWebsox;
//TaskHandle_t TaskBeacon;

// Configure LoRa
// byte localaddressLORA = 0xBB;          // LoRa address of this device (irrelevant)
byte destinationLORA = 0xFF;              // LoRa destination to send to (broadcast default)

// Default settings before config.json load
String channel = "HV1";                   // Channel
String rfmodule = "RFM9X";                // RF Module
String modemconfig = "Bw125Cr45Sf128";    // Modemstring Radiohead
int modem = 0;
int frequency = 911250000;                // default settings for safe LoRa module init
int spreadfactor = 7;
int codingrate4 = 8;
int bandwidth = 125000;
int txpwr = 11;
String mycall = "NOCALL";
String myssid = "00";
String mybeacon = "QRZ? QRZ? QRZ?";       // Baacon Message
String dstcall = "BEACON";                // Destination Call ( BEACON )
String dstssid = "99";                    // Destination SSID ( 99 )

String lastMsgRX = " ";                   // Last received message
String payloadS = "";                     // LoRa message App layer payload
String lastMsgTX = " ";                   // Last transmitted 

// Create Servers
AsyncWebServer server(WEB_PORT);
WebSocketsServer webSocket(WS_PORT);


//
// FUNCTIONS
//

/// Split String

String getSubString(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/// Core 1 Task (Web, WebSox, WiFi)
void HasWebsox(void *pvParameters) {
  Serial.print("800: Task *HasWebSox* on core ");
  Serial.println(xPortGetCoreID());
  while (true) {
    webSocket.loop();
  }
}

/// Display myOLEDmsg on OLED
void OLEDme(String myOLEDmsg)
{
  myOLEDmsg = myOLEDmsg.substring(0,20);
  #ifdef HELTEC_WIFI_LORA_32_V2
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 15, myOLEDmsg);
  Heltec.display->display();
  #endif
  //delay(oledTIME);
  //Heltec.display->clear();
}

/// Core 0 Task (LoRa)
void HasTRX(void *pvParameters) {
  while (true) {
    MyRX_Reset = false;
    // Initialize LoRa
    LoRa.setSyncWord(0xFF);                 // Set for LoRa Broadcast
    LoRa.disableCrc();
    LoRa.setFrequency(frequency);
    LoRa.setTxPower(txpwr,RF_PACONFIG_PASELECT_PABOOST);
    if (modemconfig == "Bw125Cr45Sf128") {
        LoRa.setSignalBandwidth(125000);
        LoRa.setSpreadingFactor(7);
        LoRa.setCodingRate4(8);
      }
      else if (modemconfig == "Bw500Cr45Sf128") {
        LoRa.setSignalBandwidth(500000);
        LoRa.setSpreadingFactor(7);
        LoRa.setCodingRate4(5);
      }
      else if (modemconfig == "Bw31_25Cr48Sf512") {
        LoRa.setSignalBandwidth(31250);
        LoRa.setSpreadingFactor(7);
        LoRa.setCodingRate4(8);
      }
      else if (modemconfig ==  "Bw125Cr48Sf4096") {
        LoRa.setSignalBandwidth(125000);
        LoRa.setSpreadingFactor(12);
        LoRa.setCodingRate4(8);
      }
      else if (modemconfig ==  "Bw125Cr45Sf2048") {
        LoRa.setSignalBandwidth(125000);
        LoRa.setSpreadingFactor(8);
        LoRa.setCodingRate4(5);
      }
      else {
        LoRa.setSignalBandwidth(125000);
        LoRa.setSpreadingFactor(7);
        LoRa.setCodingRate4(8);
    }
    LoRa.receive();
    Serial.print("CPU("); 
    Serial.print(xPortGetCoreID());
    Serial.println("): Task (re)start - HasTRX (LoRa)"); 
    while (!MyRX_Reset) {
      delay(5);
      // try to parse packet
      int packetSize = LoRa.parsePacket();
      if (packetSize) {
        lastMsgRX = "";
        for (int i = 0; i < packetSize; i++)
                  lastMsgRX = lastMsgRX + ((char)LoRa.read());
        lastMsgRX = "RX:" + lastMsgRX + "|RSSI: " + String(LoRa.packetRssi());
        Serial.println(lastMsgRX);
        webSocket.broadcastTXT(lastMsgRX);
      }
    }
  }
}

/// TX LoRa
void sendLORA(String outgoing)
{
  LoRa.beginPacket();                     // start packet
  LoRa.write(destinationLORA);            // add destination address
  //LoRa.write(localaddressLORA);         // add sender address
  //LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                   // add payload
  LoRa.endPacket();                       // finish packet and send it
  #ifdef HAS_OLED
  OLEDme(outgoing);
  #endif
  Serial.print("TX:");
  Serial.println(outgoing);
  lastMsgTX = outgoing;                   // Record last message sent
  MyRX_Reset = true;
}

/// RX LoRa
void onReceiveLORA(int packetSize)
{
  // read packet
  lastMsgRX = "";
  for (int i = 0; i < packetSize; i++)
  {
    lastMsgRX = lastMsgRX + ((char)LoRa.read());
  }
  lastMsgRX = "RX:" + lastMsgRX + "|RSSI: " + String(LoRa.packetRssi());
  Serial.print("RX:");
  Serial.println(lastMsgRX);
  #ifdef HAS_OLED
  OLEDme(lastMsgRX);
  #endif
  webSocket.broadcastTXT(lastMsgRX);
}

/// Core 1 Task (Beacon LoRa)
void HasBeacon(void *pvParameters) {
  // read channel info then set
  delay(8000);
  webSocket.broadcastTXT("BEACON:" + lastMsgTX);
  sendLORA(lastMsgTX);
}

/// Display Logo on OLED
void logo()
{
  #ifdef HELTEC_WIFI_LORA_32_V2
  Heltec.display->clear();
  Heltec.display->drawXbm(0,5,hvdnimg_width,hvdnimg_height,hvdnimg_bits);
  Heltec.display->display();
  delay(1500);
  Heltec.display->clear();
  #endif
}

/// WebServer
void onFavRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/favicon.ico", "text/html");
}

void onIndexRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/index.html", "text/html");
}

void onCssRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/index.css", "text/css");
}

void onJsRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/index.js", "text/javascript");
}

void onJsonRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/config.json", "application/json");
}

/// WebSockets
bool isEqual(uint8_t * payload, char * message) {
    return strcmp((char *)payload, message) == 0;
}

void onWebSocketEvent(uint8_t clientID, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
      String payloadS = (char*)payload;
      
      //RADIO
      if (payloadS == "SET:RADIO:RESET") {
        // Reset Radio - HASTRX task
        webSocket.sendTXT(clientID, "ACK:RADIO:RESET");
        MyRX_Reset = true;
      }
      
      // DUMP LORA
      if (payloadS == "GET:LORA") {
        // dump lora config
        LoRa.dumpRegisters(Serial);
        webSocket.sendTXT(clientID, "ACK:GET:LORA");
      }
      
      //BEACON
      if (payloadS == "SET:BEACON:ON") {
        // turn me on
        webSocket.sendTXT(clientID, "ACK:BEACON:ON");
        //vTaskResume(TaskBeacon);               // Add a beacon vtask to Core 1
      } else if (payloadS == "SET:BEACON:OFF") {
        // turn me off
        webSocket.sendTXT(clientID, "ACK:BEACON:OFF");
        //vTaskSuspend(TaskBeacon);              // Kill beacon vtask to Core 1
      }
      
      //TXPWR
      if (payloadS == "SET:RADIO:PWR:LOW") {
        // set me low
        txpwr = 7;
        webSocket.sendTXT(clientID, "ACK:RADIO:PWR:LOW");
      } else if (payloadS == "SET:RADIO:PWR:MEDIUM") {
        // set me medium
        txpwr = 14;
        webSocket.sendTXT(clientID, "ACK:RADIO:PWR:MEDIUM");
      } else if (payloadS == "SET:RADIO:PWR:HIGH") {
        // set me high
        txpwr = 21;
        webSocket.sendTXT(clientID, "ACK:RADIO:PWR:HIGH");
      }
      
      //CHANNEL NEW  20221228-0300 this needs to be changed
      if (payloadS.indexOf("SET:TUNER:") >= 0) {
        webSocket.sendTXT(clientID, "ACK:SET:TUNER:");
        String localjunk = getSubString(payloadS,':',0);
        String localjunk2 = getSubString(payloadS,':',1);
        String localmodemconfig = getSubString(payloadS,':',2);
        String localfrequency = getSubString(payloadS,':',3);
        modemconfig = localmodemconfig;
        frequency = localfrequency.toInt();
        MyRX_Reset = true;
      }

      //CHANNEL ORG
      //if (payloadS.indexOf("SET:TUNER:") >= 0) {
      //  webSocket.sendTXT(clientID, "ACK:SET:TUNER:");
      //  String localjunk = getSubString(payloadS,':',0);
      //  String localjunk2 = getSubString(payloadS,':',1);
      //  String localfrequency = getSubString(payloadS,':',2);
      //  String localspreadfactor = getSubString(payloadS,':',3);
      //  String localcodingrate4 = getSubString(payloadS,':',4);
      //  String localbandwidth = getSubString(payloadS,':',5);   
      // frequency = localfrequency.toInt();
      //  spreadfactor = localspreadfactor.toInt();
      // codingrate4 = localcodingrate4.toInt();
      //  bandwidth = localbandwidth.toInt();
      //  MyRX_Reset = true;
      //}
      
      //TX
      if (payloadS.indexOf("TX:") >= 0) {
        // read channel info then set
        webSocket.sendTXT(clientID, "ACK:" + payloadS);
        payloadS.replace("TX:","");
        sendLORA(payloadS);
      }
    }
}

/// SETUP FUNCTIONS
void initSerial() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();
  Serial.println();
}

void initSPIFFS() {
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println(" ERR: SPIFFS mounting");
    return;
  }
  Serial.println(" 100: SPIFFs initialized");
}

void loadJsonFile() {
  File configFile = SPIFFS.open("/config.json", "r");
  JsonDocument doc;
  // The filter: it contains "true" for each value we want to keep
  //JsonDocument filter;
  //filter["CURRENT"] = true;
  if(!configFile){
    Serial.println("Failed to open config.json for reading");
    return;
  } else {
    //DeserializationError error = deserializeJson(doc, configFile, DeserializationOption::Filter(filter));
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
      Serial.print("Error parsing config.json [");
      Serial.print(error.c_str());
      Serial.println("]");
    }
    // Place JSON into Variables
    channel	= (const char*)doc["RADIO"]["channel"];
		rfmodule = (const char*)doc["RADIO"]["rfmodule"];
		modemconfig = (const char*)doc["RADIO"]["modemconfig"];
		modem = int(doc["RADIO"]["modem"]);
    frequency = int(doc["RADIO"]["frequency"]);
    spreadfactor = int(doc["RADIO"]["spreadfactor"]);
    codingrate4 = int(doc["RADIO"]["codingrate4"]);
    bandwidth = int(doc["RADIO"]["bandwidth"]);
    txpwr = int(doc["RADIO"]["txpwr"]);
    mycall = (const char*)doc["CONTACT"]["mycall"];
    myssid = (const char*)doc["CONTACT"]["mySSID"];
    mybeacon = (const char*)doc["CONTACT"]["mybeacon"];
    dstcall = (const char*)doc["CONTACT"]["dstcall"];
    dstssid = (const char*)doc["CONTACT"]["dstssid"];
    configFile.close();
  }
  Serial.println(" 200: JSON loaded");
}

void initWiFi() {
  //#ifdef WIFI_SSID
  //WiFi.mode(WIFI_STA);
  //WiFi.begin(WIFI_SSID, WIFI_KEY);
  //while(WiFi.status() != WL_CONNECTED) {
  //  delay(1000);
  //}
  //Serial.println(" 300: WiFi CL initialized");
  //Serial.println("310: WiFi IP  " + WiFi.localIP());
  //Serial.println("320: WiFi CL initialized");
  //#endif

  //if (WiFi.status() != WL_CONNECTED) 
  //{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_APSSID, WIFI_APKEY);
    IPAddress ip = WiFi.softAPIP();
    Serial.println(" 300: WiFi AP initialized");
    //Serial.println(" 310: WiFi IP  " + ip);
    //Serial.println(" 320: WiFi AP initialized");
  //}
}

void initWebServer() {
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/favicon.ico", HTTP_GET, onFavRequest);
  server.on("/index.css", HTTP_GET, onCssRequest);
  server.on("/index.js", HTTP_GET, onJsRequest);
  server.on("/config.json", HTTP_GET, onJsonRequest);
  server.begin();
  Serial.println(" 400: Webserver initialized");
}

void initWebSockets() {
  // Websocket Handler
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.println(" 500: Websockets initialized");
}

void initOLED() {
  // Start OLED 
  #ifdef HELTEC_WIFI_LORA_32_V2
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10); Serial.println(" 600: OLED Initialized");
   #endif
}

//
// SETUP
//

void setup() {
  #ifdef HELTEC_WIFI_LORA_32_V2 
  Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  #endif
  initSerial();
  Serial.println("INIT: ESP32");
  Serial.println("=====================================");
  Serial.print(" 000: Setup running on core ");
  Serial.println(xPortGetCoreID());
  initSPIFFS();
  loadJsonFile();
  initWiFi();
  initWebServer();
  initWebSockets();
  #ifdef HAS_OLED
  initOLED();
  logo();
  #endif
  Serial.println("INIT: COMPLETE");
  Serial.println("=====================================");
  Serial.println();
  
  // Create Task assignments
  //
  // HasTRX on Core 0
  // hasWebsox on Core 1 butno need to assign since Loop on that core by default
  //
  xTaskCreatePinnedToCore(HasTRX, "HasTRX", 10000, NULL, 1, &TaskTRX, 0);
  delay(500);
  //
  // HasBeacon on Core 1
  //xTaskCreatePinnedToCore(HasBeacon, "HasBeacon", 10000, NULL, 1, &TaskBeacon, 1);
  //
  //vTaskSuspend(TaskBeacon);
  //delay(500);
  //
  //  Websox on Core 1
  //
  //xTaskCreatePinnedToCore(HasWebsox, "HasWebsox", 10000, NULL, 1, &TaskWebsox, 1);
  //delay(500);

  Serial.print("CPU("); 
  Serial.print(xPortGetCoreID());
  Serial.println("): Task (re)start - WiFi, Web, WebSox"); 
}

//
// MAIN
//

void loop(){
  Serial.println("  LOOP: Started");
  while (true) 
  {
    webSocket.loop();
  }
}
