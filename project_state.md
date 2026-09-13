# Project State — HASviolet ESP32

_Last reviewed: 2026-09-13 — the SX127x multi-board work below is merged to `main` (via `ESP32-support`); the SX126x/RadioLib section reflects the `Radiolib` branch, complete and pushed but not yet merged._

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

- [src/main.cpp](src/main.cpp) — all firmware logic (517 lines): WiFi/web/websocket init, LoRa send/receive, JSON config loading, OLED logo/display.
- [src/HASviolet_config.h](src/HASviolet_config.h) — compile-time secrets/config: WiFi AP SSID/key, STA SSID/key, web user/pass. **Currently committed with real-looking placeholder credentials in plaintext** (`WIFI_SSID "HomeWAN"`, a phone-number-shaped WiFi key, `WWW_USER/WWW_KEY = radio/radio`) — worth confirming these are dummy values before any public release.
- [src/HVDN_logo.h](src/HVDN_logo.h) — compiled bitmap logo for OLED splash.
- [data/](data) — SPIFFS image contents actually served by the device: `hasVIOLET_INDEX.html`, `.css`, `.js`, `.json` (channel/contact/macro config), `favicon.ico`, plus a `.crt`/`.key` pair (present but unused — no TLS wired up in `main.cpp` yet, consistent with the README's stated gap).
- [development/](development) — **not part of the active build**; holds two alternate/parallel dashboard UI trees (`dashboard_v1`, `dashboard_v2`) and a `defines/` folder with `HASviolet_boards.h` (pin maps for TTGO LoRa v1/v2, TTGO T-Beam, Heltec), `HASviolet_channels.h` (channel presets HV0–HV5+ with frequency/modem), and `HASviolet_cmdmsgs.h` (protocol command constants). These look like a staging area for multi-board support and a v2 UI that hasn't been merged into `src`/`data` yet.
- [platformio.ini](platformio.ini) — 13 `[env:...]` sections on `Radiolib` (9 on `main`), one per supported board (see "Board support" below); board choice lives entirely in build flags, `src/main.cpp` has no per-board source. Default env (`pio run` with no `-e`) is still `heltec_wifi_lora_32_V2` for back-compat.
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

All of these use an SX1276/SX1277-class radio. Board selection is entirely via `-D<BOARD>` build flags; `src/main.cpp` dispatches on those through a shared `HASV_HELTEC_BOARD`/`hvLoRa` abstraction rather than per-board branches scattered through the file.

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

## Suggested next steps (not yet started)

1. Merge `Radiolib` into `main` (currently pushed, PR not yet opened).
2. Decide the fate of the `development/` dashboard v1/v2 trees — merge one into `data/`+`src/`, or document why both are kept.
3. Confirm `HASviolet_config.h` values are placeholders, or move real secrets out of version control (e.g., a gitignored local config).
4. Implement or explicitly schedule the TLS/auth work called out in the README.
5. Source real pin data for RAK11200 and add it to the board matrix.
6. Get any of the 13 boards on actual hardware to confirm what compiling alone can't: BUSY-line timing, TCXO/DIO1 wiring, PMU register behavior, and (Wireless Tracker specifically) TFT panel geometry.
