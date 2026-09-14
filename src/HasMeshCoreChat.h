#pragma once

// This whole file compiles to nothing on every board except the one(s)
// that actually vendor MeshCore (see platformio.ini's HASV_MESHCORE_SUPPORT
// build flag) -- PlatformIO compiles every .cpp under src/ unconditionally
// regardless of board, so HasMeshCoreChat.cpp's #include of this header
// would otherwise fail to find MeshCore's own headers on every other board.
#ifdef HASV_MESHCORE_SUPPORT

// MeshCore application layer: a trimmed-down version of MeshCore's own
// examples/simple_secure_chat/main.cpp pattern, cut down to channel-only
// messaging (see project_state.md -- no contacts/DMs in this phase, since
// HASviolet's native protocol has no addressing concept to map them onto
// anyway). Bridges MeshCore's "channel" flood messaging to the same
// webSocket.broadcastTXT("RX:...")/TX: wire shape the browser UI already
// speaks in native mode, so no client-side changes are needed to support
// this second backend.
//
// Everything below is HASviolet's own code -- nothing here is a modified
// copy of any MeshCore file (see LICENSE.md's vendoring policy). MeshCore
// itself is pulled in unmodified via platformio.ini's lib_deps.

// MeshCore's own helpers/ESP32Board.h includes <driver/rtc_io.h> without
// including <driver/gpio.h> first, even though rtc_io.h uses GPIO_PIN_COUNT
// (defined by gpio.h, not rtc_io.h itself) -- a real gap in MeshCore's own
// vendored header, not something to patch there. Whether it actually fails
// depends entirely on include order elsewhere in the same translation unit
// (confirmed: it silently worked before HasMeshCoreChat.cpp existed, then
// broke once LDF's file-compile order shifted).
//
// Fixing this by just #include <driver/gpio.h> doesn't work here: this
// project also depends on heltecautomation/Heltec ESP32 Dev-Boards (for
// display bring-up), and its bundled-but-unused LoRaWAN stack ships its
// OWN, completely unrelated driver/gpio.h (Semtech's board-GPIO shim,
// #ifndef __GPIO_H__, no GPIO_PIN_COUNT at all) at the exact same
// "driver/gpio.h" path -- whichever -I search path GCC consults first for
// that angle-bracket include wins, and it isn't guaranteed to be the real
// ESP-IDF one. Defining GPIO_PIN_COUNT directly from the unambiguous
// soc_caps.h path sidesteps that collision instead of trying to win it.
#include <soc/soc_caps.h>
#ifndef GPIO_PIN_COUNT
#define GPIO_PIN_COUNT SOC_GPIO_PIN_COUNT
#endif
// Same shadowing problem, same fix, for the handful of real ESP-IDF GPIO
// functions ESP32Board.h itself calls directly. esp_err.h/hal/gpio_types.h
// (for esp_err_t/gpio_num_t/gpio_int_type_t) aren't in the Heltec stub's
// driver/ folder so aren't at risk the way driver/gpio.h itself is. The
// real symbols are still linked in from arduino-esp32's own GPIO driver
// component regardless of which header declared them; this just gives the
// compiler a declaration to compile against.
#include <esp_err.h>
#include <hal/gpio_types.h>
extern "C" {
esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type);
int gpio_get_level(gpio_num_t gpio_num);
esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type);
esp_err_t gpio_wakeup_disable(gpio_num_t gpio_num);
}
// See meshcore_ed25519_compat.h's own comment: this #include's only real
// purpose is giving PlatformIO's LDF a graph edge to lib/MeshCoreEd25519Compat/
// so it actually gets compiled (LDF's default mode discovers lib/ folders
// by following #include chains from src/, not by scanning lib/ wholesale).
#include <meshcore_ed25519_compat.h>
#include <Mesh.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/IdentityStore.h>
#include <helpers/BaseChatMesh.h>
#include <RTClib.h>
#include <target.h>
#include <FS.h>

class HasMeshCoreChat : public BaseChatMesh {
public:
  HasMeshCoreChat(mesh::Radio& radio, StdRNG& rng, mesh::RTCClock& rtc, SimpleMeshTables& tables);

  // fs: SPIFFS, already begun by initSPIFFS() before this is called.
  // channelName: HASviolet's existing "channel" config string (e.g. "HV1")
  // -- reused as-is so native and MeshCore modes agree on which group to
  // join whenever the user already picked a channel name in hasVIOLET.json.
  void begin(FILESYSTEM& fs, const String& channelName);

  // Sends text on the joined channel. Returns false if send couldn't be
  // started (e.g. channel not joined yet).
  bool sendChannelText(const String& text);

protected:
  // BaseChatMesh's required overrides. Only onChannelMessageRecv() does
  // anything -- the rest are contact/DM/ack machinery this phase doesn't
  // use (see class comment above), stubbed the same minimal way MeshCore's
  // own simple_secure_chat example does for the pieces it doesn't need either.
  float getAirtimeBudgetFactor() const override { return 1.0f; }
  int calcRxDelay(float score, uint32_t air_time) const override { return 0; }
  bool allowPacketForward(const mesh::Packet* packet) override { return true; }
  void onDiscoveredContact(ContactInfo& contact, bool is_new, uint8_t path_len, const uint8_t* path) override {}
  void onContactPathUpdated(const ContactInfo& contact) override {}
  ContactInfo* processAck(const uint8_t* data) override { return nullptr; }
  void onMessageRecv(const ContactInfo& from, mesh::Packet* pkt, uint32_t sender_timestamp, const char* text) override {}
  void onCommandDataRecv(const ContactInfo& from, mesh::Packet* pkt, uint32_t sender_timestamp, const char* text) override {}
  void onSignedMessageRecv(const ContactInfo& from, mesh::Packet* pkt, uint32_t sender_timestamp, const uint8_t* sender_prefix, const char* text) override {}
  void onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt, uint32_t timestamp, const char* text) override;
  uint8_t onContactRequest(const ContactInfo& contact, uint32_t sender_timestamp, const uint8_t* data, uint8_t len, uint8_t* reply) override { return 0; }
  void onContactResponse(const ContactInfo& contact, const uint8_t* data, uint8_t len) override {}
  uint32_t calcFloodTimeoutMillisFor(uint32_t pkt_airtime_millis) const override { return 500 + 16 * pkt_airtime_millis; }
  uint32_t calcDirectTimeoutMillisFor(uint32_t pkt_airtime_millis, uint8_t path_len) const override { return 500; }
  void onSendTimeout() override {}

private:
  ChannelDetails* _channel = nullptr;
  char _nodeName[32] = "HASviolet";
};

#endif // HASV_MESHCORE_SUPPORT
