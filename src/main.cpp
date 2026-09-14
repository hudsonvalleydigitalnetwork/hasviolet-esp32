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
 && !defined(WIRELESS_STICK) && !defined(WIRELESS_STICK_LITE) \
 && !defined(TTGO_LORA32_V1) && !defined(TTGO_LORA32_V2) && !defined(TTGO_LORA32_V21) && !defined(TTGO_TBEAM) \
 && !defined(LILYGO_T3_S3) && !defined(BQ_STATION_G2) && !defined(TBEAM_SUPREME) && !defined(HELTEC_WIRELESS_TRACKER)
#error "No board selected. Build using one of the environments in platformio.ini (e.g. pio run -e heltec_wifi_lora_32_V2)."
#endif

// True for any Heltec board (WiFi LoRa 32 V1/V2/V3, Wireless Stick, Wireless
// Stick Lite) -- these use the Heltec library (Heltec.begin()/
// Heltec.display) for display/Vext/serial bring-up only now. These macro
// names are the Heltec library's OWN board-select macros (see heltec.h) --
// they have to be set exactly as it expects, not renamed to our own
// convention.
#if defined(WIFI_LORA_32) || defined(WIFI_LORA_32_V2) || defined(WIFI_LORA_32_V3) \
 || defined(WIRELESS_STICK) || defined(WIRELESS_STICK_LITE)
#define HASV_HELTEC_BOARD
#endif

// Chip family -- the ONLY axis that decides which lib/HasRadio subclass
// drives this board's radio (see hvRadio below). Every board's radio object
// is a HasRadio subclass talking to RadioLib directly, regardless of
// whether it's also a Heltec board for display purposes: WIFI_LORA_32_V3
// belongs here despite being HASV_HELTEC_BOARD above, because its actual
// chip is an SX1262, not the SX1276 every other Heltec board here has.
// Confirmed on real hardware -- Heltec's own bundled LoRa driver hangs the
// CPU forever (an unconditional while(1) in heltec.cpp) trying to detect a
// chip it doesn't recognize on V3.
#if defined(LILYGO_T3_S3) || defined(BQ_STATION_G2) || defined(TBEAM_SUPREME) || defined(HELTEC_WIRELESS_TRACKER) || defined(WIFI_LORA_32_V3)
#define HASV_SX126X_BOARD
#endif

// True for boards whose OLED is an Adafruit_SH110X-family controller
// (SH1107 or SH1106) rather than the SSD1306 the rest of this file assumes.
// Same idea as HASV_SX126X_BOARD: a small adapter (see below) speaks the
// OLEDme()/logo() calls this file already makes, so nothing downstream
// needs to know which controller it's actually talking to.
#if defined(BQ_STATION_G2)
#define HASV_SH1107_BOARD
#define HASV_SH110X_BOARD
#endif
#if defined(TBEAM_SUPREME)
#define HASV_SH1106_BOARD
#define HASV_SH110X_BOARD
#endif

// Heltec Wireless Tracker has no OLED at all -- its status display is a
// color ST7735 TFT on its own dedicated SPI bus (separate from the LoRa
// radio's SPI pins). Same adapter idea again, applied to a completely
// different kind of panel this time.
#if defined(HELTEC_WIRELESS_TRACKER)
#define HASV_ST7735_BOARD
#endif

//
// LIBRARIES
//

#include "Arduino.h"
#include "TimeLib.h"
#include "HASviolet_config.h"
// Heltec boards use heltec.h for display/Vext/serial bring-up only now --
// the radio is always a lib/HasRadio subclass talking to RadioLib directly
// (see HASV_SX126X_BOARD above and hvRadio below), on every board.
#ifdef HASV_HELTEC_BOARD
#include "heltec.h"
#endif
#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#ifdef HASV_SX126X_BOARD
#include <HasRadioSX126x.h>
#else
#include <HasRadioSX127x.h>
#endif
#ifdef HAS_OLED
#ifdef HASV_SH110X_BOARD
#include <Adafruit_SH110X.h>
#elif defined(HASV_ST7735_BOARD)
#include <Adafruit_ST7735.h>
#elif !defined(HASV_HELTEC_BOARD)
#include "SSD1306Wire.h"
#endif
#endif
#ifdef HAS_AXP192
#include <axp20x.h>
#endif
#ifdef HAS_AXP2101
#include <XPowersLib.h>
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

#if defined(HASV_SH110X_BOARD) || defined(HASV_ST7735_BOARD)
// OLEDme()/logo() (below) call oledDisplay->setTextAlignment(TEXT_ALIGN_LEFT)
// and ->setFont(ArialMT_Plain_10) unconditionally -- those two symbols
// normally come from the ThingPulse SSD1306Wire library, which these boards
// don't use. Since every adapter here always left-aligns and only ever has
// the one built-in font anyway, they're just stand-ins to satisfy the call
// sites rather than a real SSD1306Wire include pulled in for two constants.
#define TEXT_ALIGN_LEFT 0
static const uint8_t ArialMT_Plain_10[] = {0};
#endif

