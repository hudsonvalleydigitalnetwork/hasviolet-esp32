# Meshtastic On-Air Wire Format — Spec Lock (Phase 0)

Status: **reference only — nothing in `src/` reads this yet.** This is the Phase 0
deliverable from the Meshtastic interop plan in [project_state.md](../../project_state.md):
a byte-level spec of the parts of the Meshtastic wire protocol this project needs for
on-air interop, pulled from the actual firmware/protobuf source rather than docs or
memory — same discipline already used for this project's board pin tables.

Explicitly out of scope (per the plan): Meshtastic's phone/companion-app surface
(BLE/serial/TCP device API, admin protobufs, PKI/Curve25519 direct-message encryption).
This doc only covers what's needed to send/receive broadcast text on a shared channel
and behave as a correct flood-routing relay.

## Pinned sources

Everything below was read directly out of these two archives (downloaded, extracted,
and grepped — not fetched through a summarizer) rather than assumed:

| Repo | Commit | Tag |
|---|---|---|
| `meshtastic/firmware` | `54e0d8d0ab2ff56b3a9ce967e53f79e49af560fb` | `v2.7.26.54e0d8d` (latest release as of 2026-09-13) |
| `meshtastic/protobufs` | `6b1ded439633cd03d4af85b44231b91d1d106278` | (submodule pin of the firmware tag above) |

If Meshtastic's wire format changes upstream, re-pull both archives at a new tag and
diff against this file before touching any implementation code — don't hand-edit this
spec from memory.

**License note (decided, see project_state.md):** both repos above are GPL-3.0;
HASviolet-ESP32 is MIT (see [LICENSE.md](../../LICENSE.md)) and stays that way, so it
cannot absorb GPL-3.0 source. Everything in this file is transcribed *facts about a
wire format* (struct layouts, algorithms, constants) for interop purposes, not copied
source text. Actual implementation code must be written fresh from this spec, not
copy-pasted from the firmware — permanently, not just until a licensing decision lands.

---

## 1. PHY layer

| Parameter | Value | Source |
|---|---|---|
| Preamble length | **16 symbols** (not the SX12xx/RadioLib default of 8) | `src/mesh/RadioInterface.h:98` |
| Sync word | **`0x2B`**, fixed — no separate public/private value in current firmware | `src/mesh/RadioLibInterface.h:84` |
| Hardware CRC | **On** (`RADIOLIB_SX126X_LORA_CRC_ON` / equivalent SX127x call) | `src/mesh/SX126xInterface.cpp:201`, `RF95Interface.cpp:191` |
| Header mode | Explicit (variable length) — implicit here, no code changed this from the RadioLib/LoRa library default | — |

This project's current `hvLoRa` adapters do the opposite on two of these three: `HasTRX()`
calls `hvLoRa.disableCrc()` unconditionally, and preamble length isn't exposed on the
adapter interface at all (defaults to whatever RadioLib/sandeepmistry's `LoRa.begin()`
picks, which is 8). Phase 1 needs to add `setPreambleLength()` and `enableCrc()` to the
adapter and branch HASviolet-classic vs. mesh-mode radio config on them.

## 2. Regions relevant to the 433–510MHz test hardware

Full table has 27 entries (`src/mesh/RadioInterface.cpp:43`, the `RDEF(...)` macro list);
these are the ones whose frequency range overlaps the Heltec HiLetgo V3 433–510MHz test
boards:

| Region | Range (MHz) | Duty cycle | Power limit (dBm) |
|---|---|---|---|
| `EU_433` | 433.0 – 434.0 | 10% | 10 |
| `CN` | 470.0 – 510.0 | 100% | 19 |
| `ANZ_433` | 433.05 – 434.79 | 100% | 14 |
| `UA_433` | 433.0 – 434.7 | 10% | 10 |
| `MY_433` | 433.0 – 435.0 | 100% | 20 |
| `KZ_433` | 433.075 – 434.775 | 100% | 10 |
| `PH_433` | 433.0 – 434.7 | 100% | 10 |

