#include "HasRadioSX126x.h"

// PlatformIO's "one lib/ folder, one static lib" model compiles this .cpp
// for every board environment, not just the SX126x ones that actually
// construct a HasRadioSX126x -- so board-specific macros set only in
// platformio.ini's SX126x [env:...] sections (see HASV_SX126X_BOARD's
// comment in main.cpp for which boards those are) need an inert fallback
// here rather than being assumed to exist.
#ifndef SX126X_TCXO_VOLTAGE
#define SX126X_TCXO_VOLTAGE 0
#endif

volatile bool HasRadioSX126x::dio1Fired = false;

// Defined out-of-line: an IRAM_ATTR function defined inline inside the class
// body trips the Xtensa toolchain's "literal placed after use" relocation
// error, since the literal pool ends up on the wrong side of the jump.
void IRAM_ATTR HasRadioSX126x::onDio1Rise() { dio1Fired = true; }

HasRadioSX126x::HasRadioSX126x(int cs, int dio1, int rst, int busy)
  : radio(new Module(cs, dio1, rst, busy)) {}

bool HasRadioSX126x::begin(long frequencyHz) {
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

void HasRadioSX126x::setFrequency(long frequencyHz) { radio.setFrequency(frequencyHz / 1.0e6); }
void HasRadioSX126x::setBandwidth(long bandwidthHz) { radio.setBandwidth(bandwidthHz / 1000.0); }
void HasRadioSX126x::setSpreadingFactor(int sf) { radio.setSpreadingFactor(sf); }
void HasRadioSX126x::setCodingRate(int denominator) { radio.setCodingRate(denominator); }

void HasRadioSX126x::setTxPower(int dBm) {
  #ifdef SX126X_MAX_POWER
  if (dBm > SX126X_MAX_POWER) dBm = SX126X_MAX_POWER;
  #endif
  radio.setOutputPower(dBm);
}

void HasRadioSX126x::setSyncWord(int syncWord) { radio.setSyncWord((uint8_t)syncWord); }

// Length 2 is the "on" value RadioLib's own SX126x::begin() defaults to;
// matches what disabling CRC (setCRC(0)) is already assumed to be turning
// off.
void HasRadioSX126x::setCrc(bool enabled) { radio.setCRC(enabled ? 2 : 0); }

void HasRadioSX126x::setPreambleLength(long length) { radio.setPreambleLength((size_t)length); }

void HasRadioSX126x::startReceive() { dio1Fired = false; radio.startReceive(); }

int HasRadioSX126x::recvRaw(uint8_t *buf, int maxLen) {
  if (!dio1Fired) return 0;
  dio1Fired = false;
  int state = radio.readData(buf, maxLen);
  if (state != RADIOLIB_ERR_NONE) return 0;
  return radio.getPacketLength();
}

// Blocking under the hood -- see HasRadio.h's class comment. RadioLib's
// transmit() doesn't return until the packet is actually on air.
bool HasRadioSX126x::startSendRaw(const uint8_t *buf, int len) {
  radio.transmit((uint8_t *)buf, len);
  return true;
}

bool HasRadioSX126x::isSendComplete() { return true; }

int HasRadioSX126x::getLastRSSI() { return (int)radio.getRSSI(); }

void HasRadioSX126x::dumpRegisters(Stream &out) {
  out.println("dumpRegisters() is not supported on SX126x/RadioLib");
}
