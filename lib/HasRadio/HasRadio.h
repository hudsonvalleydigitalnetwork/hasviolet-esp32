#pragma once
#include <Arduino.h>

// Shared radio hardware-abstraction layer.
//
// Every HASviolet networking backend -- native broadcast today, MeshCore
// later -- drives the LoRa chip through exactly one HasRadio instance rather
// than each owning its own radio object. Method names/shapes are
// deliberately close to MeshCore's own mesh::Radio interface (startSendRaw/
// recvRaw/isInRecvMode/getLastRSSI, etc.) so that a later phase vendoring
// MeshCore's Mesh/Dispatcher onto this same HAL is a near drop-in, not a
// second rewrite. See project_state.md for the fuller comparison this was
// designed against.
//
// TX is intentionally still blocking under the hood in both current
// implementations (HasRadioSX126x/HasRadioSX127x) -- startSendRaw() doesn't
// return until the packet is actually on air, matching this project's
// original behavior exactly (the old RadioLibSX126x adapter's endPacket()
// called RadioLib's blocking transmit(), and the SX127x libraries this
// project used before also block in their own send calls). isSendComplete()
// exists on the interface for when MeshCore's own non-blocking TX arrives;
// today's implementations just always report true.
class HasRadio {
public:
  virtual ~HasRadio() {}

  // One-time chip init; pins are already bound at construction (matches the
  // pattern this project's original per-chip adapter used).
  virtual bool begin(long frequencyHz) = 0;

  virtual void setFrequency(long frequencyHz) = 0;
  virtual void setBandwidth(long bandwidthHz) = 0;
  virtual void setSpreadingFactor(int sf) = 0;
  virtual void setCodingRate(int denominator) = 0;
  virtual void setTxPower(int dBm) = 0;
  virtual void setSyncWord(int syncWord) = 0;
  virtual void setCrc(bool enabled) = 0;
  virtual void setPreambleLength(long length) = 0;

  // Arms the radio to receive continuously. Safe to call again after a TX or
  // a channel-parameter change, same as the old parsePacket()-based loop
  // re-arming on every HasTRX() restart.
  virtual void startReceive() = 0;

  // Non-blocking poll: 0 if nothing has arrived yet, else the number of
  // bytes copied into buf (up to maxLen). Mirrors the old
  // parsePacket()+read() loop collapsed into one call.
  virtual int recvRaw(uint8_t *buf, int maxLen) = 0;

  // Sends len bytes from buf. See the class comment above re: blocking.
  // Returns false only if the send could not even be started (e.g. buffer
  // too large for this chip).
  virtual bool startSendRaw(const uint8_t *buf, int len) = 0;
  virtual bool isSendComplete() = 0;

  // RSSI of the last received packet, in dBm. SNR is intentionally left off
  // this interface for now -- nothing consumes it yet; add it alongside
  // MeshCore's routing-score use of SNR when that lands.
  virtual int getLastRSSI() = 0;

  virtual void dumpRegisters(Stream &out) = 0;
};
