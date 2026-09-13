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
// Stick Lite) -- these are driven by the Heltec library (Heltec.begin()/
// Heltec.display/Heltec.LoRa) instead of talking to the radio and OLED
// directly. These macro names are the Heltec library's OWN board-select
// macros (see heltec.h) -- they have to be set exactly as it expects, not
// renamed to our own convention.
#if defined(WIFI_LORA_32) || defined(WIFI_LORA_32_V2) || defined(WIFI_LORA_32_V3) \
 || defined(WIRELESS_STICK) || defined(WIRELESS_STICK_LITE)
#define HASV_HELTEC_BOARD
#endif

// True for any board whose LoRa radio is an SX1262/SX1268 (RadioLib) instead
// of an SX1276/SX1277 (sandeepmistry/LoRa, or the Heltec library's bundled
// fork of it). Radio init/TX/RX go through a RadioLibSX126x adapter object
// (see below) that speaks the exact same method names the SX127x path uses,
// so HasTRX/sendLORA/onReceiveLORA/onWebSocketEvent don't need to know or
// care which radio chip is actually on the other end of "hvLoRa".
//
// WIFI_LORA_32_V3 belongs here despite also being HASV_HELTEC_BOARD above:
// its actual chip is an SX1262, not the SX1276 Heltec's own bundled LoRa
// library was written for. Confirmed on real hardware -- asking Heltec's
// library to init V3's radio doesn't just fail, it hangs the CPU forever
// (an unconditional while(1) in their own heltec.cpp when LoRa.begin() can't
// detect a chip it recognizes). V3 uses Heltec.begin()/Heltec.display for
// display+Vext+serial only (see HASV_HELTEC_RADIO_BOARD below); the radio
// goes through this same RadioLibSX126x adapter as the other four boards.
#if defined(LILYGO_T3_S3) || defined(BQ_STATION_G2) || defined(TBEAM_SUPREME) || defined(HELTEC_WIRELESS_TRACKER) || defined(WIFI_LORA_32_V3)
#define HASV_SX126X_BOARD
#endif

// True for Heltec boards whose real chip Heltec's own bundled LoRa library
// actually matches (SX1276) -- i.e. HASV_HELTEC_BOARD minus WIFI_LORA_32_V3.
// hvLoRa/initLoRaRadio() below key off this, not HASV_HELTEC_BOARD, so V3
// falls through to the RadioLibSX126x path like any other HASV_SX126X_BOARD.
#if defined(WIFI_LORA_32) || defined(WIFI_LORA_32_V2) || defined(WIRELESS_STICK) || defined(WIRELESS_STICK_LITE)
#define HASV_HELTEC_RADIO_BOARD
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
#ifdef HASV_HELTEC_BOARD
// See platformio.ini's Class_Wifi_LoRa build_flag comment for why this
// project has to define that macro itself -- a #define here in main.cpp
// would only affect this translation unit, not heltec.cpp's own separate
// compilation, so it has to be a compiler flag instead.
#include "heltec.h"
#else
#include <SPI.h>
#include <Wire.h>
#ifdef HASV_SX126X_BOARD
#include <RadioLib.h>
#else
#include <LoRa.h>
#endif
#ifdef HAS_OLED
#ifdef HASV_SH110X_BOARD
#include <Adafruit_SH110X.h>
#elif defined(HASV_ST7735_BOARD)
#include <Adafruit_ST7735.h>
#else
#include "SSD1306Wire.h"
#endif
#endif
#ifdef HAS_AXP192
#include <axp20x.h>
#endif
#ifdef HAS_AXP2101
#include <XPowersLib.h>
#endif
#endif
#if defined(HASV_HELTEC_BOARD) && defined(HASV_SX126X_BOARD)
// WIFI_LORA_32_V3 only: needs RadioLib for its actual SX1262 radio on top of
// heltec.h for display/Vext/serial -- see HASV_SX126X_BOARD's comment above.
#include <RadioLib.h>
#endif
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <WebSocketsServer.h>
#include "SPIFFS.h"
#include "ArduinoJson.h"
#include "FS.h"
#include "HVDN_logo.h"
#ifdef MESHTASTIC_PHY_TEST
// Phase 1 bring-up only -- see project_state.md's Meshtastic interop roadmap.
// Classic-mode builds never define MESHTASTIC_PHY_TEST, so never see this.
#include "Meshtastic_RadioConfig.h"
#endif


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
// SSD1306Wire -- same idea as RadioLibSX126x below, applied to the display
// instead of the radio. Templated on the concrete Adafruit_SH110X subclass
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