#ifdef HASV_SH110X_BOARD
// Adapter that makes an Adafruit_SH110X-family display (SH1107 on BQ
// Station G2, SH1106 on T-Beam Supreme) answer to the same handful of
// calls OLEDme()/logo()/initOLED() already make against a ThingPulse
// SSD1306Wire -- same idea as lib/HasRadio's HasRadio subclasses, applied to
// the display instead of the radio. Templated on the concrete Adafruit_SH110X subclass
// since Adafruit_SH1107/Adafruit_SH1106G share an identical constructor and
// drawing API.
template <typename SH110xDisplay>
class SH110XOLEDAdapter {
public:
  SH110XOLEDAdapter(int w, int h, TwoWire *wire, int rstPin)
    : dev(w, h, wire, rstPin) {}

  void init() { dev.begin(0x3C, true); }
  void flipScreenVertically() { dev.setRotation(2); }
  void setFont(const uint8_t *) {}     // only ever asked for one font; ignored
  void setTextAlignment(int) {}        // only ever asked to left-align; ignored
  void clear() { dev.clearDisplay(); }
  void drawString(int x, int y, const String &text) {
    dev.setCursor(x, y);
    dev.setTextColor(SH110X_WHITE);
    dev.print(text);
  }
  void display() { dev.display(); }
  void drawXbm(int x, int y, int w, int h, const uint8_t *bits) {
    dev.drawXBitmap(x, y, bits, w, h, SH110X_WHITE);
  }

private:
  SH110xDisplay dev;
};
#endif

#ifdef HASV_ST7735_BOARD
// Adapter for Heltec Wireless Tracker's ST7735 color TFT -- on its own SPI
// bus, separate from the LoRa radio's. Unlike the OLEDs above, an ST7735
// pushes each drawing call straight to the panel (no separate frame buffer
// to flush), so display() is a no-op here.
//
// NOTE: panel geometry (160x80, INITR_MINI160x80, landscape rotation) is
// inferred from Meshtastic's own TFT_WIDTH/HEIGHT/OFFSET_X defines for this
// exact board, not confirmed against real hardware -- see project_state.md.
class ST7735TFTAdapter {
public:
  ST7735TFTAdapter(SPIClass *spi, int cs, int dc, int rst)
    : dev(spi, cs, dc, rst) {}

  void init() {
    #ifdef VEXT_ENABLE
    // Powers the TFT (and GPS) rail on this board; nothing on the panel
    // responds until this is driven high.
    pinMode(VEXT_ENABLE, OUTPUT);
    digitalWrite(VEXT_ENABLE, VEXT_ON_VALUE);
    delay(10);
    #endif
    dev.initR(INITR_MINI160x80);
    dev.setRotation(1);
  }
  void flipScreenVertically() {}        // handled by setRotation() above; no-op here
  void setFont(const uint8_t *) {}      // only ever asked for one font; ignored
  void setTextAlignment(int) {}         // only ever asked to left-align; ignored
  void clear() { dev.fillScreen(ST77XX_BLACK); }
  void drawString(int x, int y, const String &text) {
    dev.setCursor(x, y);
    dev.setTextColor(ST77XX_WHITE);
    dev.print(text);
  }
  void display() {}                     // draws straight to the panel; nothing to flush
  void drawXbm(int x, int y, int w, int h, const uint8_t *bits) {
    dev.drawXBitmap(x, y, bits, w, h, ST77XX_WHITE);
  }

private:
  Adafruit_ST7735 dev;
};
#endif

// OLED handle. On Heltec boards Heltec.begin() already creates and owns
// Heltec.display (an SSD1306Wire*); on SH110X/ST7735 boards we wrap one in
// the adapters above; everywhere else we create our own SSD1306Wire
// instance from the board's OLED_SDA/OLED_SCL pins. Everything below the
// init functions just talks to "oledDisplay" either way.
#ifdef HAS_OLED
#ifdef HASV_HELTEC_BOARD
#define oledDisplay Heltec.display
#elif defined(HASV_SH1107_BOARD)
SH110XOLEDAdapter<Adafruit_SH1107> genericSH110xOLED(128, 64, &Wire, -1);
#define oledDisplay (&genericSH110xOLED)
#elif defined(HASV_SH1106_BOARD)
SH110XOLEDAdapter<Adafruit_SH1106G> genericSH110xOLED(128, 64, &Wire, -1);
#define oledDisplay (&genericSH110xOLED)
#elif defined(HASV_ST7735_BOARD)
SPIClass tftSPI(HSPI);
ST7735TFTAdapter genericTFT(&tftSPI, ST7735_CS, ST7735_RS, ST7735_RESET);
#define oledDisplay (&genericTFT)
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