**Open decision, not mine to make:** which region string this device should actually
identify as depends on where it's legally operated, not just which frequencies the
hardware can tune to. `CN` has by far the widest span (40MHz) and highest power ceiling
of this set, but claiming a region is a regulatory statement, not just a frequency
range pick — worth an explicit call before Phase 1, most likely `EU_433` or `CN` given
the hardware's stated band. All of them share `spacing = 0`, `audio_permitted = true`,
`frequency_switching = false`, `wide_lora = false` — none of the special cases (LR11xx
dual-band, 2.4GHz `LORA_24`) apply to this hardware.

## 3. Modem presets (non-wideLora column — this hardware isn't LR11xx)

Exact table from `modemPresetToParams()`, `src/mesh/MeshRadio.h:132`:

| Preset | Bandwidth (kHz) | Spreading factor | Coding rate (4/x) |
|---|---|---|---|
| `SHORT_TURBO` | 500 | 7 | 5 |
| `SHORT_FAST` | 250 | 7 | 5 |
| `SHORT_SLOW` | 250 | 8 | 5 |
| `MEDIUM_FAST` | 250 | 9 | 5 |
| `MEDIUM_SLOW` | 250 | 10 | 5 |
| `LONG_TURBO` | 500 | 11 | 8 |
| `LONG_MODERATE` | 125 | 11 | 8 |
| `LONG_SLOW` | 125 | 12 | 8 |
| **`LONG_FAST`** (default) | **250** | **11** | **5** | 

`LONG_FAST`/250kHz/SF11/CR4:5 matches the public web docs consulted during planning —
cross-checked, not just trusted from one source.

## 4. Frequency selection (which exact MHz to transmit on)

Two *different* hash functions are in play — do not conflate them (a real bug risk):

- **djb2** (`src/mesh/RadioInterface.cpp:736`, `hash(const char*)`) picks which frequency
  slot to use, over the **channel's display name** (see §6 for what that name actually is
  for the default channel).
- **XOR hash** (§6 below) computes the **1-byte `channel` field** in the packet header —
  a completely separate value, used by receivers to filter which locally-configured
  channel/PSK to try, not to pick a frequency.

Frequency slot algorithm, verbatim from `RadioInterface.cpp:840-862`:

```
numChannels = floor((region.freqEnd - region.freqStart) / (region.spacing + bwKHz/1000))
channel_num = (loraConfig.channel_num != 0)
                ? loraConfig.channel_num - 1
                : djb2_hash(primaryChannelDisplayName) % numChannels
freq = region.freqStart + (bwKHz / 2000) + channel_num * (bwKHz / 1000)
```

`primaryChannelDisplayName` for the default public channel is `"LongFast"` (see §6) —
so for `LONG_FAST` + an unconfigured `channel_num` (the out-of-box state on every real
node), the frequency is fully determined by region + `djb2("LongFast") % numChannels`.
This is directly computable and should be spot-checked against a real HiLetgo V3's own
serial log (`LOG_INFO("frequency: %f", ...)`) before writing any code — cheapest possible
PHY-parity check.

## 5. `PacketHeader` — 16 bytes, exact wire layout

From `src/mesh/RadioInterface.h:34` (`MESHTASTIC_HEADER_LENGTH = 16`); `NodeNum` and
`PacketId` are both `typedef uint32_t` (`src/mesh/MeshTypes.h:9-10`):

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0 | 4 | `to` | destination NodeNum, `0xFFFFFFFF` = broadcast |
| 4 | 4 | `from` | sender NodeNum |
| 8 | 4 | `id` | packet sequence number |
| 12 | 1 | `flags` | bitfield, see below |
| 13 | 1 | `channel` | XOR channel hash (§6) — hint for decoder, not a frequency |
| 14 | 1 | `next_hop` | last byte of intended next-hop NodeNum, 0 = none |
| 15 | 1 | `relay_node` | last byte of the NodeNum that most recently relayed this packet |

`flags` byte (`RadioInterface.h:24-28`):

| Bits | Mask | Field | Notes |
|---|---|---|---|
| 0–2 | `0x07` | `hop_limit` | current remaining hops |
| 3 | `0x08` | `want_ack` | |
| 4 | `0x10` | `via_mqtt` | |
| 5–7 | `0xE0` (shift 5) | `hop_start` | original hop_limit at TX time |