#ifdef HASV_SX126X_BOARD
// Adapter that makes a RadioLib SX1262 look like the old sandeepmistry
// LoRaClass API (setSyncWord/disableCrc/.../beginPacket/write/endPacket)
// that HasTRX/sendLORA/onReceiveLORA/onWebSocketEvent already call through
// "hvLoRa" -- so none of that shared code needs to know or care that the
// radio underneath is a completely different chip talking a completely
// different SPI protocol. Frequency/bandwidth arrive from the rest of the
// app in Hz (this project's convention); RadioLib wants MHz/kHz, so the
// conversion happens at the boundary here, once.
class RadioLibSX126x {
public:
  RadioLibSX126x(int cs, int dio1, int rst, int busy)
    : radio(new Module(cs, dio1, rst, busy)) {}

  bool begin(long frequencyHz) {
    int state = radio.begin(frequencyHz / 1.0e6, 125.0, 7, 5,
                             RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 17, 8,
                             SX126X_TCXO_VOLTAGE);
    if (state != RADIOLIB_ERR_NONE) return false;
    #ifdef SX126X_DIO2_AS_RF_SWITCH
    radio.setDio2AsRfSwitch(true);
    #endif
    radio.setDio1Action(onDio1Rise);
    return true;
  }

  // Pins are already bound in the constructor via Module; nothing to do.
  void setPins(int, int, int) {}

  void setSyncWord(int sw) { radio.setSyncWord((uint8_t)sw); }
  void disableCrc() { radio.setCRC(0); }
  // 2 = the "on" length RadioLib's own SX126x::begin() defaults to; matches
  // what disableCrc()'s setCRC(0) is already assumed to be turning off.
  void enableCrc() { radio.setCRC(2); }
  // Hardcoded to 8 inside begin() below; this lets callers change it
  // afterward (Meshtastic wants 16, not the classic-mode default).
  void setPreambleLength(long length) { radio.setPreambleLength((size_t)length); }
  void setFrequency(long freqHz) { radio.setFrequency(freqHz / 1.0e6); }
  void setTxPower(int level, int /*outputPin, no equivalent on SX126x*/) {
    #ifdef SX126X_MAX_POWER
    if (level > SX126X_MAX_POWER) level = SX126X_MAX_POWER;
    #endif
    radio.setOutputPower(level);
  }
  void setSignalBandwidth(long bwHz) { radio.setBandwidth(bwHz / 1000.0); }
  void setSpreadingFactor(int sf) { radio.setSpreadingFactor(sf); }
  void setCodingRate4(int denominator) { radio.setCodingRate(denominator); }

  void receive() { dio1Fired = false; radio.startReceive(); }

  int parsePacket() {
    if (!dio1Fired) return 0;
    dio1Fired = false;
    int state = radio.readData(rxBuf, sizeof(rxBuf) - 1);
    if (state != RADIOLIB_ERR_NONE) { rxLen = 0; return 0; }
    rxLen = radio.getPacketLength();
    rxPos = 0;
    return rxLen;
  }

  int read() { return (rxPos < rxLen) ? rxBuf[rxPos++] : -1; }
  int packetRssi() { return (int)radio.getRSSI(); }

  void beginPacket() { txLen = 0; }
  size_t write(uint8_t b) { if (txLen < sizeof(txBuf)) txBuf[txLen++] = b; return 1; }
  size_t print(const String &s) { for (size_t i = 0; i < s.length(); i++) write((uint8_t)s[i]); return s.length(); }
  void endPacket() { radio.transmit(txBuf, txLen); }

  void dumpRegisters(Stream &out) { out.println("dumpRegisters() is not supported on SX126x/RadioLib"); }

private:
  SX1262 radio;
  uint8_t rxBuf[256]; size_t rxLen = 0, rxPos = 0;
  uint8_t txBuf[256]; size_t txLen = 0;
  static volatile bool dio1Fired;
  static void IRAM_ATTR onDio1Rise();
};
volatile bool RadioLibSX126x::dio1Fired = false;
// Defined out-of-line: an IRAM_ATTR function defined inline inside the class
// body trips the Xtensa toolchain's "literal placed after use" relocation
// error, since the literal pool ends up on the wrong side of the jump.
void IRAM_ATTR RadioLibSX126x::onDio1Rise() { dio1Fired = true; }

RadioLibSX126x sx126xRadio(LORA_CS, SX126X_DIO1, LORA_RST, SX126X_BUSY);
#endif