// AXP2101 power-management IC (e.g. T-Beam Supreme) -- same idea as AXP192
// above but a different chip and library (XPowersLib), and on T-Beam
// Supreme specifically it lives on the ESP32's second I2C bus (Wire1)
// shared with the onboard PCF8563 RTC, not the main Wire bus. XPowersLib's
// concrete XPowersAXP2101 class keeps setPowerChannelVoltage()/
// enablePowerOutput() protected -- they're only public on the
// XPowersLibInterface base, which is why PMU is that interface type
// (matching how Meshtastic's own Power.cpp uses this library) rather than
// an XPowersAXP2101 value.
#ifdef HAS_AXP2101
#ifdef PMU_USE_WIRE1
XPowersLibInterface *PMU = new XPowersAXP2101(Wire1);
#else
XPowersLibInterface *PMU = new XPowersAXP2101(Wire);
#endif
#endif

// Radio handle. Every board's radio -- SX1276/77 or SX1262/68 alike -- is
// one of lib/HasRadio's two HasRadio subclasses now, talking to RadioLib
// directly; HasTRX/sendLORA/onWebSocketEvent all go through the shared
// HasRadio interface via "hvRadio" rather than through a per-chip adapter
// shaped like the old Arduino LoRaClass API. This used to be a Heltec-vs-
// generic distinction too (Heltec boards used Heltec.LoRa); now it's purely
// the HASV_SX126X_BOARD chip-family axis -- Heltec boards' radios go through
// HasRadioSX127x (or HasRadioSX126x for V3) exactly like every other board.
#ifdef HASV_SX126X_BOARD
HasRadioSX126x hvRadioImpl(LORA_CS, SX126X_DIO1, LORA_RST, SX126X_BUSY);
#else
HasRadioSX127x hvRadioImpl(LORA_CS, LORA_IRQ, LORA_RST);
#endif
HasRadio &hvRadio = hvRadioImpl;

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
    hvRadio.setSyncWord(0xFF);                 // Set for LoRa Broadcast
    hvRadio.setCrc(false);
    hvRadio.setFrequency(frequency);
    hvRadio.setTxPower(txpwr);
    if (modemconfig == "Bw125Cr45Sf128") {
        hvRadio.setBandwidth(125000);
        hvRadio.setSpreadingFactor(7);
        hvRadio.setCodingRate(8);
      }
      else if (modemconfig == "Bw500Cr45Sf128") {
        hvRadio.setBandwidth(500000);
        hvRadio.setSpreadingFactor(7);
        hvRadio.setCodingRate(5);
      }
      else if (modemconfig == "Bw31_25Cr48Sf512") {
        hvRadio.setBandwidth(31250);
        hvRadio.setSpreadingFactor(7);
        hvRadio.setCodingRate(8);
      }
      else if (modemconfig ==  "Bw125Cr48Sf4096") {
        hvRadio.setBandwidth(125000);
        hvRadio.setSpreadingFactor(12);
        hvRadio.setCodingRate(8);
      }
      else if (modemconfig ==  "Bw125Cr45Sf2048") {
        hvRadio.setBandwidth(125000);
        hvRadio.setSpreadingFactor(8);
        hvRadio.setCodingRate(5);
      }
      else {
        hvRadio.setBandwidth(125000);
        hvRadio.setSpreadingFactor(7);
        hvRadio.setCodingRate(8);
    }
    hvRadio.startReceive();
    Serial.print("CPU(");
    Serial.print(xPortGetCoreID());
    Serial.println("): Task (re)start - HasTRX (LoRa)");
    while (!MyRX_Reset) {
      delay(5);
      // try to receive a packet
      uint8_t rxBuf[256];
      int packetSize = hvRadio.recvRaw(rxBuf, sizeof(rxBuf));
      if (packetSize) {
        lastMsgRX = "";
        for (int i = 0; i < packetSize; i++)
                  lastMsgRX = lastMsgRX + ((char)rxBuf[i]);
        lastMsgRX = "RX:" + lastMsgRX + "|RSSI: " + String(hvRadio.getLastRSSI());
        Serial.println(lastMsgRX);
        webSocket.broadcastTXT(lastMsgRX);
      }
    }
  }
}

