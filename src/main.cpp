//
//  HASviolet-ESP32
//
//
//  20210320-1700
//     
//


// Chosen board
//
// The board is selected by a -D<BOARD> build flag in platformio.ini (see the
// [env:...] sections there), NOT hardcoded here, so the same main.cpp builds
// for every supported board just by picking a different PIO environment.
#if !defined(WIFI_LORA_32) && !defined(WIFI_LORA_32_V2) && !defined(WIFI_LORA_32_V3) \
 && !defined(TTGO_LORA32_V1) && !defined(TTGO_LORA32_V2) && !defined(TTGO_LORA32_V21) && !defined(TTGO_TBEAM)
#error "No board selected. Build using one of the environments in platformio.ini (e.g. pio run -e heltec_wifi_lora_32_V2)."
#endif

// True for any Heltec WiFi LoRa 32 board (V1/V2/V3) -- these are driven by the
// Heltec library (Heltec.begin()/Heltec.display/Heltec.LoRa) instead of
// talking to the radio and OLED directly. WIFI_LORA_32(_V2/_V3) are the
// Heltec library's OWN board-select macros (see heltec.h) -- they have to be
// set exactly as it expects, not renamed to our own convention.
#if defined(WIFI_LORA_32) || defined(WIFI_LORA_32_V2) || defined(WIFI_LORA_32_V3)
#define HASV_HELTEC_BOARD
#endif

//
// LIBRARIES
//

#include "Arduino.h"
#include "TimeLib.h"
#include "HASviolet_config.h"
#ifdef HASV_HELTEC_BOARD
#include "heltec.h"
#else
#include <SPI.h>
#include <LoRa.h>
#ifdef HAS_OLED
#include "SSD1306Wire.h"
#endif
#ifdef HAS_AXP192
#include <axp20x.h>
#endif
#endif
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "FS.h"
#include "HVDN_logo.h"


//
// DEFINES
//
#define WIFI_POLL_DELAY 500
#define WIFI_POLL_TRIES 20
#define BAND 911250000                    // you can set band here directly, ( 868E6,915E6 )

// OLED handle. On Heltec boards Heltec.begin() already creates and owns
// Heltec.display (an SSD1306Wire*); on every other board we create our own
// SSD1306Wire instance from the board's OLED_SDA/OLED_SCL pins. Everything
// below the init functions just talks to "oledDisplay" either way.
#ifdef HAS_OLED
#ifdef HASV_HELTEC_BOARD
#define oledDisplay Heltec.display
#else
SSD1306Wire genericOLED(0x3c, OLED_SDA, OLED_SCL);
#define oledDisplay (&genericOLED)
#endif
#endif

// AXP192 power-management IC (e.g. TTGO T-Beam v1.1). It gates the 3.3V
// rails the LoRa radio and GPS run on, so it has to be enabled before those
// peripherals will respond to anything.
#ifdef HAS_AXP192
AXP20X_Class PMU;
#endif

// LoRa radio handle. The Heltec library owns its own LoRaClass instance as a
// member (Heltec.LoRa) rather than the bare global "LoRa" object that
// standalone LoRa libraries (and the Heltec library's own internals,
// confusingly, expose under the same name) provide -- so on Heltec boards
// the bare "LoRa" symbol in scope here is a *different*, never-initialized
// object. hvLoRa always points at whichever one Heltec.begin()/initLoRaRadio()
// actually set up.
#ifdef HASV_HELTEC_BOARD
#define hvLoRa Heltec.LoRa
#else
#define hvLoRa LoRa
#endif

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

// Default settings before HASviolet.json load
String channel = "HV1";                   // Channel
String rfmodule = "RFM9X";                // RF Module
String modemconfig = "Bw125Cr45Sf128";    // Modemstring Radiohead
int modem = 0;
int frequency = 911250000;                // HASviolet default settings for safe LoRa module init
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
  #ifdef HAS_OLED
  oledDisplay->clear();
  oledDisplay->setTextAlignment(TEXT_ALIGN_LEFT);
  oledDisplay->setFont(ArialMT_Plain_10);
  oledDisplay->drawString(0, 15, myOLEDmsg);
  oledDisplay->display();
  #endif
  //delay(oledTIME);
  //oledDisplay->clear();
}