`HOP_MAX = 7`, `HOP_RELIABLE = 3` (default) — `src/mesh/MeshTypes.h:38,41`. A received
`hop_limit` above 7 gets clamped to `HOP_RELIABLE` and logged as a warning
(`RadioInterface.cpp:984-986`), not rejected outright.

Everything after these 16 bytes is either a serialized `Data` protobuf (`decoded` case,
sent in the clear — used for MQTT-uplinked or unencrypted debug traffic) or the AES
output (`encrypted` case, §7) — which one depends on the `MeshPacket.encrypted` vs.
`MeshPacket.decoded` oneof at the protobuf layer, not on anything in this 16-byte header.

## 6. Channel identity: name, hash, default PSK

`Channels::generateHash()` (`Channels.cpp:39-48`):

```
hash = xorHash(channelName) XOR xorHash(pskBytes)
```

where `xorHash(bytes)` is just `code ^= byte` over every byte (`Channels.cpp:27-33`) —
distinct from the djb2 hash in §4.

**`channelName`** (`Channels::getName()`, `Channels.cpp:358-372`): if the channel's
configured name is the empty string — which it is out of the box — the name used for
*both* hashing purposes (§4's djb2 and this XOR) is the **modem preset's display name**
instead (e.g. `"LongFast"`), not literally `""`. Easy place to get a silent mismatch if
this fallback isn't replicated exactly.

**`pskBytes`** (`Channels::getKey()`, `Channels.cpp:208-249`): the out-of-box default
channel has a 1-byte PSK, value `0x01`. A 1-byte PSK is an index: `0` disables
encryption entirely; any other value expands to the 16-byte `defaultpsk` constant with
its **last byte incremented by `(index - 1)`** — so index `1` is the unmodified
`defaultpsk`. A 2–15 byte PSK is zero-padded to 16 (AES-128); 17–31 bytes pads to 32
(AES-256); exactly 16 or 32 bytes is used as-is.

`defaultpsk` exact bytes (`Channels.h`, 16 bytes, AES-128 key for the public channel
every device ships listening on):

```
d4 f1 bb 3a 20 29 07 59 f0 bc ff ab cf 4e 69 01
```

So: **out-of-the-box interop = channel name `"LongFast"`, PSK = `defaultpsk` verbatim,
channel-hash byte = `xorHash("LongFast") XOR xorHash(defaultpsk)`** — computable and
worth hardcoding as a named constant plus a unit-testable derivation, not a magic byte.

## 7. Encryption

**In scope:** AES (CTR mode) legacy channel encryption — this is what a broadcast text
message on a shared channel uses.
**Out of scope (per the plan):** AES-CCM + Curve25519 PKI, used only for direct messages
between two specific nodes (`encryptCurve25519`/`aes_ccm_ae`, gated by
`MESHTASTIC_EXCLUDE_PKI`) — not needed for broadcast text.

Key size: 16 bytes (AES-128, what the default channel actually uses) or 32 (AES-256,
for a custom channel with a full-length PSK) — both go through the same CTR code path,
just a different key length passed to the AES engine.

Nonce construction, `CryptoEngine::initNonce()` (`CryptoEngine.cpp:263-271`), 16 bytes:

```
nonce[0:8]  = packetId, widened to uint64_t, little-endian, upper 4 bytes zero
                (unless extraNonce is set — see below)
nonce[8:12] = fromNode (uint32_t, little-endian)
nonce[12:16] = 0
if extraNonce != 0:
    nonce[4:8] = extraNonce (little-endian)   -- overwrites the zero upper half
                                                   of the widened packetId above
```

For a plain broadcast text message (no `extraNonce`), that's just: bytes 0-3 = packet
id (LE), bytes 4-11 = zero/fromNode as laid out above, bytes 12-15 = zero. Worth writing
as an explicit unit test against a known (packetId, fromNode) pair before trusting it
against real hardware.

## 8. Protobuf messages needed

Only two messages matter for broadcast text; both from `meshtastic/protobufs` at the
pinned commit above. Field numbers matter — this is wire format, not just a struct:

**`Data`** (`meshtastic/mesh.proto`) — this is what gets AES-encrypted into
`MeshPacket.encrypted`:

| Field | # | Type |
|---|---|---|
| `portnum` | 1 | `PortNum` enum — `TEXT_MESSAGE_APP = 1` (`meshtastic/portnums.proto:40`) |
| `payload` | 2 | `bytes` — for `TEXT_MESSAGE_APP`, raw UTF-8 text, not further wrapped |
| `want_response` | 3 | `bool` |
| `dest` | 4 | `fixed32` |
| `source` | 5 | `fixed32` |
| `request_id` | 6 | `fixed32` |
| `reply_id` | 7 | `fixed32` |
| `emoji` | 8 | `fixed32` |
| `bitfield` | 9 | `optional uint32` |

**`MeshPacket`** (`meshtastic/mesh.proto`) — the framing this project actually needs to
build (the `to`/`from`/`id`/`hop_limit`/etc. here are the protobuf-layer mirrors of the
same values that also get packed into the 16-byte `PacketHeader` in §5 — the two are
redundant on purpose, since only the on-air `PacketHeader` bytes are what real hardware
parses; `MeshPacket` is the in-RAM/API representation):

| Field | # | Type |
|---|---|---|
| `from` / `to` | 1 / 2 | `fixed32` |
| `channel` | 3 | `uint32` — **not the same as the header's channel-hash byte**; this is a local channel index, only meaningful inside one device |
| `decoded` / `encrypted` | 4 / 5 | `oneof payload_variant`: `Data` message, or raw `bytes` (max 233 bytes per `DATA_PAYLOAD_LEN` — this bounds message length) |
| `id` | 6 | `fixed32` |
| `hop_limit` | 9 | `uint32` |
| `want_ack` | 10 | `bool` |
| `via_mqtt` | 14 | `bool` |
| `hop_start` | 15 | `uint32` |
| `next_hop` / `relay_node` | 18 / 19 | `uint32` |

Everything else on `MeshPacket` (`priority`, `rx_snr`, `rx_rssi`, `public_key`,
`pki_encrypted`, `delayed`, `transport_mechanism`) belongs to the PKI/priority-queue/API
machinery this project isn't implementing — safe to ignore for Phase 2/3.

## 9. Routing (forward reference for Phase 3 — captured now since the source was already open, not yet spec-locked to the same rigor as §1-8)

- **Dedupe**: `PacketHistory` (`src/mesh/PacketHistory.cpp`) keys on `(sender NodeNum,
  packet id)`, one fixed-size ring of `PacketRecord` structs (`sender`, `id`,
  `rxTimeMsec`, `next_hop`, `hop_limit`, and up to `NUM_RELAYERS` node bytes that have
  already relayed this packet). `wasSeenRecently()` is the check-and-insert entry point.
- **Rebroadcast delay**: not a fixed jitter — `RadioInterface::getTxDelayMsecWeighted()`
  (`RadioInterface.cpp:619`) picks a random backoff sized by a contention window
  (`CWmin`/`CWmax`) that's itself scaled by measured channel utilization and, for a
  weighted variant, by the received SNR of the packet being relayed — better SNR receipt
  implies a shorter delay, so the "best placed" relay tends to go first.
- Needs its own full pass (exact `CWmin`/`CWmax` values, `NUM_RELAYERS`,
  `computeSlotTimeMsec()`) before Phase 3 implementation starts — flagged here so it
  isn't forgotten, not because it's already spec-locked to the level §1-8 are.

## 10. Test plan tie-in

Cheapest-to-most-expensive verification steps, matching the Phase 1/2 exit criteria in
`project_state.md`:

1. Compute the expected `LONG_FAST`/default-channel frequency for the chosen region by
   hand from §4, compare against a HiLetgo V3's own boot log (`LOG_INFO("frequency:
   ...")`) — zero code needed, pure arithmetic check.
2. Phase 1: transmit a fixed-content frame at that frequency/preamble/sync-word/CRC
   combo from this firmware; confirm the HiLetgo V3 logs *any* reception (even a CRC-valid,
   header-nonsense "bad packet") — proves PHY parity before framing/crypto are attempted.
3. Phase 2: build the real `PacketHeader` + encrypted `Data` payload per §5-8, confirm a
   `TEXT_MESSAGE_APP` message decodes correctly on the HiLetgo V3's serial console.