/// TX LoRa
void sendLORA(String outgoing)
{
  uint8_t txBuf[256];
  size_t txLen = 0;
  txBuf[txLen++] = destinationLORA;         // add destination address
  //txBuf[txLen++] = localaddressLORA;      // add sender address
  //txBuf[txLen++] = outgoing.length();     // add payload length
  for (size_t i = 0; i < outgoing.length() && txLen < sizeof(txBuf); i++)
    txBuf[txLen++] = (uint8_t)outgoing[i];  // add payload
  hvRadio.startSendRaw(txBuf, txLen);       // send it (blocks until on air)
  while (!hvRadio.isSendComplete()) { }
  #ifdef HAS_OLED
  OLEDme(outgoing);
  #endif
  Serial.print("TX:");
  Serial.println(outgoing);
  lastMsgTX = outgoing;                   // Record last message sent
  MyRX_Reset = true;
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
        hvRadio.dumpRegisters(Serial);
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
  // Pre-existing bug, unrelated to this HAL work: this loop had no bound at
  // all -- WIFI_POLL_DELAY/WIFI_POLL_TRIES were already defined up top for
  // exactly this but never actually used, so if WIFI_SSID's network isn't
  // reachable the board hangs here forever and never reaches the AP
  // fallback below (or logo()/HasTRX() afterward).
  int wifiTries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTries < WIFI_POLL_TRIES) {
    delay(WIFI_POLL_DELAY);
    wifiTries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" 300: WiFi CL initialized");
    //Serial.println("310: WiFi IP  " + WiFi.localIP());
  }
  #endif

  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_AP);
    // Pre-existing bug, unrelated to this HAL work: softAP()'s return value
    // was never checked, so a rejected passphrase (WPA2-PSK requires 8-63
    // chars -- confirmed on real hardware with the previous 6-char
    // WIFI_APKEY, which softAP() silently refused) still printed
    // "WiFi AP initialized" with no AP actually up.
    if (WiFi.softAP(WIFI_APSSID, WIFI_APKEY)) {
      IPAddress ip = WiFi.softAPIP();
      Serial.println(" 300: WiFi AP initialized");
      //Serial.println(" 310: WiFi IP  " + ip);
    } else {
      Serial.println(" ERR: WiFi AP init failed (check WIFI_APKEY length -- WPA2-PSK needs 8-63 chars)");
    }
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
  #elif defined(HASV_ST7735_BOARD)
  // ST7735 is SPI, not I2C -- no Wire.begin() here; the adapter's own
  // init() brings up its dedicated SPI bus.
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

#ifdef HAS_AXP2101
void initPMU() {
  // T-Beam Supreme: LoRa radio, OLED, and sensors all run off 3.3V rails
  // gated by the AXP2101 PMU, reached over the ESP32's second I2C bus.
  // Rails/voltages match Meshtastic's own AXP2101 init for this board
  // (src/Power.cpp, LILYGO_TBEAM_S3_CORE branch); GNSS (ALDO4), the M.2
  // slot (DCDC3), and the SD card (BLDO1) are left off since nothing here
  // uses them.
  #ifdef PMU_USE_WIRE1
  Wire1.begin(I2C_SDA1, I2C_SCL1);
  #else
  Wire.begin();
  #endif
  if (PMU->init()) {
    PMU->setPowerChannelVoltage(XPOWERS_ALDO1, 3300);   // sensors/OLED/PCF8563 RTC VDD
    PMU->enablePowerOutput(XPOWERS_ALDO1);
    PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);   // required baseline rail (sensor comms)
    PMU->enablePowerOutput(XPOWERS_ALDO2);
    PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);   // LoRa radio VDD
    PMU->enablePowerOutput(XPOWERS_ALDO3);
    Serial.println(" 050: AXP2101 PMU initialized");
  } else {
    Serial.println(" ERR: AXP2101 PMU init failed - LoRa/OLED may be unpowered");
  }
}
#endif

void initLoRaRadio() {
  // Every board wires its own radio up by hand now, from the board's pin
  // definitions -- including Heltec boards, whose Heltec.begin() call
  // (setup(), below) deliberately skips its own bundled radio init
  // (LoRaEnable=false) so hvRadio is always the one driving the chip.
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  if (!hvRadio.begin(BAND)) {
    Serial.println(" ERR: LoRa radio init failed");
  }
}

//
// SETUP
//

void setup() {
  #if defined(HAS_AXP192) || defined(HAS_AXP2101)
  initPMU();
  #endif
  #ifdef HASV_HELTEC_BOARD
  // LoRa disabled: the radio is always driven by hvRadio/initLoRaRadio()
  // now (see HasTRX/sendLORA and the HASV_SX126X_BOARD comment above) --
  // Heltec.begin() here is display/Vext/serial bring-up only.
  Heltec.begin(true /*DisplayEnable*/, false /*LoRaEnable*/, true /*SerialEnable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
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
  initLoRaRadio();
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