// LoRa radio handle. The Heltec library owns its own LoRaClass instance as a
// member (Heltec.LoRa) rather than the bare global "LoRa" object that
// standalone LoRa libraries (and the Heltec library's own internals,
// confusingly, expose under the same name) provide -- so on Heltec boards
// the bare "LoRa" symbol in scope here is a *different*, never-initialized
// object. hvLoRa always points at whichever one Heltec.begin()/initLoRaRadio()
// actually set up. Keyed on HASV_HELTEC_RADIO_BOARD, not HASV_HELTEC_BOARD --
// WIFI_LORA_32_V3 is the latter (display/Vext/serial via Heltec.begin()) but
// not the former (its radio is an SX1262, see HASV_SX126X_BOARD's comment).
#ifdef HASV_HELTEC_RADIO_BOARD
#define hvLoRa Heltec.LoRa
#elif defined(HASV_SX126X_BOARD)
#define hvLoRa sx126xRadio
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
    hvLoRa.setTxPower(txpwr,RF_PACONFIG_PASELECT_PABOOST);
    #ifdef MESHTASTIC_PHY_TEST
    // Phase 1 bring-up: EU_433 + LongFast, computed via the same lookup/formula
    // Phase 2+ will reuse -- see Meshtastic_RadioConfig.h and project_state.md's
    // hand-derived cross-check (433.875MHz). Ignores the classic-mode
    // modemconfig/frequency JSON settings entirely.
    {
      const MeshtasticRegion &meshRegion = meshtasticFindRegion("EU_433");
      const MeshtasticPreset &meshPreset = meshtasticFindPreset("LongFast");
      float meshFreqMHz = meshtasticFrequency(meshRegion, meshPreset.bwKHz, meshPreset.name);
      hvLoRa.setSyncWord(0x2B);
      hvLoRa.enableCrc();
      hvLoRa.setPreambleLength(16);
      hvLoRa.setFrequency((long)(meshFreqMHz * 1.0e6));
      hvLoRa.setSignalBandwidth((long)(meshPreset.bwKHz * 1000));
      hvLoRa.setSpreadingFactor(meshPreset.sf);
      hvLoRa.setCodingRate4(meshPreset.cr);
      Serial.print("MESH: PHY test freq (MHz): ");
      Serial.println(meshFreqMHz, 6);
    }
    #else
    hvLoRa.setSyncWord(0xFF);                 // Set for LoRa Broadcast
    hvLoRa.disableCrc();
    hvLoRa.setFrequency(frequency);
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
    #endif
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
  // Pre-existing bug, unrelated to mesh work: this loop had no bound at all --
  // WIFI_POLL_DELAY/WIFI_POLL_TRIES were already defined up top for exactly
  // this but never actually used, so if WIFI_SSID's network isn't reachable
  // the board hangs here forever and never reaches the AP fallback below (or
  // logo()/HasTRX() afterward). Confirmed on real hardware: a HiLetgo V3
  // sitting blank since first boot with HASviolet_config.h's placeholder
  // WIFI_SSID ("HomeWAN") not present at the test bench.
  int wifiTries = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTries < WIFI_POLL_TRIES) {
    delay(WIFI_POLL_DELAY);
    wifiTries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" 300: WiFi CL initialized");
    //Serial.println("310: WiFi IP  " + WiFi.localIP());
    //Serial.println("320: WiFi CL initialized");
  }
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

#ifndef HASV_HELTEC_RADIO_BOARD
void initLoRaRadio() {
  // HASV_HELTEC_RADIO_BOARD boards get this for free from Heltec.begin();
  // everyone else -- including WIFI_LORA_32_V3, whose radio Heltec.begin()
  // deliberately skips (see setup() below) -- wires the radio up by hand
  // from the board's variant pin definitions.
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
  #if defined(HAS_AXP192) || defined(HAS_AXP2101)
  initPMU();
  #endif
  #ifdef HASV_HELTEC_BOARD
  #ifdef HASV_HELTEC_RADIO_BOARD
  Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  #else
  // WIFI_LORA_32_V3: LoRaEnable=false. Heltec's own bundled LoRa init only
  // knows SX1276 and permanently hangs the CPU (an unconditional while(1) in
  // their own heltec.cpp) if asked to init V3's actual SX1262 chip --
  // confirmed on real hardware. Display/Vext/serial still come from
  // Heltec.begin(); the radio comes from initLoRaRadio() below instead.
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable -- see comment*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  #endif
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
  #ifndef HASV_HELTEC_RADIO_BOARD
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
