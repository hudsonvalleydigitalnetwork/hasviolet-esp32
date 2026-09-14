# Release History

## Pre-Release v0.6 — 2026-09-13

`Radiolib` work

### Added
- SX126x radio support via [RadioLib](https://github.com/jgromes/RadioLib), bringing the total to **13 supported boards**: LILYGO T3-S3, B&Q Station G2, LILYGO T-Beam Supreme (T-Beam S3-Core), and Heltec Wireless Tracker — the four Meshtastic community-favorite ESP32-S3 boards scoped as a follow-up in v0.5.
- `RadioLibSX126x`: an adapter class implementing the same method names the existing SX127x `LoRaClass` API uses, so `HasTRX`/`sendLORA`/`onReceiveLORA`/the `GET:LORA` websocket handler needed no changes to support the new radio chip.
- `SH110XOLEDAdapter<T>`: a templated adapter covering both SH1107 (Station G2) and SH1106 (T-Beam Supreme) OLED controllers, plus `ST7735TFTAdapter` for Wireless Tracker's color TFT — its one board with no OLED at all.
- AXP2101 PMU support (`lewisxhe/XPowersLib`) for T-Beam Supreme, alongside the existing AXP192 path for T-Beam v1.1.
- All board pins/rail assignments hand-sourced from Meshtastic's own shipping firmware config (`meshtastic/firmware`), not guessed.

### Changed
- `project_state.md`: SX126x section marked complete; board-support tables split by branch; cross-checked all 13 boards against Meshtastic's actual `variants/` directory rather than device names alone — found that `heltec_wireless_stick` (plain, non-Lite) isn't actually a Meshtastic-supported device, and `ttgo_lora32_v2` uses a board id no current Meshtastic variant builds against, even though both remain useful to this project.

### Verification
All 13 board environments (9 from v0.5 + 4 new) build clean via `pio run -e <env>`, including regression checks across every existing board family after each new addition.

## Pre-Release v0.5 — 2026-09-13

### Added
- Multi-board support: **9 boards** now build from the same `src/main.cpp`, selected purely via `platformio.ini` build flags —
  Heltec WiFi LoRa 32 V1/V2/V3, Heltec Wireless Stick, Heltec Wireless Stick Lite, LILYGO/TTGO LoRa32 V1/V2/V2.1, and LILYGO/TTGO T-Beam v1.1.
- AXP192 power-management init (`initPMU()`) for T-Beam, which gates its LoRa/GPS power rails.
- `hvLoRa` radio abstraction and a generic (non-Heltec) LoRa/OLED init path, so TTGO/T-Beam boards no longer depend on the Heltec library.
- `project_state.md`: a project snapshot plus a scoped (not yet started) follow-up plan for SX126x radio support (RadioLib) to cover Meshtastic's community-favorite ESP32-S3 boards (T3-S3, T-Beam Supreme, Station G2, Heltec Wireless Tracker).

### Fixed
- README.md: broken/malformed `git clone` command and duplicated `https://` link, plus numerous typos.
- Pre-existing build breakage that affected every board (not previously caught since the project had never been build-verified): missing `Time.h`/`FreeRTOS.h` dependencies, an unused `esp32_https_server` lib that no longer compiles against current ESP-IDF, and an LDF over-pull of the RP2040-only `AsyncTCP_RP2040W` backend.

### Verification
All 9 board environments built successfully with `pio run -e <env>` against a real espressif32 toolchain.
