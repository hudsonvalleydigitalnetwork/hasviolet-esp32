#include "HasMeshCoreChat.h"

// See HasMeshCoreChat.h's own top-of-file comment: this file compiles to
// nothing on every board that doesn't vendor MeshCore.
#ifdef HASV_MESHCORE_SUPPORT

#include <WebSocketsServer.h>

// Declared in main.cpp; HasTRX's native-mode RX path already broadcasts
// through this same object the same way (see HasTRX() in main.cpp).
extern WebSocketsServer webSocket;

// Deliberately NOT #include <base64.hpp> -- its function bodies aren't
// marked inline, so a second translation unit including it is a One
// Definition Rule violation at link time. Confirmed the hard way: this
// only started failing once MAX_GROUP_CHANNELS was defined (see
// platformio.ini), because that's what makes MeshCore's own
// BaseChatMesh.cpp start including it too (its addChannel() stub, taken
// when MAX_GROUP_CHANNELS is undefined, never touches base64 at all).
// Forward-declaring just the one function used here lets the linker
// resolve it against BaseChatMesh.cpp's copy instead of defining a second,
// colliding one.
extern unsigned int encode_base64(const unsigned char input[], unsigned int input_length, unsigned char output[]);

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
  if (_channel == nullptr) {
    Serial.println(" ERR: MeshCore addChannel() failed -- MAX_GROUP_CHANNELS not defined, or psk wasn't 16/32 raw bytes");
  } else {
    Serial.print(" MC: joined channel \"");
    Serial.print(channelName);
    Serial.println("\"");
  }
}

bool HasMeshCoreChat::sendChannelText(const String& text) {
  if (_channel == nullptr) {
    Serial.println(" ERR: MeshCore sendChannelText() -- no channel joined");
    return false;
  }
  uint32_t timestamp = getRTCClock()->getCurrentTime();
  bool ok = sendGroupMessage(timestamp, _channel->channel, _nodeName, text.c_str(), text.length());
  Serial.print(" MC TX (");
  Serial.print(ok ? "sent" : "FAILED");
  Serial.print("): ");
  Serial.println(text);
  return ok;
}

void HasMeshCoreChat::onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt, uint32_t timestamp, const char* text) {
  String msg = "RX:";
  msg += text;
  Serial.print("MC RX:");
  Serial.println(text);
  webSocket.broadcastTXT(msg);
}

#endif // HASV_MESHCORE_SUPPORT
