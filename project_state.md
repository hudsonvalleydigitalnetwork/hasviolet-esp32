# Project State — HASviolet ESP32

_Last reviewed: 2026-09-13 — reflects the `ESP32-support` branch, not yet merged to `main`._

## What this is

HASviolet ESP32 is a work-in-progress firmware for an ESP32 board that acts as a **LoRa transceiver node** with a **web/WebSocket UI**, built to have UI/UX parity with the sister project [HASviolet on RPi](https://github.com/hudsonvalleydigitalnetwork/hasviolet). It targets amateur-radio-style LoRa messaging: browser client connects over WiFi, sends/receives text over a WebSocket session, and the ESP32 bridges that to LoRa TX/RX.

Built with **PlatformIO** (Arduino framework). Author develops in VS Code + PlatformIO extension on Linux. `main` only builds for the Heltec WiFi LoRa 32 V2; this branch expands that to nine boards (see below), all sharing SX127x-class radios — see the SX126x follow-up section for what's deliberately not covered yet.

No SSL/TLS and no user authentication are implemented yet on the ESP32 side (unlike the RPi version) — called out explicitly in the README as a known gap to be addressed later.

## Architecture

Three conceptual parts, one physical device:

- **Server** — ESP32 running Web + WebSocket services on Core 1, LoRa comms on Core 0 (FreeRTOS task `HasTRX` pinned to core 0 via `xTaskCreatePinnedToCore`). This requires a dual-core chip — see the SX126x/C3/C6 note below.
- **Radio** — SX1276-class LoRa module, driven either via the Heltec library (Heltec boards) or directly via `sandeepmistry/LoRa` + per-board pins (TTGO/T-Beam) behind a shared `hvLoRa` macro in `main.cpp`.
- **Client** — Static files served from SPIFFS: `hasVIOLET_INDEX.html` → loads `hasVIOLET.css` + `hasVIOLET.js` → JS opens a WebSocket to the device and drives the whole UI (channel/radio settings, TX/RX text, macros, CMDline).

Boot sequence (from `setup()` in [src/main.cpp](src/main.cpp)) prints numbered INIT stages: 000 core start → 100 SPIFFS → 200 JSON config load → 300 WiFi (STA, falls back to self-hosted AP) → 400 web server → 500 WebSockets → 600 OLED. HasTRX (LoRa) task then starts on core 0; `loop()` on core 1 just pumps `webSocket.loop()`.

Communication protocol between client and server is a simple string-command scheme (`SET:...`, `GET:...`, `TX:`, `RX:`, acks prefixed `ACK:`) — see `development/defines/HASviolet_cmdmsgs.h` for the (currently unused/reference) canonical list.

## Repo layout

- [src/main.cpp](src/main.cpp) — all firmware logic (517 lines): WiFi/web/websocket init, LoRa send/receive, JSON config loading, OLED logo/display.
- [src/HASviolet_config.h](src/HASviolet_config.h) — compile-time secrets/config: WiFi AP SSID/key, STA SSID/key, web user/pass. **Currently committed with real-looking placeholder credentials in plaintext** (`WIFI_SSID "HomeWAN"`, a phone-number-shaped WiFi key, `WWW_USER/WWW_KEY = radio/radio`) — worth confirming these are dummy values before any public release.
- [src/HVDN_logo.h](src/HVDN_logo.h) — compiled bitmap logo for OLED splash.
- [data/](data) — SPIFFS image contents actually served by the device: `hasVIOLET_INDEX.html`, `.css`, `.js`, `.json` (channel/contact/macro config), `favicon.ico`, plus a `.crt`/`.key` pair (present but unused — no TLS wired up in `main.cpp` yet, consistent with the README's stated gap).
- [development/](development) — **not part of the active build**; holds two alternate/parallel dashboard UI trees (`dashboard_v1`, `dashboard_v2`) and a `defines/` folder with `HASviolet_boards.h` (pin maps for TTGO LoRa v1/v2, TTGO T-Beam, Heltec), `HASviolet_channels.h` (channel presets HV0–HV5+ with frequency/modem), and `HASviolet_cmdmsgs.h` (protocol command constants). These look like a staging area for multi-board support and a v2 UI that hasn't been merged into `src`/`data` yet.
- [platformio.ini](platformio.ini) — nine `[env:...]` sections, one per supported board (see "Board support" below); board choice lives entirely in build flags, `src/main.cpp` has no per-board source. Default env (`pio run` with no `-e`) is still `heltec_wifi_lora_32_V2` for back-compat.
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
The two `INIT` commits attempted a project rename/rebrand to **"SIGnora"** (renaming `hasVIOLET.*` client files, `HASviolet_*.h` headers, and README text to SIGnora, and deleting the prebuilt release binaries). Both were fully reverted immediately after, so `main` is currently back to the pre-rename **HASviolet** state with the release binaries restored. Working tree is clean; nothing in flight on this branch beyond that revert.

## Board support (this branch)

`platformio.ini` now has nine build environments, all verified with a real `pio run -e <env>` against the espressif32 toolchain (not just written and assumed):

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

All of these use an SX1276/SX1277-class radio. Board selection is entirely via `-D<BOARD>` build flags; `src/main.cpp` dispatches on those through a shared `HASV_HELTEC_BOARD`/`hvLoRa` abstraction rather than per-board branches scattered through the file.

Deliberately **not** added: **RAK11200** (WisBlock module — pin map depends on which base-board slot the LoRa module sits in; needs sourcing RAK's own schematics before trusting it) and a Heltec "V2.1" (doesn't exist as distinct hardware from V2 — that naming belongs to TTGO's V2.1, not Heltec's).

## Planned follow-up: SX126x radio support (RadioLib)

Not started. Meshtastic's own hardware docs and current buying guides converge on a set of "community favorite" ESP32-S3 boards that this branch does *not* cover, because they all use the **SX1262/SX1268** radio rather than SX1276/SX1277:

- LILYGO T3-S3
- T-Beam Supreme (T-Beam S3-Core) — successor to the T-Beam v1.1 already supported
- B&Q Station G2
- Heltec Wireless Tracker

(T-Deck/T-Deck Plus also fit this radio family but are intentionally excluded from the target list below — their keyboard/screen add nothing to HASviolet's browser-driven UI model.)

**Why this is a bigger lift than the boards above:** both `sandeepmistry/LoRa` and the Heltec library's bundled LoRa fork only implement the SX127x register map (direct register peek/poke, DIO0 interrupt). SX126x chips speak a completely different command-based SPI protocol, need the driver to wait on a BUSY line, and interrupt on DIO1 instead of DIO0 — none of that is a drop-in swap.

**Proposed approach:**
1. Bring in [RadioLib](https://github.com/jgromes/RadioLib) (`jgromes/RadioLib`) as the radio backend for these boards — it supports SX127x *and* SX126x/SX128x under one API, so it's also a plausible path to eventually retiring the two existing radio backends in favor of one.
2. `main.cpp` already centralizes every radio call behind the `hvLoRa` macro/four call sites (`HasTRX`, `sendLORA`, `onReceiveLORA`, the `GET:LORA` websocket handler) — RadioLib's API (a `Module` bound to NSS/DIO1/RST/BUSY, `transmit()`/`readData()` instead of `beginPacket()`/`parsePacket()`) is different enough that this becomes a second implementation behind that same seam, not a tweak to the existing one.
3. New pin sets needed per board: NSS, DIO1, RST, BUSY (no DIO0 on SX126x). Where the board's arduino-esp32 variant already defines these (as it did for the TTGO boards above), reuse them; T-Beam Supreme and T3-S3 are newer than what's cached in this environment's arduino-esp32 core and may need pins sourced from LILYGO's schematics by hand.
4. T-Beam Supreme and Wireless Tracker also carry GPS (and Supreme a BME280 sensor) on shared buses — out of scope for a radio-only pass, but worth flagging for whoever eventually wants telemetry.

**Effort signal:** the boards already added were a config-and-pin-map exercise catchable entirely by `pio run` (~1 day). This is a new radio driver layer whose correctness — BUSY-line timing, TCXO/DIO1 wiring per board — can't be fully confirmed by compiling alone; realistically needs the actual hardware in hand.

## Known issues / gaps (from code + README, not fixed by anyone yet)

- No TLS and no user authentication on the ESP32 web/WebSocket services, despite `data/hasVIOLET.crt`/`.key` and `WWW_USER`/`WWW_KEY` existing — explicitly flagged by the author as deferred.
- Plaintext WiFi/web credentials committed in `src/HASviolet_config.h`.
- `development/` trees (dashboard_v1/v2, multi-board defines, protocol header) are disconnected from the active `src/`+`data/` build — unclear which, if any, is meant to land next. The old `development/defines/HASviolet_boards.h` pin tables are now superseded by `platformio.ini` + arduino-esp32's own variant pins for the boards on this branch, but the file itself wasn't touched.
- README build instructions reference a mistyped/broken clone command (`git clone https://https://github.com/.../hasviolet-esp32.git`) — fixed on `main` separately from this branch.

## Suggested next steps (not yet started)

1. Decide the fate of the `development/` dashboard v1/v2 trees — merge one into `data/`+`src/`, or document why both are kept.
2. Confirm `HASviolet_config.h` values are placeholders, or move real secrets out of version control (e.g., a gitignored local config).
3. Implement or explicitly schedule the TLS/auth work called out in the README.
4. Scope and implement the SX126x/RadioLib follow-up above.
5. Source real pin data for RAK11200 and add it to the board matrix.
