#pragma once
#include "HasRadio.h"
#include <RadioLib.h>

// SX1276/SX1277 implementation of the shared HasRadio interface, replacing
// this project's previous per-board radio drivers for these chips
// (sandeepmistry/LoRa on TTGO/T-Beam boards, the Heltec library's own
// bundled LoRa fork on Heltec V1/V2/Wireless Stick(Lite)) with RadioLib --
// the same driver library HasRadioSX126x already used, so every board in
// this project now shares one radio driver family. On Heltec boards the
// Heltec library is still used for display/Vext/serial bring-up
// (Heltec.begin() with LoRaEnable=false, see main.cpp) but never for the
// radio itself.
//
// Interrupt line is DIO0 (RadioLib's setDio0Action), not DIO1 -- SX127x's
// default IRQ mapping puts RX-done/TX-done on DIO0, which is what this
// project's LORA_IRQ pin already refers to for every SX127x board.
class HasRadioSX127x : public HasRadio {
public:
  HasRadioSX127x(int cs, int dio0, int rst);

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
  SX1276 radio;
  static volatile bool dio0Fired;
  static void IRAM_ATTR onDio0Rise();
};
