# Project State — HASviolet ESP32

_Last reviewed: 2026-09-13_

## What this is

HASviolet ESP32 is a work-in-progress firmware for an ESP32 board that acts as a **LoRa transceiver node** with a **web/WebSocket UI**, built to have UI/UX parity with the sister project [HASviolet on RPi](https://github.com/hudsonvalleydigitalnetwork/hasviolet). It targets amateur-radio-style LoRa messaging: browser client connects over WiFi, sends/receives text over a WebSocket session, and the ESP32 bridges that to LoRa TX/RX.

Built with **PlatformIO** (Arduino framework) for the **Heltec WiFi LoRa 32 V2** board (embedded SX127x LoRa + SSD1306 OLED). Author develops in VS Code + PlatformIO extension on Linux.

No SSL/TLS and no user authentication are implemented yet on the ESP32 side (unlike the RPi version) — called out explicitly in the README as a known gap to be addressed later.

## Architecture

Three conceptual parts, one physical device:

- **Server** — ESP32 running Web + WebSocket services on Core 1, LoRa comms on Core 0 (FreeRTOS task `HasTRX` pinned to core 0 via `xTaskCreatePinnedToCore`).
- **Radio** — SX1276-class LoRa module (Heltec-embedded), driven over SPI via the Heltec library.
- **Client** — Static files served from SPIFFS: `hasVIOLET_INDEX.html` → loads `hasVIOLET.css` + `hasVIOLET.js` → JS opens a WebSocket to the device and drives the whole UI (channel/radio settings, TX/RX text, macros, CMDline).

Boot sequence (from `setup()` in [src/main.cpp](src/main.cpp)) prints numbered INIT stages: 000 core start → 100 SPIFFS → 200 JSON config load → 300 WiFi (STA, falls back to self-hosted AP) → 400 web server → 500 WebSockets → 600 OLED. HasTRX (LoRa) task then starts on core 0; `loop()` on core 1 just pumps `webSocket.loop()`.

Communication protocol between client and server is a simple string-command scheme (`SET:...`, `GET:...`, `TX:`, `RX:`, acks prefixed `ACK:`) — see `development/defines/HASviolet_cmdmsgs.h` for the (currently unused/reference) canonical list.

## Repo layout

- [src/main.cpp](src/main.cpp) — all firmware logic (517 lines): WiFi/web/websocket init, LoRa send/receive, JSON config loading, OLED logo/display.
- [src/HASviolet_config.h](src/HASviolet_config.h) — compile-time secrets/config: WiFi AP SSID/key, STA SSID/key, web user/pass. **Currently committed with real-looking placeholder credentials in plaintext** (`WIFI_SSID "HomeWAN"`, a phone-number-shaped WiFi key, `WWW_USER/WWW_KEY = radio/radio`) — worth confirming these are dummy values before any public release.
- [src/HVDN_logo.h](src/HVDN_logo.h) — compiled bitmap logo for OLED splash.
- [data/](data) — SPIFFS image contents actually served by the device: `hasVIOLET_INDEX.html`, `.css`, `.js`, `.json` (channel/contact/macro config), `favicon.ico`, plus a `.crt`/`.key` pair (present but unused — no TLS wired up in `main.cpp` yet, consistent with the README's stated gap).
- [development/](development) — **not part of the active build**; holds two alternate/parallel dashboard UI trees (`dashboard_v1`, `dashboard_v2`) and a `defines/` folder with `HASviolet_boards.h` (pin maps for TTGO LoRa v1/v2, TTGO T-Beam, Heltec), `HASviolet_channels.h` (channel presets HV0–HV5+ with frequency/modem), and `HASviolet_cmdmsgs.h` (protocol command constants). These look like a staging area for multi-board support and a v2 UI that hasn't been merged into `src`/`data` yet.
- [platformio.ini](platformio.ini) — single env `heltec_wifi_lora_32_V2`; lib deps: ESP Async WebServer, `fhessel/esp32_https_server`, `links2004/WebSockets`, ArduinoJson. Build flags: `-DHELTEC -DARDUINO_LORA -DHAS_OLED`.
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

## Known issues / gaps (from code + README, not fixed by anyone yet)

- No TLS and no user authentication on the ESP32 web/WebSocket services, despite `data/hasVIOLET.crt`/`.key` and `WWW_USER`/`WWW_KEY` existing — explicitly flagged by the author as deferred.
- Plaintext WiFi/web credentials committed in `src/HASviolet_config.h`.
- `development/` trees (dashboard_v1/v2, multi-board defines, protocol header) are disconnected from the active `src/`+`data/` build — unclear which, if any, is meant to land next.
- README build instructions reference a mistyped/broken clone command (`git clone https://https://github.com/.../hasviolet-esp32.git`).
- Only one board (Heltec WiFi LoRa 32 V2) is actually wired up in `platformio.ini`/build flags, though pin maps for TTGO LoRa v1/v2 and T-Beam already exist in `development/defines/HASviolet_boards.h`, suggesting multi-board support is planned but not yet exposed.

## Suggested next steps (not yet started)

1. Decide the fate of the `development/` dashboard v1/v2 trees — merge one into `data/`+`src/`, or document why both are kept.
2. Confirm `HASviolet_config.h` values are placeholders, or move real secrets out of version control (e.g., a gitignored local config).
3. Implement or explicitly schedule the TLS/auth work called out in the README.
4. Fix the broken `git clone` line in the README.
