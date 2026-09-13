# Project State — HASviolet ESP32

_Last reviewed: 2026-09-13 — the multi-board work below is merged to `main`; the SX126x/RadioLib section reflects the `Radiolib` branch, in progress._

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

## SX126x radio support (RadioLib) — in progress on `Radiolib` branch

Meshtastic's own hardware docs and current buying guides converge on a set of "community favorite" ESP32-S3 boards this project didn't cover, because they all use the **SX1262/SX1268** radio rather than SX1276/SX1277: LILYGO T3-S3, T-Beam Supreme (T-Beam S3-Core), B&Q Station G2, Heltec Wireless Tracker. (T-Deck/T-Deck Plus fit this radio family too but are intentionally excluded — their keyboard/screen add nothing to HASviolet's browser-driven UI model.)

### Done: LILYGO T3-S3 (`env:lilygo_t3_s3`)

Landed and build-verified. The approach turned out cleaner than originally scoped:

- Added [RadioLib](https://github.com/jgromes/RadioLib) (`jgromes/RadioLib`) as the SX126x backend.
- Rather than rewriting `HasTRX`/`sendLORA`/`onReceiveLORA`/the `GET:LORA` websocket handler for a second radio API, `main.cpp` gained a `RadioLibSX126x` adapter class that implements the *exact same method names* (`setSyncWord`, `disableCrc`, `setFrequency`, `setTxPower`, `setSignalBandwidth`, `setSpreadingFactor`, `setCodingRate4`, `receive`, `parsePacket`, `read`, `packetRssi`, `beginPacket`, `write`, `print`, `endPacket`, `dumpRegisters`) as the old SX127x `LoRaClass` — so `hvLoRa` just points at this adapter instead of `LoRa`/`Heltec.LoRa` on this board, and every one of those four call sites is untouched. `parsePacket()` is non-blocking via a DIO1-triggered ISR flag, matching the existing polling-loop control flow exactly.
- `-D<BOARD>` pins are hand-sourced from Meshtastic's own shipping variant file (`variants/esp32s3/tlora_t3s3_v1/variant.h` in `meshtastic/firmware`), since PlatformIO's `lilygo-t3-s3` board id maps to the generic `esp32s3` variant with no board-specific `pins_arduino.h` to pull from.
- All API calls (`Module` constructor arg order, `begin()`/`setSyncWord()`/`setCRC()`/etc. signatures) were checked against the actual installed RadioLib 7.7.1 headers, not assumed from memory.
- One toolchain gotcha worth remembering: an `IRAM_ATTR` static method defined *inline inside* the class body trips an Xtensa "literal placed after use" linker error — has to be declared in-class and defined out-of-line instead.

### Remaining three: bigger than "swap the radio," each in its own way

Pulled real pin/config data from `meshtastic/firmware` for all three before writing anything, and each turned out to need more than RadioLib alone:

- **T-Beam Supreme (`tbeam-s3-core`)** — uses an **AXP2101** PMU, not the AXP192 the existing T-Beam v1.1 support uses. That's a different chip and a different library (`lewisxhe/XPowersLib`, not `AXP202X_Library`), plus a PCF8563 RTC sharing a second I2C bus (`Wire1`). SX1262 pins themselves (CS=10, DIO1=1, BUSY=4, RESET=5) are simple enough.
- **Station G2** — its `variant.h` is just `#include "station_common.h"`; the real pin definitions live in that shared header, not yet pulled.
- **Heltec Wireless Tracker** — has **no OLED**; its display is an ST7735S TFT over a dedicated SPI bus, which `oledDisplay`/`SSD1306Wire` can't drive. Also has GPS and a Vext power-rail enable pin similar in spirit to T-Beam's PMU gating.

None of these are just "add a `RadioLibSX126x` instance with different pins" the way T3-S3 was — each needs its own small chunk of new code (a PMU driver swap, sourcing a shared header, or a TFT display path) before the radio part even comes into play.

**Effort signal going forward:** T3-S3 was closer to the original "config + pin map" boards than expected, thanks to the adapter design. The other three are each their own small feature, not a repeat of T3-S3 — and like the SX127x boards, none of this can be fully confirmed correct (BUSY-line timing, TCXO wiring, PMU register behavior) without the actual hardware in hand.

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