/// Core 0 Task (LoRa)
void HasTRX(void *pvParameters) {
  while (true) {
    MyRX_Reset = false;
    // Initialize LoRa
    hvLoRa.setSyncWord(0xFF);                 // Set for LoRa Broadcast
    hvLoRa.disableCrc();
    hvLoRa.setFrequency(frequency);
    hvLoRa.setTxPower(txpwr,RF_PACONFIG_PASELECT_PABOOST);
    if (modemconfig == "Bw125Cr45Sf128") {
        hvLoRa.setSignalBandwidth(125000);
        hvLoRa.setSpreadingFactor(7);
        hvLoRa.setCodingRate4(8);
      }
      else if (modemconfig == "Bw500Cr45Sf128") {
        hvLoRa.setSignalBandwidth(500000);
        hvLoRa.setSpreadingFactor(7);
        hvLoRa.setCodingRate4(5);
      }
      else if (modemconfig == "Bw31_25Cr48Sf512") {
        hvLoRa.setSignalBandwidth(31250);
        hvLoRa.setSpreadingFactor(7);
        hvLoRa.setCodingRate4(8);
      }
      else if (modemconfig ==  "Bw125Cr48Sf4096") {
        hvLoRa.setSignalBandwidth(125000);
        hvLoRa.setSpreadingFactor(12);
        hvLoRa.setCodingRate4(8);
      }
      else if (modemconfig ==  "Bw125Cr45Sf2048") {
        hvLoRa.setSignalBandwidth(125000);
        hvLoRa.setSpreadingFactor(8);
        hvLoRa.setCodingRate4(5);
      }
      else {
        hvLoRa.setSignalBandwidth(125000);
        hvLoRa.setSpreadingFactor(7);
        hvLoRa.setCodingRate4(8);
    }
    hvLoRa.receive();
    Serial.print("CPU("); 
    Serial.print(xPortGetCoreID());
    Serial.println("): Task (re)start - HasTRX (LoRa)"); 
    while (!MyRX_Reset) {
      delay(5);
      // try to parse packet
      int packetSize = hvLoRa.parsePacket();
      if (packetSize) {
        lastMsgRX = "";
        for (int i = 0; i < packetSize; i++)
                  lastMsgRX = lastMsgRX + ((char)hvLoRa.read());
        lastMsgRX = "RX:" + lastMsgRX + "|RSSI: " + String(hvLoRa.packetRssi());
        Serial.println(lastMsgRX);
        webSocket.broadcastTXT(lastMsgRX);
      }
    }
  }
}

/// TX LoRa
void sendLORA(String outgoing)
{
  hvLoRa.beginPacket();                     // start packet
  hvLoRa.write(destinationLORA);            // add destination address
  //hvLoRa.write(localaddressLORA);         // add sender address
  //hvLoRa.write(outgoing.length());        // add payload length
  hvLoRa.print(outgoing);                   // add payload
  hvLoRa.endPacket();                       // finish packet and send it
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
    lastMsgRX = lastMsgRX + ((char)hvLoRa.read());
  }
  lastMsgRX = "RX:" + lastMsgRX + "|RSSI: " + String(hvLoRa.packetRssi());
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
  #ifdef HAS_OLED
  oledDisplay->clear();
  oledDisplay->drawXbm(0,5,hvdnimg_width,hvdnimg_height,hvdnimg_bits);
  oledDisplay->display();
  delay(1500);
  oledDisplay->clear();
  #endif
}

/// WebServer
void onFavRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/favicon.ico", "text/html");
}

void onIndexRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET_INDEX.html", "text/html");
}

void onCssRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET.css", "text/css");
}

void onJsRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET.js", "text/javascript");
}

