# Project State — HASviolet ESP32

_Last reviewed: 2026-09-14 — the SX127x multi-board work and the SX126x/RadioLib work below are both merged to `main`. A `meshcore` branch (cut from `main`) has since replaced the split described in "Architecture" and "Board support" below with a single shared radio HAL across all 13 boards — see "Shared Radio HAL + RadioLib everywhere" below, which supersedes those sections' radio-layer descriptions (the board/OLED/PMU material in them is still accurate) — and, on top of that HAL, a first working MeshCore backend + runtime native/MeshCore switch on one board (Heltec V3) — see "MeshCore backend + runtime network switch" below._

## What this is

HASviolet ESP32 is a work-in-progress firmware for an ESP32 board that acts as a **LoRa transceiver node** with a **web/WebSocket UI**, built to have UI/UX parity with the sister project [HASviolet on RPi](https://github.com/hudsonvalleydigitalnetwork/hasviolet). It targets amateur-radio-style LoRa messaging: browser client connects over WiFi, sends/receives text over a WebSocket session, and the ESP32 bridges that to LoRa TX/RX.

Built with **PlatformIO** (Arduino framework). Author develops in VS Code + PlatformIO extension on Linux. `main` builds for nine SX127x boards; the `Radiolib` branch adds four more on SX1262, for **13 supported boards total** across the two.

No SSL/TLS and no user authentication are implemented yet on the ESP32 side (unlike the RPi version) — called out explicitly in the README as a known gap to be addressed later.

## Architecture

Three conceptual parts, one physical device:

- **Server** — ESP32 running Web + WebSocket services on Core 1, LoRa comms on Core 0 (FreeRTOS task `HasTRX` pinned to core 0 via `xTaskCreatePinnedToCore`). This requires a dual-core chip — see the SX126x/C3/C6 note below.
- **Radio** — either SX1276-class (Heltec library, or `sandeepmistry/LoRa` + per-board pins for TTGO/T-Beam) or, on `Radiolib`-branch boards, SX1262-class via RadioLib and a `RadioLibSX126x` adapter — all behind a shared `hvLoRa` macro in `main.cpp` so the rest of the file doesn't care which. **Superseded on `meshcore`** — see "Shared Radio HAL + RadioLib everywhere" below.
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

## Shared Radio HAL + RadioLib everywhere (`meshcore` branch)

Started as prep work for letting a user choose HASviolet native networking or [MeshCore](https://github.com/meshcore-dev/MeshCore) at runtime. Comparing the two stacks layer-by-layer found they diverge at the mesh/routing/security layer but already converge on the physical layer — both ultimately drive an SX126x/SX127x chip through RadioLib with an interrupt-flag+poll state machine. This work makes that convergence real in this codebase: every one of the 13 boards now goes through RadioLib and one shared HAL, instead of only the 4 SX126x boards.

### What changed

- New local library, [lib/HasRadio/](lib/HasRadio/): `HasRadio.h` is an abstract interface (method names/shapes deliberately close to MeshCore's own `mesh::Radio` contract — `startSendRaw`/`recvRaw`/`getLastRSSI`/etc. — so vendoring MeshCore's `Mesh`/`Dispatcher` onto this same HAL later is a near drop-in). `HasRadioSX126x` and `HasRadioSX127x` both implement it directly on RadioLib.
- `sandeepmistry/LoRa` and the Heltec library's bundled radio driver are both gone. Heltec boards now use `Heltec.begin()` for display/Vext/serial only (`LoRaEnable=false`); the radio object is always a `HasRadio` subclass, same as every other board.
- `src/main.cpp`'s old `hvLoRa` macro maze (`HASV_HELTEC_BOARD`/`HASV_SX126X_BOARD`/three-way dispatch) collapsed to one axis (chip family) and one object (`hvRadio`). `HasTRX`/`sendLORA` were rewritten against the new interface; the dead `onReceiveLORA()` (defined, never actually registered as a callback) was removed.
- Heltec boards needed new explicit `-DLORA_SCK/MISO/MOSI/CS/RST/IRQ` pin flags in `platformio.ini` (previously implicit inside `Heltec.begin()`) — sourced from arduino-esp32's own `pins_arduino.h` for each board id; all four SX1276 Heltec boards (V1/V2/Wireless Stick/Wireless Stick Lite) share identical wiring (SCK=5, MISO=19, MOSI=27, CS=18, RST=14, DIO0=26).
- The now-dead `generic_radio`/`RF_PACONFIG_PASELECT_PABOOST` build-flag section was removed — RadioLib's SX127x `setOutputPower()` handles PA_BOOST-vs-RFO selection internally; `HasRadioSX127x::setTxPower()` just clamps to `>=2` dBm to keep it always picking PA_BOOST (RFO isn't wired on any board here).
- **Known regression, accepted deliberately:** `GET:LORA`'s register dump is now "not supported" on SX127x boards too (previously real, via `sandeepmistry/LoRa`). RadioLib doesn't expose raw register reads on its public API without a "godmode" build (which MeshCore's own pinned RadioLib fork uses — this project doesn't build RadioLib that way yet).
- TX is still blocking under the hood in both `HasRadio` implementations — matches prior behavior exactly (`RadioLib::transmit()` was already blocking before this work); real non-blocking TX arrives when MeshCore's own dispatcher does. `isSendComplete()` exists on the interface for that but always returns `true` today.
- Also ported two hardware-confirmed bugs found on the (separate, unrelated) `mesh` branch's Meshtastic-interop hardware bring-up, since they directly affect boards this work touches:
  - `-DClass_Wifi_LoRa` build flag, working around a real case-mismatch bug in `heltecautomation/Heltec ESP32 Dev-Boards@^2.1.1` itself (`heltec.h` defines `Class_WIFI_LORA`, `heltec.cpp`'s constructor checks the differently-cased `Class_Wifi_LoRa`, so `display` was never allocated and `Heltec.begin()` null-derefed).
  - Heltec V3 recategorized as SX1262 (not SX1276 like every other Heltec board here) — folded directly into this session's `HASV_SX126X_BOARD` axis rather than landing as a separate intermediate fix.
  - Bounded the previously-infinite WiFi STA-connect loop in `initWiFi()` (`WIFI_POLL_DELAY`/`WIFI_POLL_TRIES` were already defined for this but never used).
- Found and fixed one more, independently, during this session's own hardware bring-up (see below): `WiFi.softAP()`'s return value was never checked, so a rejected passphrase still printed "WiFi AP initialized." `WIFI_APKEY` in `HASviolet_config.h` was also too short for WPA2-PSK (6 chars; needs 8+) — confirmed as the actual cause on real hardware.

### Verification

- **Build**: all 13 `pio run -e <env>` environments compile clean.
- **Hardware**: a real Heltec WiFi LoRa 32 V3 was flashed and confirmed live — this is the one board that previously hung permanently at boot on `main` (the `Class_Wifi_LoRa` null-deref, and separately Heltec's own SX1276 driver looping forever on V3's actual SX1262 radio). With this work: clean boot through `INIT: COMPLETE`, `HasRadioSX126x::begin()` succeeds against the real chip (pins SCK=9, MISO=11, MOSI=10, CS=8, RST=12, DIO1=14, BUSY=13, TCXO=1.8V — Meshtastic-sourced, now hardware-confirmed rather than build-only), WiFi AP came up and was joined from a phone, and the existing `hasVIOLET_INDEX.html` web UI loaded over it. Not yet tried on this board: an actual LoRa TX/RX (no second node on hand) or a visual check of the OLED logo.
- The other 12 boards remain build-verified only, same as before this session — no new hardware claims for them.

## MeshCore backend + runtime network switch (`meshcore` branch, on top of the Radio HAL work above)

The actual second networking backend this branch exists for: [MeshCore](https://github.com/meshcore-dev/MeshCore) vendored as a real dependency, a HASviolet-authored application layer bridging it to the existing browser UI's wire protocol, and a JSON-persisted, reboot-to-apply runtime switch between it and native. Scoped deliberately narrow for this first pass, per decisions made before starting: **Heltec V3 only**, **MeshCore "channel" flood messaging only** (no contacts/DMs — native's own wire protocol has no addressing concept to map them onto anyway), **switch takes effect on reboot**, not a live hot-swap (native's `hvRadio` and MeshCore's own radio classes can't share the SPI/radio pins at once).

### Vendoring MeshCore — real, and messier than a one-line lib_deps entry

MeshCore ships a genuine `library.json` (MIT-licensed, version 1.10.0) with a `build_as_lib.py` extraScript, so it *can* be pulled in as a plain `lib_deps` git dependency rather than cloned in full — pinned to a commit (`https://github.com/meshcore-dev/MeshCore.git#0679dbe...`; no single clean "core library" version tag exists upstream, only per-firmware-role tags like `repeater-v1.17.1`). Its own build sets two global RadioLib flags, `RADIOLIB_STATIC_ONLY`/`RADIOLIB_GODMODE` — checked directly against RadioLib's own source before adding them project-wide: both only change RadioLib's *internal* SPI-buffer strategy and member visibility, never its public API, so `lib/HasRadio` keeps working completely unchanged with these on.

Consuming MeshCore this way (rather than cloning the whole repo and building one of *its own* `platformio.ini` environments, which is how MeshCore's own CI always does it) surfaced several real gaps, each fixed on this project's side without touching any of MeshCore's own vendored files:

- `build_as_lib.py`'s `MC_VARIANT=heltec_v3` build define does pull in MeshCore's own already-correct `variants/heltec_v3/target.h`/`target.cpp` (near-identical pins to what's already hardware-verified for native mode) — but that variant's `target.h` `#include`s a sibling file (`HeltecV3Board.h`) via angle brackets, which only resolves with an explicit `-I` flag pointed at wherever PlatformIO's lib_deps fetch actually put it (`.pio/libdeps/<env>/MeshCore/variants/heltec_v3`) — LDF's usual same-directory quoted-include handling doesn't cover this case.
- MeshCore's `Identity.cpp` needs an ed25519 reference implementation vendored *inside MeshCore's own repo* at `lib/ed25519/` (a sibling of MeshCore's `src/` tree, not something `build_as_lib.py`'s SRC_FILTER reaches — that only compiles `src/`). New `lib/MeshCoreEd25519Compat/` (this project's own code) provides nine tiny pass-through `.c` files, each `#include`-ing exactly one of MeshCore's own vendored `lib/ed25519/*.c` files by name via another explicit `-I` flag — one file per source (not a unity build) to avoid symbol-collision risk across files not designed to be concatenated, and deliberately *not* named the same as their target (a quoted `#include` checks its own file's directory first, so e.g. a file named `fe.c` including `"fe.c"` would just re-include itself).
- Four more of MeshCore's own declared `library.json` dependencies (`rweather/Crypto`, `adafruit/RTClib`, `melopero/Melopero RV3028`, `electroniccats/CayenneLPP`) plus one *undeclared* one its code actually needs (`densaugeo/base64`, for `encode_base64`/`decode_base64` — used by MeshCore's own `BaseChatMesh::addChannel()` and by this project's channel-PSK derivation) don't get auto-resolved when MeshCore is consumed as a plain git-URL `lib_dep` — added explicitly to `platformio.ini` instead of relying on transitive resolution.
- `heltecautomation/Heltec ESP32 Dev-Boards` (needed for this board's own display bring-up, unrelated to MeshCore) ships its own bundled-but-unused LoRaWAN stack containing a file at the exact same relative path MeshCore's `ESP32Board.h` needs from the *real* ESP-IDF framework (`driver/gpio.h`) — completely different file, same path, so whichever `-I` search path GCC consults first wins and it isn't guaranteed to be the real one. Confirmed by hitting exactly this collision on a real build. Fixed by sidestepping the ambiguous path entirely: defining `GPIO_PIN_COUNT` from the unambiguous `soc/soc_caps.h` header instead, and forward-declaring the handful of real ESP-IDF GPIO functions `ESP32Board.h` calls directly (the actual symbols are already linked in from arduino-esp32's own GPIO driver component regardless of which header declared them).
- PlatformIO's default LDF mode discovers `lib/` folders by doing a *static text scan* for `#include` lines from `src/` — it does **not** respect `#ifdef` guards during that scan. Two consequences, both hit on real builds: (1) `lib/MeshCoreEd25519Compat/` needed its own header (`meshcore_ed25519_compat.h`) `#include`d from `HasMeshCoreChat.h` just to give LDF a graph edge to discover it at all — without any file referencing it, LDF never compiled it, producing `undefined reference` link errors for symbols that were never even compiled in; (2) once discovered, LDF then compiles that library's files on *every* board regardless of runtime/compile-time gating elsewhere, so each of those nine pass-through files (and `HasMeshCoreChat.h`/`.cpp` themselves) needed their own internal `#ifdef HASV_MESHCORE_SUPPORT` guard to compile to nothing on boards that don't vendor MeshCore — confirmed by a real build failure on non-MeshCore boards before this guard was added (same root cause, different manifestation, as the `HasRadioSX126x.cpp`/`SX126X_TCXO_VOLTAGE` issue from the Radio HAL work above: anything under `lib/` compiles project-wide, board-specific content needs its own fallback or guard, not just an `#ifdef` in whatever happens to include it).

### HASviolet's MeshCore application layer

New `src/HasMeshCoreChat.h`/`.cpp` (gated behind `-DHASV_MESHCORE_SUPPORT`, currently only set for `env:heltec_wifi_lora_32_v3`): a trimmed version of MeshCore's own `examples/simple_secure_chat/main.cpp` pattern (`class HasMeshCoreChat : public BaseChatMesh`), cut down to channel-only — no contact/DM handling, those `BaseChatMesh` overrides are stubbed the same minimal way the example itself stubs the pieces it doesn't need. `begin()` derives the MeshCore channel's name *and* PSK from HASviolet's own existing `channel` config string (SHA256 → base64) rather than a separately-entered secret, so a user who already agrees on a channel name in native mode lands in the same MeshCore group too — not a secure secret-sharing scheme (the PSK is fully determined by the public channel name), matching native mode's own unauthenticated-broadcast security posture. `onChannelMessageRecv()` formats incoming messages as `"RX:" + text` and broadcasts through the same `webSocket` object `HasTRX` already uses — reusing the exact wire prefix `data/hasVIOLET.js` already recognizes, so the browser UI needed zero changes to support this second backend for receiving. Sending is symmetric: the `TX:` WebSocket handler now branches on `networkMode` and calls `sendChannelText()` (which just calls `BaseChatMesh`'s own public `sendGroupMessage()`) instead of `sendLORA()`.

### Runtime switch

New `NETWORK.mode` field in `hasVIOLET.json` (default `"native"` — existing installs with no such field must not silently switch modes), plus a new `SET:NETWORK:NATIVE`/`SET:NETWORK:MESHCORE` WebSocket command (same pattern as the existing `SET:` handlers) that persists the choice back to the JSON file and calls `ESP.restart()`. `setup()` branches once, early: `networkMode == "meshcore"` calls MeshCore's own vendored `radio_init()` and `HasMeshCoreChat::begin()`, pumping its `loop()` from a new Core-0 task (`HasMeshCoreLoop`) in the exact task slot `HasTRX` uses in native mode — never both, which is what keeps the two backends' independent radio objects (MeshCore's own `CustomSX1262`-based one, native's `HasRadioSX126x`) from fighting over the same SPI bus and pins despite both being statically constructed in the one firmware image (C++ global objects construct unconditionally; only one ever has `begin()` actually called on it per boot).

**Incidental fix needed to make the switch actually persist:** `loadJsonFile()`'s ArduinoJson deserialization filter only ever allowed a top-level `"CURRENT"` key through — but `hasVIOLET.json` has no such key at all, so every field in it (`RADIO`, `CONTACT`) has silently kept its hardcoded default forever, never actually loading from JSON, since long before this session. Left as-is, the new `NETWORK.mode` field would have been equally dead on arrival. Corrected the filter to the keys the code actually reads (`RADIO`, `CONTACT`, `NETWORK`) rather than leave a second feature quietly broken on top of the first — a real, if narrow, behavior change to native mode's existing config loading as a side effect, flagged here rather than left implicit.

### Verification

- **Build**: all 13 board environments compile clean (confirmed via a full `pio run -e <env>` sweep across every environment, same standard as the Radio HAL work) — `HASV_MESHCORE_SUPPORT`'s gating, once corrected per the LDF-static-scan gotcha above, keeps the other 12 boards byte-for-byte unaffected by anything MeshCore-related.
- **Hardware**: a real Heltec V3 was flashed and confirmed live in native mode after this branch's changes (boot, SX1262 radio init, WiFi AP, web UI — see the Radio HAL section above); MeshCore mode itself has **not yet been confirmed on real hardware** in this pass — the session's USB/WSL device passthrough broke (a `usbipd`/WSL-interop "Exec format error" unrelated to this project) before MeshCore mode could be flashed and its boot log captured. Re-attaching the board and confirming `HasMeshCoreChat::begin()` completes (identity created/loaded, channel joined) via Serial log is the next concrete step, not yet done.
- **Not yet tried at all:** actual over-the-air MeshCore channel messaging (needs a second MeshCore node — even a phone running the official app on the same channel name/PSK would do); the OLED showing anything in MeshCore mode (native's `initOLED()`/`logo()` calls are unconditional in `setup()`, unrelated to `networkMode`, so this should already work, just unconfirmed).



- No TLS and no user authentication on the ESP32 web/WebSocket services, despite `data/hasVIOLET.crt`/`.key` and `WWW_USER`/`WWW_KEY` existing — explicitly flagged by the author as deferred.
- Plaintext WiFi/web credentials committed in `src/HASviolet_config.h`.
- `development/` trees (dashboard_v1/v2, multi-board defines, protocol header) are disconnected from the active `src/`+`data/` build — unclear which, if any, is meant to land next. The old `development/defines/HASviolet_boards.h` pin tables are now superseded by `platformio.ini` + arduino-esp32's own variant pins for the boards on this branch, but the file itself wasn't touched.
- README build instructions reference a mistyped/broken clone command (`git clone https://https://github.com/.../hasviolet-esp32.git`) — fixed on `main` separately from this branch.

## Suggested next steps (not yet started)

1. Merge `meshcore`'s shared Radio HAL work into `main` (`Radiolib` itself is already merged, per the top note).
2. Decide the fate of the `development/` dashboard v1/v2 trees — merge one into `data/`+`src/`, or document why both are kept.
3. Confirm `HASviolet_config.h` values are placeholders, or move real secrets out of version control (e.g., a gitignored local config).
4. Implement or explicitly schedule the TLS/auth work called out in the README.
5. Source real pin data for RAK11200 and add it to the board matrix.
6. Get the other 12 boards on actual hardware (Heltec V3 is now done, see above) to confirm what compiling alone can't: BUSY-line timing, TCXO/DIO1 wiring, PMU register behavior, and (Wireless Tracker specifically) TFT panel geometry.
7. ~~Vendor MeshCore as a pinned `lib_deps` entry... then add the runtime native-vs-MeshCore selection this branch is ultimately for.~~ **Done** — see "MeshCore backend + runtime network switch" above. `lib/HasRadio` and MeshCore's own radio classes ended up exactly as decided below: two separate RadioLib consumers sharing an interface shape and the same `platformio.ini` pin/config source of truth, not one literal shared object.
   - **Decided:** MeshCore's radio layer stays *its own* RadioLib-subclassing classes (`CustomSX1262`/etc.), not an adapter over `lib/HasRadio`. Chosen over writing our own adapter (which would reuse the already-hardware-verified `HasRadioSX126x`/`HasRadioSX127x` for MeshCore mode too) specifically to keep zero HASviolet code inside MeshCore's own call path, so a future MeshCore version bump can only break our (small) glue layer, never code we wrote against their internals. Trade-off accepted, and now real: MeshCore mode's radio path doesn't inherit native mode's hardware verification and needs its own hardware bring-up (still pending, see above).
8. Confirm MeshCore mode on the real Heltec V3 (identity/channel join via Serial log), then an actual over-the-air channel message with a second MeshCore node.
9. Extend MeshCore support to boards beyond Heltec V3 — most of this project's other SX126x boards likely have official MeshCore variants too (`lilygo_t3s3`, `station_g2`, `lilygo_tbeam_supreme_SX1262` were all seen in MeshCore's own `variants/` listing during this session), unconfirmed against exact pin sets.
10. Decide whether/how to extend MeshCore mode past channel-only messaging (contacts/DMs) — would need new `hasVIOLET.json` schema (today's `CONTACT` block is one flat mycall/dstcall pair, not a contact list) and new browser UI, not just server-side work.
