#include "HasRadioSX127x.h"

volatile bool HasRadioSX127x::dio0Fired = false;

// Defined out-of-line: an IRAM_ATTR function defined inline inside the class
// body trips the Xtensa toolchain's "literal placed after use" relocation
// error, since the literal pool ends up on the wrong side of the jump (same
// reason HasRadioSX126x's ISR is out-of-line).
void IRAM_ATTR HasRadioSX127x::onDio0Rise() { dio0Fired = true; }

HasRadioSX127x::HasRadioSX127x(int cs, int dio0, int rst)
  : radio(new Module(cs, dio0, rst)) {}

bool HasRadioSX127x::begin(long frequencyHz) {
  int state = radio.begin(frequencyHz / 1.0e6, 125.0, 7, 5,
                           RADIOLIB_SX127X_SYNC_WORD, 17, 8, 0);
  if (state != RADIOLIB_ERR_NONE) return false;
  radio.setDio0Action(onDio0Rise, RISING);
  return true;
}

void HasRadioSX127x::setFrequency(long frequencyHz) { radio.setFrequency(frequencyHz / 1.0e6); }
void HasRadioSX127x::setBandwidth(long bandwidthHz) { radio.setBandwidth(bandwidthHz / 1000.0); }
void HasRadioSX127x::setSpreadingFactor(int sf) { radio.setSpreadingFactor(sf); }
void HasRadioSX127x::setCodingRate(int denominator) { radio.setCodingRate(denominator); }

void HasRadioSX127x::setTxPower(int dBm) {
  // Every SX127x board in this project has its radio wired to the PA_BOOST
  // RF output, not RFO (see platformio.ini's generic_radio comment).
  // RadioLib's setOutputPower(power) only selects PA_BOOST for power >= 2
  // dBm -- below that it silently switches to the RFO output-power
  // register bits, which on this hardware aren't connected to anything.
  // None of this project's presets use anything below 2 dBm today, but
  // clamping here keeps that true even if one ever does.
  if (dBm < 2) dBm = 2;
  radio.setOutputPower(dBm);
}

void HasRadioSX127x::setSyncWord(int syncWord) { radio.setSyncWord((uint8_t)syncWord); }
void HasRadioSX127x::setCrc(bool enabled) { radio.setCRC(enabled); }
void HasRadioSX127x::setPreambleLength(long length) { radio.setPreambleLength((size_t)length); }

void HasRadioSX127x::startReceive() { dio0Fired = false; radio.startReceive(); }

int HasRadioSX127x::recvRaw(uint8_t *buf, int maxLen) {
  if (!dio0Fired) return 0;
  dio0Fired = false;
  int state = radio.readData(buf, maxLen);
  if (state != RADIOLIB_ERR_NONE) return 0;
  return radio.getPacketLength();
}

// Blocking under the hood -- see HasRadio.h's class comment. RadioLib's
// transmit() doesn't return until the packet is actually on air.
bool HasRadioSX127x::startSendRaw(const uint8_t *buf, int len) {
  radio.transmit((uint8_t *)buf, len);
  return true;
}

bool HasRadioSX127x::isSendComplete() { return true; }

int HasRadioSX127x::getLastRSSI() { return (int)radio.getRSSI(); }

void HasRadioSX127x::dumpRegisters(Stream &out) {
  // The sandeepmistry/LoRa library this project used before had a real
  // register dump here; RadioLib doesn't expose raw register reads on its
  // public API (only via a "godmode" build, which this project doesn't
  // enable -- see project_state.md's HAL notes). GET:LORA just reports that
  // for now rather than pretending to support it.
  out.println("dumpRegisters() is not supported on SX127x/RadioLib");
}
