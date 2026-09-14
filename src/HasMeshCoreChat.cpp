#include "HasMeshCoreChat.h"

// See HasMeshCoreChat.h's own top-of-file comment: this file compiles to
// nothing on every board that doesn't vendor MeshCore.
#ifdef HASV_MESHCORE_SUPPORT

#include <WebSocketsServer.h>
#include <base64.hpp>

// Declared in main.cpp; HasTRX's native-mode RX path already broadcasts
// through this same object the same way (see HasTRX() in main.cpp).
extern WebSocketsServer webSocket;

HasMeshCoreChat::HasMeshCoreChat(mesh::Radio& radio, StdRNG& rng, mesh::RTCClock& rtc, SimpleMeshTables& tables)
  : BaseChatMesh(radio, *new ArduinoMillis(), rng, rtc, *new StaticPoolPacketManager(16), tables) {
}

void HasMeshCoreChat::begin(FILESYSTEM& fs, const String& channelName) {
  BaseChatMesh::begin();

  IdentityStore store(fs, "/identity");
  if (!store.load("_main", self_id, _nodeName, sizeof(_nodeName))) {
    self_id = mesh::LocalIdentity(getRNG());  // create new random identity
    int count = 0;
    while (count < 10 && (self_id.pub_key[0] == 0x00 || self_id.pub_key[0] == 0xFF)) {  // reserved id hashes
      self_id = mesh::LocalIdentity(getRNG());
      count++;
    }
    store.save("_main", self_id);
  }

  // HASviolet's own channel name doubles as the MeshCore group name; the
  // PSK is derived from it (SHA256 -> base64) rather than entered
  // separately -- anyone who already agrees on a channel name in native
  // mode lands in the same MeshCore group too. Not a secure secret-sharing
  // scheme (the PSK is fully determined by the public channel name), same
  // security posture as native mode's own unauthenticated broadcast.
  uint8_t channelSecret[32];
  mesh::Utils::sha256(channelSecret, sizeof(channelSecret),
                       (const uint8_t *)channelName.c_str(), channelName.length());
  char psk[64];
  unsigned int pskLen = encode_base64(channelSecret, sizeof(channelSecret), (unsigned char *)psk);
  psk[pskLen] = 0;
  _channel = addChannel(channelName.c_str(), psk);
}

bool HasMeshCoreChat::sendChannelText(const String& text) {
  if (_channel == nullptr) return false;
  uint32_t timestamp = getRTCClock()->getCurrentTime();
  return sendGroupMessage(timestamp, _channel->channel, _nodeName, text.c_str(), text.length());
}

void HasMeshCoreChat::onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt, uint32_t timestamp, const char* text) {
  String msg = "RX:";
  msg += text;
  Serial.print("MC RX:");
  Serial.println(text);
  webSocket.broadcastTXT(msg);
}

#endif // HASV_MESHCORE_SUPPORT
