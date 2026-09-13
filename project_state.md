# Project State — HASviolet ESP32

_Last reviewed: 2026-09-13 — the SX127x multi-board work below is merged to `main` (via `ESP32-support`); the SX126x/RadioLib section reflects the `Radiolib` branch, complete and pushed but not yet merged. A `mesh` branch has since been cut off `main` for Meshtastic-interop work (see "Meshtastic interop plan" below): Phase 0 (spec) and Phase 1 (PHY parity) are code-complete and pushed, with real hardware bring-up on a HiLetgo V3 done and documented below — Phase 1's actual on-air test (against a second stock-Meshtastic node) is still pending, and none of this is merged to `main`._

## What this is

HASviolet ESP32 is a work-in-progress firmware for an ESP32 board that acts as a **LoRa transceiver node** with a **web/WebSocket UI**, built to have UI/UX parity with the sister project [HASviolet on RPi](https://github.com/hudsonvalleydigitalnetwork/hasviolet). It targets amateur-radio-style LoRa messaging: browser client connects over WiFi, sends/receives text over a WebSocket session, and the ESP32 bridges that to LoRa TX/RX.

Built with **PlatformIO** (Arduino framework). Author develops in VS Code + PlatformIO extension on Linux. `main` builds for nine SX127x boards; the `Radiolib` branch adds four more on SX1262, for **13 supported boards total** across the two.

No SSL/TLS and no user authentication are implemented yet on the ESP32 side (unlike the RPi version) — called out explicitly in the README as a known gap to be addressed later.

## Architecture

Three conceptual parts, one physical device:

- **Server** — ESP32 running Web + WebSocket services on Core 1, LoRa comms on Core 0 (FreeRTOS task `HasTRX` pinned to core 0 via `xTaskCreatePinnedToCore`). This requires a dual-core chip — see the SX126x/C3/C6 note below.
- **Radio** — either SX1276-class (Heltec library, or `sandeepmistry/LoRa` + per-board pins for TTGO/T-Beam) or, on `Radiolib`-branch boards, SX1262-class via RadioLib and a `RadioLibSX126x` adapter — all behind a shared `hvLoRa` macro in `main.cpp` so the rest of the file doesn't care which.
- **Client** — Static files served from SPIFFS: `hasVIOLET_INDEX.html` → loads `hasVIOLET.css` + `hasVIOLET.js` → JS opens a WebSocket to the device and drives the whole UI (channel/radio settings, TX/RX text, macros, CMDline).

Boot sequence (from `setup()` in [src/main.cpp](src/main.cpp)) prints numbered INIT stages: 000 core start → 100 SPIFFS → 200 JSON config load → 300 WiFi (STA, falls back to self-hosted AP) → 400 web server → 500 WebSockets → 600 OLED. HasTRX (LoRa) task then starts on core 0; `loop()` on core 1 just pumps `webSocket.loop()`.

Communication protocol between client and server is a simple string-command scheme (`SET:...`, `GET:...`, `TX:`, `RX:`, acks prefixed `ACK:`) — see `development/defines/HASviolet_cmdmsgs.h` for the (currently unused/reference) canonical list.

## Repo layout

- [src/main.cpp](src/main.cpp) — all firmware logic (978 lines as of this branch, up from 896 pre-`mesh`; the previously-recorded "517 lines" here was already stale before this branch's own growth): WiFi/web/websocket init, LoRa send/receive, JSON config loading, OLED logo/display.
- [src/HASviolet_config.h](src/HASviolet_config.h) — compile-time secrets/config: WiFi AP SSID/key, STA SSID/key, web user/pass. **Currently committed with real-looking placeholder credentials in plaintext** (`WIFI_SSID "HomeWAN"`, a phone-number-shaped WiFi key, `WWW_USER/WWW_KEY = radio/radio`) — worth confirming these are dummy values before any public release.
- [src/HVDN_logo.h](src/HVDN_logo.h) — compiled bitmap logo for OLED splash.
- [data/](data) — SPIFFS image contents actually served by the device: `hasVIOLET_INDEX.html`, `.css`, `.js`, `.json` (channel/contact/macro config), `favicon.ico`, plus a `.crt`/`.key` pair (present but unused — no TLS wired up in `main.cpp` yet, consistent with the README's stated gap).
- [development/](development) — **not part of the active build**; holds two alternate/parallel dashboard UI trees (`dashboard_v1`, `dashboard_v2`) and a `defines/` folder with `HASviolet_boards.h` (pin maps for TTGO LoRa v1/v2, TTGO T-Beam, Heltec), `HASviolet_channels.h` (channel presets HV0–HV5+ with frequency/modem), and `HASviolet_cmdmsgs.h` (protocol command constants). These look like a staging area for multi-board support and a v2 UI that hasn't been merged into `src`/`data` yet.
- [platformio.ini](platformio.ini) — 13 `[env:...]` sections on `Radiolib` (9 on `main`), one per supported board (see "Board support" below); board choice lives entirely in build flags, `src/main.cpp` has no per-board source. Default env (`pio run` with no `-e`) is still `heltec_wifi_lora_32_V2` for back-compat. `mesh` adds a 14th, temporary `heltec_wifi_lora_32_v3_meshtest` env for Phase 1 bring-up — not a supported end-user target, see the Meshtastic interop plan below.
- [releases/](releases) — prebuilt `.bin` firmware + SPIFFS image for the Heltec board, for users who just want to flash without building.
- [docs/](docs) — an ODT user guide plus screenshots referenced from the README.
- [include/](include), [lib/](lib), [test/](test) — stock PlatformIO scaffolding, effectively empty (just README placeholders).
- `.pio/` — local build cache (gitignored except it's currently present on disk; also present under `.vscode/*` generated files, all gitignored per [.gitignore](.gitignore)).

## Recent git history (notable)

Last 6 commits on `main`:
```
777f559 Revert "INIT"
5d49238 Revert "INIT"
8a89671 INIT
4dea493 INIT
80b6f98 Update README
```
The two `INIT` commits attempted a project rename/rebrand to **"SIGnora"** (renaming `hasVIOLET.*` client files, `HASviolet_*.h` headers, and README text to SIGnora, and deleting the prebuilt release binaries). Both were fully reverted immediately after, so `main` was back to the pre-rename **HASviolet** state with the release binaries restored before any of the work below started.

Since then: `ESP32-support` (README fixes, `project_state.md`, `RELEASE-HISTORY.md`, the 9-board SX127x work) has been merged to `main`. `Radiolib` (the 4-board SX126x work) is pushed but not yet merged — see "Suggested next steps."

## Board support — SX127x boards (`main`, via `ESP32-support`)

`platformio.ini` has nine build environments for these, all verified with a real `pio run -e <env>` against the espressif32 toolchain (not just written and assumed):

| Env | Board | Chip | Radio path |
|---|---|---|---|
| `heltec_wifi_lora_32_v1` | Heltec WiFi LoRa 32 V1 | ESP32 | Heltec library |
| `heltec_wifi_lora_32_V2` | Heltec WiFi LoRa 32 V2 (default) | ESP32 | Heltec library |
| `heltec_wifi_lora_32_v3` | Heltec WiFi LoRa 32 V3 | ESP32-S3 | Heltec library |
| `heltec_wireless_stick` | Heltec Wireless Stick | ESP32 | Heltec library |
| `heltec_wireless_stick_lite` | Heltec Wireless Stick Lite (no OLED) | ESP32 | Heltec library |
| `ttgo_lora32_v1` | LILYGO/TTGO LoRa32 V1 | ESP32 | generic (`sandeepmistry/LoRa`) |
| `ttgo_lora32_v2` | LILYGO/TTGO LoRa32 V2 | ESP32 | generic |
| `ttgo_lora32_v21` | LILYGO/TTGO LoRa32 V2.1 | ESP32 | generic |
| `ttgo_tbeam` | LILYGO/TTGO T-Beam v1.1 | ESP32 | generic + AXP192 PMU init |

All of these use an SX1276/SX1277-class radio -- **except `heltec_wifi_lora_32_v3`, which does not**: confirmed on real hardware (on the `mesh` branch, not yet ported here) that its actual chip is an SX1262, and the "Heltec library" radio path in this table's row for it doesn't just fail on that hardware, it permanently hangs the CPU. See the `mesh` branch's Meshtastic interop plan, Phase 1 hardware findings, for the fix (also unrelated to Meshtastic work and worth porting to `main` on its own). Board selection is entirely via `-D<BOARD>` build flags; `src/main.cpp` dispatches on those through a shared `HASV_HELTEC_BOARD`/`hvLoRa` abstraction rather than per-board branches scattered through the file.

Deliberately **not** added: **RAK11200** (WisBlock module — pin map depends on which base-board slot the LoRa module sits in; needs sourcing RAK's own schematics before trusting it) and a Heltec "V2.1" (doesn't exist as distinct hardware from V2 — that naming belongs to TTGO's V2.1, not Heltec's; confirmed against Meshtastic's own repo, where `heltec_v2` and `heltec_v2.1` both build on the identical `heltec_wifi_lora_32_V2` PlatformIO board id).

**Checked against Meshtastic's actual `variants/` directory** (not just device names — each variant's `board =` line): 8 of these 9 map to a real Meshtastic-supported device (`heltec_v1`/`v2`/`v3`, `heltec_wsl_v2.1`, `tlora_v1`/`v1_3`/`v2`, `tlora_v2_1_16`/`_18`/`_tcxo`, `tbeam`). Two things worth knowing:
- **`heltec_wireless_stick`** (plain, non-Lite) is **not** currently a Meshtastic-supported device — only the Lite variant is. Still useful to this project, just not a Meshtastic overlap.
- **`ttgo_lora32_v2`** uses a distinct PlatformIO board id (`ttgo-lora32-v2`) that no current Meshtastic variant builds against — their "tlora_v2" naming actually builds on the `ttgo-lora32-v1` profile. Same hardware family, not a separate Meshtastic build target.

## SX126x radio support (RadioLib) — complete on `Radiolib` branch

Meshtastic's own hardware docs and current buying guides converge on a set of "community favorite" ESP32-S3 boards this project didn't cover, because they all use the **SX1262/SX1268** radio rather than SX1276/SX1277: LILYGO T3-S3, T-Beam Supreme (T-Beam S3-Core), B&Q Station G2, Heltec Wireless Tracker. (T-Deck/T-Deck Plus fit this radio family too but are intentionally excluded — their keyboard/screen add nothing to HASviolet's browser-driven UI model.) All four are now supported.

### Done: LILYGO T3-S3 (`env:lilygo_t3_s3`)

Landed and build-verified. The approach turned out cleaner than originally scoped:

- Added [RadioLib](https://github.com/jgromes/RadioLib) (`jgromes/RadioLib`) as the SX126x backend.
- Rather than rewriting `HasTRX`/`sendLORA`/`onReceiveLORA`/the `GET:LORA` websocket handler for a second radio API, `main.cpp` gained a `RadioLibSX126x` adapter class that implements the *exact same method names* (`setSyncWord`, `disableCrc`, `setFrequency`, `setTxPower`, `setSignalBandwidth`, `setSpreadingFactor`, `setCodingRate4`, `receive`, `parsePacket`, `read`, `packetRssi`, `beginPacket`, `write`, `print`, `endPacket`, `dumpRegisters`) as the old SX127x `LoRaClass` — so `hvLoRa` just points at this adapter instead of `LoRa`/`Heltec.LoRa` on this board, and every one of those four call sites is untouched. `parsePacket()` is non-blocking via a DIO1-triggered ISR flag, matching the existing polling-loop control flow exactly.
- `-D<BOARD>` pins are hand-sourced from Meshtastic's own shipping variant file (`variants/esp32s3/tlora_t3s3_v1/variant.h` in `meshtastic/firmware`), since PlatformIO's `lilygo-t3-s3` board id maps to the generic `esp32s3` variant with no board-specific `pins_arduino.h` to pull from.
- All API calls (`Module` constructor arg order, `begin()`/`setSyncWord()`/`setCRC()`/etc. signatures) were checked against the actual installed RadioLib 7.7.1 headers, not assumed from memory.
- One toolchain gotcha worth remembering: an `IRAM_ATTR` static method defined *inline inside* the class body trips an Xtensa "literal placed after use" linker error — has to be declared in-class and defined out-of-line instead.

### Done: B&Q Station G2 (`env:bq_station_g2`)

Also landed and build-verified. Radio side reused the T3-S3 work with zero new code — same `RadioLibSX126x` adapter, just different `-D` pins (SCK=12, MISO=14, MOSI=13, CS=11, RESET=21, DIO1=48, BUSY=47), sourced from Meshtastic's shared `variants/esp32s3/station-common/station_common.h` (Station G2 and G3 build on the same file). Also added a max-power clamp (`SX126X_MAX_POWER`) inside the adapter's `setTxPower()`, since G2's real hardware tops out at 19 dBm — a `#ifdef`, no-op on boards that don't define it.

The OLED did need new code: G2's 1.3" display is an **SH1107** (Adafruit_SH110X), not the SSD1306 every other board here uses. Added a `SH1107OLEDAdapter` — same pattern as the radio adapter, applied to the display: it implements the handful of calls `OLEDme()`/`logo()`/`initOLED()` actually make (`init`, `flipScreenVertically`, `clear`, `drawString`, `display`, `drawXbm`) on top of `Adafruit_SH110X`, so those shared functions didn't need touching either. `setFont`/`setTextAlignment` are no-ops on this adapter since the code only ever asks for one font and left-alignment; two small stand-in constants (`TEXT_ALIGN_LEFT`, `ArialMT_Plain_10`) replace the ones that normally come from the SSD1306 library this board doesn't link.

No dedicated PlatformIO board id exists for Station G2, so it builds on the generic `esp32-s3-devkitc-1` profile rather than borrowing another vendor's board identity.

### Done: T-Beam Supreme / T-Beam S3-Core (`env:tbeam_supreme`)

Radio side reused `RadioLibSX126x` again with just new pins (SCK=12, MISO=13, MOSI=11, CS=10, RESET=5, DIO1=1, BUSY=4) and OLED reused the SH110X adapter template with `Adafruit_SH1106G` (this board's display is SH1106, not SH1107).

The real addition was the PMU: this board uses an **AXP2101**, a different chip from the AXP192 the original T-Beam v1.1 uses, needing a different library (`lewisxhe/XPowersLib`) and its own `initPMU()`. One non-obvious wrinkle: XPowersLib's concrete `XPowersAXP2101` class keeps `setPowerChannelVoltage()`/`enablePowerOutput()` **protected** — they're only public on its `XPowersLibInterface` base, so `PMU` has to be declared as that interface type (a pointer), matching exactly how Meshtastic's own `src/Power.cpp` uses this library. Rail assignments (ALDO1 for sensors/OLED/RTC, ALDO2 as a required baseline rail, ALDO3 for the LoRa radio) and the fact that the PMU lives on the ESP32's second I2C bus (`Wire1`, shared with an onboard PCF8563 RTC) both came straight from Meshtastic's own `LILYGO_TBEAM_S3_CORE` branch in `Power.cpp` — GNSS, the M.2 slot, and the SD card rails were left off since nothing here uses them.

### Done: Heltec Wireless Tracker (`env:heltec_wireless_tracker`)

Radio side, again, is just `RadioLibSX126x` with new pins (SCK=9, MISO=11, MOSI=10, CS=8, RESET=12, DIO1=14, BUSY=13).

This board has **no OLED at all** — its display is a color **ST7735 TFT** on its own dedicated SPI bus, separate from the LoRa radio's. Added an `ST7735TFTAdapter` following the same pattern as the OLED adapters (`Adafruit ST7735 and ST7789 Library`), with one real difference: an ST7735 draws each primitive straight to the panel rather than buffering in RAM, so its `display()` is a no-op. Also needs a `VEXT_ENABLE` GPIO driven high before the panel (and GPS, unused here) will power on — a much simpler version of the PMU-gating T-Beam boards need, handled inline in the adapter's `init()`.

**Caveat worth flagging:** the panel geometry (`INITR_MINI160x80`, landscape rotation) is inferred from Meshtastic's own `TFT_WIDTH`/`HEIGHT`/`OFFSET_X` defines for this exact board, not confirmed against real hardware — if the image comes up offset or mirrored, that init call is the first place to look.

**Effort signal, in retrospect:** three of the four boards (T3-S3, Station G2, T-Beam Supreme) turned into config-and-adapter-reuse exercises once the `hvLoRa`/`oledDisplay` seams existed — even the "different chip" pieces (SH1106 vs SH1107, AXP2101 vs AXP192) were a few dozen lines each, not new architecture. Wireless Tracker's TFT was the only genuinely new *kind* of adapter. As with every board in this repo, none of this can be fully confirmed correct (BUSY-line timing, TCXO wiring, PMU register behavior, TFT geometry) without the actual hardware in hand — everything here is build-verified, not hardware-verified.

## Known issues / gaps (from code + README, not fixed by anyone yet)

- No TLS and no user authentication on the ESP32 web/WebSocket services, despite `data/hasVIOLET.crt`/`.key` and `WWW_USER`/`WWW_KEY` existing — explicitly flagged by the author as deferred.
- Plaintext WiFi/web credentials committed in `src/HASviolet_config.h`.
- `development/` trees (dashboard_v1/v2, multi-board defines, protocol header) are disconnected from the active `src/`+`data/` build — unclear which, if any, is meant to land next. The old `development/defines/HASviolet_boards.h` pin tables are now superseded by `platformio.ini` + arduino-esp32's own variant pins for the boards on this branch, but the file itself wasn't touched.
- README build instructions reference a mistyped/broken clone command (`git clone https://https://github.com/.../hasviolet-esp32.git`) — fixed on `main` separately from this branch.
- `WiFi.softAP()` rejects `HASviolet_config.h`'s default AP passphrase (`"purple"`, 6 characters — WPA2-PSK needs 8+); confirmed via serial on real hardware on the `mesh` branch (Phase 1 hardware findings below), not yet fixed anywhere, affects `main`/`Radiolib` too.

## Meshtastic interop plan (`mesh` branch — Phase 0 and Phase 1 code landed, not yet merged to `main`)

Goal, as scoped with the author: the board should become a **real node on the Meshtastic mesh** — same LoRa packet format, encryption, and flood-routing as the official firmware, able to send/receive with actual Meshtastic hardware over the air. Explicitly **out of scope**: Meshtastic's phone/companion-app surface (BLE/serial/TCP device API, admin protobufs) — HASviolet's own web UI remains the control surface, with custom client apps to follow later. This is additive: existing HASviolet classic mode (current wire format on `main`/`Radiolib`) stays intact as the default, with mesh mode a switchable option, per the original ask.

**Test hardware:** a pair of Heltec HiLetgo ESP32 LoRa V3 boards, 433–510MHz band variant. Real over-the-air verification against this pair (and ideally one further node running stock Meshtastic firmware) is required at every phase below — unlike the SX126x board bring-up work, which shipped "build-verified, not hardware-verified," a framing/crypto/PHY mismatch here produces silence, not a compile error or a garbled screen, so it cannot be self-verified from PlatformIO builds alone.

Four wire-format layers have to match real Meshtastic nodes exactly, sourced from a **pinned commit** of `meshtastic/firmware`/`meshtastic/protobufs` (not memory or blog posts), the same discipline already used for this project's board pin tables:
1. **PHY** — frequency, bandwidth, spreading factor, coding rate, preamble length (16, not this project's current default), explicit sync word (not HASviolet's hardcoded `0xFF`), and hardware CRC **on** (HASviolet currently calls `hvLoRa.disableCrc()`).
2. **Framing** — Meshtastic's fixed binary `PacketHeader` (to/from/packet-id/hop-limit/channel-hash/next-hop/relay-node) in front of an encrypted payload.
3. **Encryption** — AES-256-CTR keyed by channel PSK (well-known default key for the public "LongFast" channel), nonce from packet id + sender, plus a channel-hash byte.
4. **Routing** — hop-limit decrement + rebroadcast, a recently-seen-packet-ID dedupe cache, randomized pre-rebroadcast delay.

### Roadmap

| Phase | Goal | Exit criteria |
|---|---|---|
| 0 — Spec lock | Pull `PacketHeader`, `Channels.cpp` hash algorithm, `CryptoEngine.cpp`, and region/frequency-slot logic from a pinned `meshtastic/firmware` commit. Decide GPL-3.0 posture (see below). | **Done** — [development/meshtastic/MESHTASTIC-SPEC.md](development/meshtastic/MESHTASTIC-SPEC.md), sourced from `meshtastic/firmware`@`54e0d8d0` (tag `v2.7.26.54e0d8d`) and the matching `meshtastic/protobufs` submodule pin, downloaded and grepped directly rather than taken from docs/memory. License posture also now decided (permissive, not GPL-3.0 — see below), which locks in the doc's "write from spec, don't copy Meshtastic's source" stance permanently. |
| 1 — PHY parity | **Code done, build-verified, and boots clean on real hardware.** On-air test (needs a second unit running stock Meshtastic) still pending. Turned out narrower in scope than planned: `setPreambleLength()`/`enableCrc()`/`setSyncWord()` already exist natively on both `LoRaClass` (generic SX127x) and Heltec's bundled fork of it — so only `RadioLibSX126x` (this project's own adapter) needed the first two added. Full 27-region + 9-preset tables, the djb2 frequency-slot formula, and name-based lookups live in [src/Meshtastic_RadioConfig.h](src/Meshtastic_RadioConfig.h), included only under `-DMESHTASTIC_PHY_TEST` so classic-mode builds never see it. `HasTRX()` branches on that flag to use EU_433/LongFast PHY params computed via the header's own lookup code. New env `heltec_wifi_lora_32_v3_meshtest` (temporary, not a supported end-user target) plus the production `heltec_wifi_lora_32_v3`/`v1`/`V2` envs all build clean. Region: **`EU_433`** — the operator is a licensed US ham, so actual transmit authorization is their FCC Part 97 license on the 70cm band, not any region's ISM limits; there's no literal "US_433" Meshtastic region, and `EU_433` is the de facto convention for bench-testing 433MHz hardware regardless of locale. Test frequency **433.875MHz**, hand-computed in advance and then **confirmed matching exactly** in the real device's own boot log (see hardware findings below) — the formula transcription is correct. | A fixed test frame shows up in a real node's log — **pending**: needs a second HiLetgo V3 running stock Meshtastic (region `EU_433`, preset `Long Fast`) to confirm reception. |
| 2 — Framing + crypto | Vendor nanopb + generated Meshtastic protobuf sources (`mesh.proto`, `portnums.proto`, channel/config messages) into `lib/` (currently an empty placeholder); build `PacketHeader` encode/decode; implement channel hash + AES-256-CTR via mbedtls (already bundled in the ESP32 Arduino core — no new crypto dependency). | A `TEXT_MESSAGE_APP` message round-trips both directions with the real node on the public LongFast channel. |
| 3 — Routing | Hop-limit decrement, dedupe cache, randomized rebroadcast delay. | 3-node test (this device sandwiched between two real nodes, or the HiLetgo pair plus one stock-firmware node) relays correctly with no duplicate storms. |
| 4 — Presence | Periodic `NodeInfo`/`User` broadcast so the device shows a name instead of "Unknown" on real nodes. | Device appears by name in a real node's node list. |
| 5 — Config & UI | Extend `hasVIOLET.json` + web UI (`data/` — untouched by all prior board work, first feature to actually need it) with mode/region/preset/channel/PSK/node-name fields; mesh mode toggles at **runtime**, not a build flag (avoids doubling the existing 13-environment `platformio.ini` matrix). Region ships `UNSET` and must present the full region list to the user — no hardcoded regional default, this project has no way to know where a given device is deployed. | Mesh mode switches on/off at runtime without reflashing, classic HASviolet mode unaffected; radio refuses to transmit in mesh mode until the user has explicitly picked a region, matching stock Meshtastic firmware's own behavior. |
| 6 — Multi-board rollout | Exercise both radio families (RadioLib/SX126x and sandeepmistry/SX127x) on real hardware — they may differ subtly in preamble/CRC semantics even behind the shared `hvLoRa` adapter. | At least one board per radio family verified on-air (Heltec V3 covers the Heltec-library/SX127x path; still need an SX126x board on-air separately). |

### Phase 1 hardware findings — three real, pre-existing bugs found and fixed; one found and deferred

Flashing a real HiLetgo V3 for the first time (this project's whole board matrix had only ever been "build-verified, not hardware-verified" until now) surfaced actual defects, all unrelated to the Meshtastic work itself and all still present on `main`/`Radiolib`:

1. **`Class_Wifi_LoRa` case-mismatch crash (fixed)** — `heltecautomation/Heltec ESP32 Dev-Boards@^2.1.1`'s own `heltec.h` defines `Class_WIFI_LORA` (all-caps LORA) for `WIFI_LORA_32`/`_V2`/`_V3`, but `heltec.cpp`'s `Heltec_ESP32` constructor checks the differently-cased `Class_Wifi_LoRa` — since macros are case-sensitive, that check was never true, so `display` (an `SSD1306Wire*`) was never allocated even though `Heltec_Screen` (correctly using the right case) was defined. `Heltec.begin()` then called `display->init()` on a null pointer: `Guru Meditation Error: LoadProhibited`, confirmed via serial. Fixed with a `-DClass_Wifi_LoRa` build flag (has to be a build flag, not a `#define` in `main.cpp` — `heltec.cpp` is compiled as its own separate translation unit) on `heltec_wifi_lora_32_v1`/`V2`/`v3` (not Wireless Stick(Lite) — their branch already uses the correctly-cased raw `WIRELESS_STICK` macro).
2. **`heltec_wifi_lora_32_v3` was categorized as an SX127x/Heltec-radio board and is actually SX1262 (fixed)** — confirmed against Heltec's own product page and independent community reports: **WiFi LoRa 32 V3's real chip is an SX1262**, not the SX1276 Heltec's bundled `LoRa.h` driver was written for. Worse than just failing: `heltec.cpp` calls an unconditional `while(1);` when `LoRa.begin()` can't detect a recognized chip, permanently halting the CPU — this, not a crash, is what produced "blank screen, stuck since first connect." Fixed by splitting `HASV_HELTEC_BOARD` (display/Vext/serial via Heltec.begin(), still true for V3) from a new `HASV_HELTEC_RADIO_BOARD` (true only for boards whose chip Heltec's bundled driver actually matches — V1/V2/Wireless Stick(Lite), NOT V3) in `main.cpp`; V3 now also sets `HASV_SX126X_BOARD` and gets its radio from `RadioLibSX126x` like the other four SX126x boards, with `Heltec.begin()`'s `LoRaEnable` argument set to `false` on V3 specifically to skip its broken radio init entirely. Pins sourced from Meshtastic's own `variants/esp32s3/heltec_v3/variant.h` (same discipline as every other board's pins here). **Confirmed on real hardware**: full boot now completes, and the firmware's own serial log reports `433.875000` MHz — matching the hand computation above exactly.
3. **`initWiFi()`'s STA-connect loop had no timeout (fixed)** — `while(WiFi.status() != WL_CONNECTED) { delay(1000); }` had no bound at all; with `HASviolet_config.h`'s placeholder `WIFI_SSID "HomeWAN"` not present at the test bench, this spun forever and the AP-fallback code right after it was unreachable. The `WIFI_POLL_DELAY`/`WIFI_POLL_TRIES` constants already defined near the top of `main.cpp` look like they were meant to bound exactly this loop and were simply never wired up. Now bounded by those two constants, falling through to AP mode on timeout as the surrounding code already clearly intended.
4. **`softAP()` passphrase too short (found, not yet fixed)** — confirmed via serial: `[E][WiFiAP.cpp:147] softAP(): passphrase too short!`. `HASviolet_config.h`'s default `WIFI_APKEY "purple"` is 6 characters; WPA2-PSK requires 8-63. The AP still comes up, but may be falling back to open/unsecured rather than password-protected — not confirmed either way yet. Not blocking Phase 1, deferred rather than scope-creeping further into this session.

### Known risks / decisions still open

- ~~License: GPL-3.0 for the whole project~~ — **decided against**. HASviolet-ESP32's own code will carry a permissive (MIT-family) license: give credit, respect each bundled component's own license, nothing more. See [LICENSE.md](LICENSE.md) (component-by-component attribution, verified against the actual files under `.pio/libdeps/`, not assumed). Consequence for the Meshtastic work: the Phase 0 spec-doc's "write fresh from spec, don't copy Meshtastic's GPL-3.0 source" posture is now the permanent stance, not a placeholder — this project cannot absorb GPL-3.0 code from `meshtastic/firmware`/`protobufs` while staying MIT-family.
  - While investigating GPL-3.0 as an option, found and then **disproved** a suspected blocker: the Heltec ESP32 Dev-Boards library ships a precompiled, no-source-available `liblorawan.a` blob for a LoRaWAN stack this project never calls into. Initially flagged as a GPL Corresponding-Source problem for the 5 Heltec-library boards — but checking the actual linked `firmware.elf` (`nm` against a real build) shows **zero symbols from that blob make it into the shipped binary**: the linker drops an entire unreferenced static-library member rather than including it, since nothing in `heltec.cpp`/`main.cpp` calls into it. Worth having actually checked rather than asserted — moot either way now that the project isn't going GPL-3.0, but the verification method (check the real `.elf`, don't just read `library.properties`) is worth remembering next time a similar question comes up.
- **Duty-cycle/airtime limiting**: some regions (not the 433–510MHz test band, but relevant if this ever targets EU868) enforce a duty-cycle limiter in real Meshtastic firmware; skipping it works on a bench but is a bad-citizen move on a shared mesh. Same category of deferred gap as the TLS/auth item above.
- **Runtime vs. build-time mode switch**: leaning runtime (JSON config, like frequency/channel already are) specifically to avoid a 13→26 environment explosion in `platformio.ini`; not yet implemented or committed to in code.
- ~~Which Meshtastic region to claim~~ — **decided**: this firmware is distributed globally, so region is a **user-set runtime config field** (full enum from the spec doc §2, all 27 Meshtastic regions — not just the 7 overlapping the test hardware), never a hardcoded default picked by this project. Matches real Meshtastic firmware's own onboarding behavior: region ships as `UNSET` and the radio does not transmit until the user picks one — worth deliberately replicating that refuse-to-TX-until-configured behavior rather than silently defaulting to something. For the HiLetgo V3 bench-testing pair specifically, whoever runs the Phase 1/2 hardware tests still has to set *some* legal region on both boards to get a frequency to test against (see spec doc §2 for the 7 candidates in range) — that's a local test-setup choice, not the shipped default.

## Suggested next steps (not yet started)

1. Merge `Radiolib` into `main` (currently pushed, PR not yet opened).
2. Decide the fate of the `development/` dashboard v1/v2 trees — merge one into `data/`+`src/`, or document why both are kept.
3. Confirm `HASviolet_config.h` values are placeholders, or move real secrets out of version control (e.g., a gitignored local config).
4. Implement or explicitly schedule the TLS/auth work called out in the README.
5. Source real pin data for RAK11200 and add it to the board matrix.
6. Get any of the 13 boards on actual hardware to confirm what compiling alone can't: BUSY-line timing, TCXO/DIO1 wiring, PMU register behavior, and (Wireless Tracker specifically) TFT panel geometry. (Heltec V3 is now partially done — see the Meshtastic interop plan's Phase 1 hardware findings below, which surfaced real bugs, though the check there was prompted by mesh work rather than this item.)
7. Finish Phase 1's on-air test on the `mesh` branch (needs a second HiLetgo V3 running stock Meshtastic, region `EU_433`/preset `Long Fast`), then start Phase 2 (framing + encryption).
8. Fix the `softAP()` short-passphrase bug found during Phase 1 hardware bring-up (see below) — affects `main`/`Radiolib` too, independent of mesh work.