void onJsonRequest(AsyncWebServerRequest* request) {
  request->send(SPIFFS, "/hasVIOLET.json", "application/json");
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
        hvLoRa.dumpRegisters(Serial);
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
      if (payloadS == "SET:TXPWR:LOW") {
        // set me low
        txpwr = 7;
        webSocket.sendTXT(clientID, "ACK:TXPWR:LOW");
      } else if (payloadS == "SET:TXPWR:MEDIUM") {
        // set me medium
        txpwr = 14;
        webSocket.sendTXT(clientID, "ACK:TXPWR:MEDIUM");
      } else if (payloadS == "SET:TXPWR:HIGH") {
        // set me high
        txpwr = 21;
        webSocket.sendTXT(clientID, "ACK:TXPWR:HIGH");
      }
      
      //CHANNEL NEW
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
  File configFile = SPIFFS.open("/hasVIOLET.json", "r");
  DynamicJsonDocument doc(1024);
  // The filter: it contains "true" for each value we want to keep
  StaticJsonDocument<500> filter;
  filter["CURRENT"] = true;
  if(!configFile){
    Serial.println("Failed to open hasVIOLET.json for reading");
    return;
  } else {
    DeserializationError error = deserializeJson(doc, configFile, DeserializationOption::Filter(filter));
    if (error) {
      Serial.print("Error parsing hasVIOLET.json [");
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
  #ifdef WIFI_SSID
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_KEY);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  Serial.println(" 300: WiFi CL initialized");
  //Serial.println("310: WiFi IP  " + WiFi.localIP());
  //Serial.println("320: WiFi CL initialized");
  #endif

  if (WiFi.status() != WL_CONNECTED) 
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_APSSID, WIFI_APKEY);
    IPAddress ip = WiFi.softAPIP();
    Serial.println(" 300: WiFi AP initialized");
    //Serial.println(" 310: WiFi IP  " + ip);
    //Serial.println(" 320: WiFi AP initialized");
  }
  
}

void initWebServer() {
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/favicon.ico", HTTP_GET, onFavRequest);
  server.on("/hasVIOLET.css", HTTP_GET, onCssRequest);
  server.on("/hasVIOLET.js", HTTP_GET, onJsRequest);
  server.on("/hasVIOLET.json", HTTP_GET, onJsonRequest);
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
  #ifdef HAS_OLED
  #ifdef HASV_HELTEC_BOARD
  oledDisplay->init();
  #else
  Wire.begin(OLED_SDA, OLED_SCL);
  oledDisplay->init();
  #endif
  oledDisplay->flipScreenVertically();
  oledDisplay->setFont(ArialMT_Plain_10);
  Serial.println(" 600: OLED Initialized");
  #endif
}

#ifdef HAS_AXP192
void initPMU() {
  // T-Beam (and similar boards): the LoRa radio and GPS run off 3.3V rails
  // gated by the AXP192 PMU. Nothing downstream of this will respond until
  // it is powered on.
  Wire.begin();
  if (!PMU.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    PMU.setPowerOutPut(AXP192_LDO2, AXP202_ON);   // LoRa radio VDD
    PMU.setPowerOutPut(AXP192_LDO3, AXP202_ON);   // GPS VDD
    PMU.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
    PMU.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
    PMU.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
    Serial.println(" 050: AXP192 PMU initialized");
  } else {
    Serial.println(" ERR: AXP192 PMU init failed - LoRa/GPS may be unpowered");
  }
}
#endif

#ifndef HASV_HELTEC_BOARD
void initLoRaRadio() {
  // Heltec boards get this for free from Heltec.begin(); everyone else
  // wires the SX127x up by hand from the board's variant pin definitions.
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  hvLoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  if (!hvLoRa.begin(BAND)) {
    Serial.println(" ERR: LoRa radio init failed");
  }
}
#endif

//
// SETUP
//

void setup() {
  #ifdef HAS_AXP192
  initPMU();
  #endif
  #ifdef HASV_HELTEC_BOARD
  Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  #endif
  initSerial();
  Serial.println("INIT: HASviolet ESP32");
  Serial.println("=====================================");
  Serial.print(" 000: Setup running on core ");
  Serial.println(xPortGetCoreID());
  initSPIFFS();
  loadJsonFile();
  initWiFi();
  initWebServer();
  initWebSockets();
  #ifndef HASV_HELTEC_BOARD
  initLoRaRadio();
  #endif
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
