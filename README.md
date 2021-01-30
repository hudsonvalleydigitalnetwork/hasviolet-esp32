# HASviolet ESP32

HASviolet ESP32 is a (very much) work-in-progress LoRa transceiver node that is part of the HASviolet project. Best efforts are made to document as much of the build and code as possible in unvarnished language.

## Table of Contents

- [Quick Start](#Quick-Start)

- [Build It](#Build-It)

- [Watch It](#Watch-It)

- [Hack It](#Hack-It)

# Quick Start

The quickest way to get started with HASviolet ESP32 is to

- Buy a [HiLetgo ESP32 LoRa v2 device](https://www.amazon.com/gp/product/B07WHRS2XG/)
- Download the [latest release](https://github.com/joecupano/hasty-esther/releases)
- Upload the BIN file to the device using your favorite ESP32 Flash tool

Alternatively, you can compile the firmware yourself by cloning this repo and following the instructions in the "Build It" section.

# Build It

All coding was performed using [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO for VS Code extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide). Follow the installation instructions for those components. Linux is my platform of choice for development. 

## Create your Workspace

I keep a home directory for all my source code so I suggest creating your own before cloning the HASviolet ESP32 repo.

```
mkdir ~/source
git clone https://github.com/joecupano/hasty-esther.git
```

Once cloned, go ahead and start Visual Studio Code and open the folder of where the cloned repo is in. You should get a screen similar to below.

![vsc-pio-view.png](docs/vscpio-view.png)

Edit `platformio.ini` to suit your needs. If you are flashing a <a href="https://www.amazon.com/gp/product/B07WHRS2XG/" target="_blank">HiLetgo ESP32 LoRa v2 board</a> then you will probably not have to edit anything in `platformio.ini` but make sure <mark>`upload_port`</mark> is set to the correct device (such as<mark> /dev/ttyUSB0</mark>) which may vary depending on your operating system and other devices you have connected.

## Build firmware

To test that the firmware is compiling correctly without flashing it to a device either use the terming that is part of VSC or open a terminal and navidate to the clones repo folder and run the following

```
pio run
```

This will compile all libraries and main firmware files. The resulting binary is stored as `.pio/build/heltec_wifi_lora_32_v2/firmware.bin`

## Flash firmware

Connect your computer to the board using a usb cable,

In `platformio.ini` make sure `upload_port`, `upload_speed`, etc are correct for your device. First, you flash the file system as follows:

```
pio run -t uploadfs
```

followed by flashing the code:

```
pio run -t upload
```

By default, PlatformIO builds and flashes firmware for the <a href="https://www.amazon.com/gp/product/B07WHRS2XG/" target="_blank">HiLetgo ESP32 LoRa v2 board</a>. If you would like to build for another supported board, select the corresponding build environment. For example to build and flash the firmware and file system for the ESP32 TTG0 V1 board, use the following, 

```
pio run -e ttgo-lora32-v1 -t upload -t uploadfs
```

## Creating Binary for Release

A full binary image can be created by reading the flash contents of a ESP32 that is flashed with the latest release of the firmware. To do this, run the following command,

```
esptool.py -p /dev/ttyUSB0 -b 921600 read_flash 0 0x400000 esp_flash.bin
```

This can then be flashed to a new device like so,

```
esptool.py -p /dev/ttyUSB0 --baud 460800 write_flash 0x00000 esp_flash.bin
```

# Watch It

Once the firmware and SPIFFS image has been successfully flashed, connect to the USB serial in monitor mode  

```
pio device monitor
```

After that, press the RST button on the ESP32 and you should see the following boot sequeunce;

```
INIT: HASviolet ESP32
=====================================
 000: Setup running on core 1
 100: SPIFFs initialized
 300: WiFi CL initialized
 400: Webserver initialized
 500: Websockets initialized
 600: OLED Initialized
 700: HasTRX (LoRa) running on core 0
INIT: COMPLETE
=====================================
```

# Hack It

## Architecture Model

HASviolet ESP32 is made up of three parts;  radio (LoRa transceiver), server, and client.

- Server
  
  - an ESP32 device that runs Web/Websockets services for communications with the client on Core 1 and LoRa communications on Core 0
  - LoRa libraries/functions interact with the radio via the [SPI interface](http://iot-bits.com/esp32/esp32-spi-tutorial-part-1/)

- Radio
  
  - an RF hardware module (such as the [sx1276](https://www.semtech.com/products/wireless-rf/lora-transceivers/sx1276)) capable of LoRa that is either embedded or external to the ESP32 device and connected via SPI

- Client
  
  - Web browser that runs the HASviolet ESP32 Web UI interface
  - Browser connects to Web service on ESP32 device and loads an [HTML file](https://github.com/joecupano/hasty-esther/blob/main/data/hasVIOLET_INDEX.html) loaded with HTML5, CSS, and Javascript
  - On load, Javascript runs and connects to the Websocket service on the ESP32 device to establish a websocket session
  - Received data, data to be transmitted, and radio commandsare commuicated over the websocket session

## Workflow Model

### Server

This is the sequence of initialization (<mark>INIT</mark>) events that occurs doing server (ESP32) device startup from within the <mark>void setup()</mark> loop;

- **000** When the server (ESP32) device starts it initialize board specific routines and intiializes the serial port

- **100** SPIFF system initializes

- **200** Loading of the <mark>hasVIOLET.json</mark> file from the SPIFFs image. The server is updated with the JSON configuration

- **300** WiFi service starts first looking for an existing known network and if none then starst its own AP. The WiFi service will first try to connect to any <mark>WIFI_SSID</mark> and <mark>WIFI_KEY</mark> that was specified in <mark>HASviolet_config.h</mark> at compile time. If it cannot connect to an existgin WiFi specified, it will create a WiFI Access Point with the <mark>WIFI_APSSID</mark> and <mark>WIFI_APKEY</mark> specified in <mark>HASviolet_config.h</mark> at compile time.

- **400** Webserver service is started with callbacks for each of the HTML, CSS. JS, JSON files. A favorico.ico is included in the SPIFFs image for browsers that (annoyingly) look for such image objects

- **500** Websocket service is started with callback for processing websocket messages

- **600** If OLED display is included, it is initialzied and will display the HVDN logo as compiled from <mark>HASviolet_logo.h</mark>

- INIT is complete and LoRa services are started as a tasl on a separate core

#### *FreeRTOS Magic*

For many LoRa projects out there, the use case has the ESP32 as either a receiver or a transmitter but rarely both. If the use case does do both you will see alot deep dive into C/C++ in the coding yet still run into an issue with the hardware largely due to QA of the board manaufacturers. This is all because you never want LoRa receive process to miss a packet unless it is transmitting. But we also need WebSockets always running not missing a message. This led us down the path of experimenting pinning process to cores which involved touchin into the world of [FreeRTOS](https://www.freertos.org/) on the ESP32.

The ESP32 has two cores with Core 1 being the default for all operations. What we did was within <mark>void setup()</mark>  is create a LoRa services task called <mark>HasTRX</mark> and pin it to Core 0. <mark>HasTRX</mark> runs nested loops with a LoRa receive function<mark> conditional inner-loop</mark> and an endless outer-loop. Meanwhile WebSockets is running in an endless-loop within <mark>void main()</mark> on Core 1.

When <mark>HasTRX</mark> sees a LoRa packet, it captures the message and invokes a callback that sends a WebSocket client broadcast. When a TX request comes from the (browser) client via WebSockets, the <mark>sendLORA</mark> function is calledt and once the message is transmit we trip the HasTRX <mark>conditional inner-loop</mark> to restart. We use this trick for if we want to change channels from the client which includes frequency, spreadfactor, coding, and bandwidth.

Admittedly this is less than perfect but reflects our evolving education and persistence for a more perfect union of a true LoRa transciver with WebSockets.

### Radio

Our focus has been on ESP32 boards with embedded LoRa modules either from [HopeRF]([LoRa module - HOPE MicroElectronics](https://www.hoperf.com/modules/lora/index.html)) or [Semtech](https://www.semtech.com/lora/lora-products) connected via SPI. For now we have kept with the LoRa libraries included with each board. We will revisit if enough pain ensues down the line. We've been contemplating some simple GPIO, pull-up resistor, and interrupt line inclusions to avoid some software hellholeshardware solution .

### Client

When the web browser connects to the server, it loads four files in the following order;

- **hasVIOLET_INDEX.html** Besides the skeleteon look of UI, it tells the browser to load  up the CSS and JS files in that order

- **hasVIOLET.css** Makes the UI look pretty and add texture to actions on screen

- **hasVIOLET.js **Javascript code that does all the "heavy lifting." It communicates all actions as WebSocket messages to the server that include changing channels, radio settings, as well as messages to be transmitted and those received Code also includes UI usabailityt such as defining macros and UI housekeeping. The code is well documented and includes numerous logs viewable from the browsers inspect window in the consoles tab.

- **hasVIOLET.json** a client configuration (JSON) file loaded into from the server that includes radio, contact, macor, and channel settings. These settings are stoed in variabels and can be changed through various commands includ CMDline in the transmit window.

- **favicon.ico** A small graphic icon file to shut up browsers that look always look for icons when connecting to a server. 

During development it is common for me to run VSC with a monitor window set to view serial output, a web browser connected to the ESP32 with an inspect window open looking at the console tab, and a RTL-SDR running looking at LoRa packets fly by. All necessary since changing functionality may be an asychronous process of JS, EPS32, and HTML code updates.

![HASvESP32_21130.png](docs/hasv-dev.png)

## Building and uploading SPIFFS image

Building and uploading a SPIFFS image is only necessary if you change any files in the <mark>data folder</mark>. This would occur if you are making changes to the Web UI itself. When you do make a change, build and upload the new SPIFFS image as follows:

```
pio run -t uploadfs
```

## Avoiding Library Hell

Some useful hints for developing this firmware using platformio are included here,

- It may be a good idea to delete the libdeps folder prior to rebuilding, as old, out-dated libraries could case conflits. To do this, `rm -rf .pio/libdeps`.
- If you would like to make changes to a specific library you can clone the library into the `firmware/lib` folder that is created after running `pio run` and then comment it out or remove it from the `lib_deps` list in the `platformio.ini` file.
- If you're including new libraries in the firmware, for PlatformIO, you wil need to add them to `platformio.ini` under `lib_deps`.
