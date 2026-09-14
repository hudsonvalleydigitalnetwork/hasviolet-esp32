# License

This file covers two things: the license for HASviolet-ESP32's own code, and an
attribution list for every third-party library this project builds against.
Nothing here is vendored into this repository — every third-party component listed
below is pulled in unmodified at build time via PlatformIO's `lib_deps`
([platformio.ini](platformio.ini)); this repo carries none of their source.

## HASviolet-ESP32

Copyright (c) 2021–2026 Joe Cupano

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*(Copyright line names the git identity behind this repo's commits — adjust if
it should instead, or also, credit Hudson Valley Digital Network as an org.)*

## Third-party components

Every library below was verified against the actual package installed by
PlatformIO (`.pio/libdeps/`), not assumed from a project name — license, author,
and upstream repo all confirmed from each package's own `LICENSE`/`library.json`.
Each keeps its own license and copyright regardless of the MIT terms above;
nothing in this project relicenses them.

### MIT

| Component | Copyright | Upstream |
|---|---|---|
| RadioLib | Jan Gromeš | https://github.com/jgromes/RadioLib |
| ArduinoJson | Benoit Blanchon | https://github.com/bblanchon/ArduinoJson |
| XPowersLib | Lewis He | https://github.com/lewisxhe/XPowersLib |
| AXP202X_Library | Lewis He | https://github.com/lewisxhe/AXP202X_Library |
| Heltec ESP32 Dev-Boards | Heltec Automation | https://github.com/HelTecAutomation/Heltec_ESP32 |
| Adafruit BusIO | Adafruit Industries | https://github.com/adafruit/Adafruit_BusIO |
| Adafruit ST7735 and ST7789 Library | Adafruit Industries (Limor Fried/Ladyada) | https://github.com/adafruit/Adafruit-ST7735-Library |
| ESP8266 and ESP32 OLED driver for SSD1306 displays | Daniel Eichhorn & Fabrice Weinberg (ThingPulse) | https://github.com/ThingPulse/esp8266-oled-ssd1306 |
| MeshCore | meshcore-dev contributors | https://github.com/meshcore-dev/MeshCore |
| Crypto | Rhys Weatherley / Southern Storm Software, Pty Ltd | https://github.com/rweather/arduinolibs |
| RTClib | Adafruit Industries | https://github.com/adafruit/RTClib |
| Melopero RV3028 | Melopero Electronics | https://github.com/melopero/Melopero_RV-3028_Arduino_Library |
| CayenneLPP | Electronic Cats | https://github.com/ElectronicCats/CayenneLPP |
| base64 (Densaugeo) | Julian van Doorn | https://github.com/Densaugeo/base64_arduino |

Crypto/RTClib/Melopero RV3028/CayenneLPP/base64 are MeshCore's own declared
dependencies (its `library.json`), not ones this project would otherwise
need -- see project_state.md's MeshCore section for why they had to be
listed here explicitly too rather than resolving automatically.

### zlib

| Component | Copyright | Upstream |
|---|---|---|
| ed25519 (orlp) | Orson Peters | https://github.com/orlp/ed25519 |

Not a direct dependency of this project or of MeshCore's own declared
`library.json` dependencies either -- it's vendored *inside* MeshCore's own
repo (`lib/ed25519/`, a sibling of MeshCore's `src/` tree) and only reaches
this project's build via `lib/MeshCoreEd25519Compat/`'s pass-through
translation units (see that folder's own file comments and
project_state.md), which compile MeshCore's unmodified copy without this
repo carrying any of its source directly.

### BSD

| Component | Copyright | Upstream |
|---|---|---|
| Adafruit GFX Library | Adafruit Industries | https://github.com/adafruit/Adafruit-GFX-Library |
| Adafruit SH110X | Adafruit Industries | https://github.com/adafruit/Adafruit_SH110x |

### LGPL — carries its own separate obligations regardless of this project's own MIT license

| Component | Version | License | Copyright | Upstream |
|---|---|---|---|---|
| ESP Async WebServer | LGPL-3.0 | Hristo Gochkov, Mathieu Carbou | https://github.com/mathieucarbou/ESPAsyncWebServer |
| Async TCP | LGPL-3.0 | Hristo Gochkov, Mathieu Carbou | https://github.com/mathieucarbou/AsyncTCP |
| WebSockets | LGPL-2.1 | Markus Sattler | https://github.com/Links2004/arduinoWebSockets |
| TimeLib | LGPL-2.1+ | Michael Margolis, Paul Stoffregen | https://github.com/PaulStoffregen/Time |

None of these four are modified here — PlatformIO fetches each one unmodified
from its own upstream at build time, so the upstream repository linked above
*is* the corresponding source for the copy this project links against. Anyone
receiving a HASviolet-ESP32 build can get the exact same unmodified LGPL source
from those links, and (per each library's own LGPL terms) can relink a modified
version of any of these four against this project's object code.

### Bundled-but-unused, flagged for completeness

The Heltec ESP32 Dev-Boards library ships a precompiled `liblorawan.a` (per
target: `src/esp32/`, `src/esp32s3/`, `src/esp32c3/`) for a LoRaWAN stack this
project's code never calls into. Verified directly against a real build
(`nm` on `firmware.elf` for `heltec_wifi_lora_32_V2`): none of that blob's
symbols appear in the linked binary — the linker drops the entire unreferenced
`.a` member, so it isn't actually part of any HASviolet-ESP32 firmware this
project ships, despite being present in the library's own source tree.

The same library declares `heltec-eink-modules` (Todd Herbert,
https://github.com/todd-herbert/heltec-eink-modules, license unclear — no
`LICENSE` file upstream) as a dependency, for e-ink panels no board in this
project's [platformio.ini](platformio.ini) uses. Same check applies: nothing
from it appears in any built firmware, so it isn't listed above as a credited
component — noted here rather than silently omitted.
