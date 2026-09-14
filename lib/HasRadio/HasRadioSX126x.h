#pragma once
#include "HasRadio.h"
#include <RadioLib.h>

// SX1262/SX1268 implementation of the shared HasRadio interface. This is a
// direct port of this project's original RadioLibSX126x adapter (previously
// inline in src/main.cpp, back when it was shaped like the old Arduino
// LoRaClass API) -- same RadioLib calls, same DIO1-ISR-flag pattern, just
// reshaped onto HasRadio so it can sit alongside HasRadioSX127x behind one
// interface. Used by every SX126x board (T3-S3, BQ Station G2, T-Beam
// Supreme, Heltec Wireless Tracker, Heltec WiFi LoRa 32 V3 -- V3's real chip
// is an SX1262 despite being a Heltec board otherwise, see main.cpp).
class HasRadioSX126x : public HasRadio {
public:
  HasRadioSX126x(int cs, int dio1, int rst, int busy);

  bool begin(long frequencyHz) override;
  void setFrequency(long frequencyHz) override;
  void setBandwidth(long bandwidthHz) override;
  void setSpreadingFactor(int sf) override;
  void setCodingRate(int denominator) override;
  void setTxPower(int dBm) override;
  void setSyncWord(int syncWord) override;
  void setCrc(bool enabled) override;
  void setPreambleLength(long length) override;
  void startReceive() override;
  int recvRaw(uint8_t *buf, int maxLen) override;
  bool startSendRaw(const uint8_t *buf, int len) override;
  bool isSendComplete() override;
  int getLastRSSI() override;
  void dumpRegisters(Stream &out) override;

private:
  SX1262 radio;
  static volatile bool dio1Fired;
  static void IRAM_ATTR onDio1Rise();
};
