# Release History

## Pre-Release v0.5 — 2026-09-13

Work done on the `ESP32-support` branch (not yet merged to `main`).

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
