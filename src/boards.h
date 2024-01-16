//
// boards.h
//
// 20231231-1000
//
//

#define CLOCK_SPEED 240

#ifdef TTGO_LORA_V1
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS 18
#define LORA_RST 14
#define LORA_IRQ 26
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#endif

#ifdef TTGO_LORA_V2
#define SD_SCK 14
#define SD_MISO 2
#define SD_MOSI 15
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST NOT_A_PIN
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#endif

#ifdef TTGO_TBEAM
// #define LORA_SCK 14
// #define LORA_MISO 2
// #define LORA_MOSI 15
// #define LORA_CS 18
// #define LORA_RST 23
// #define LORA_IRQ 26
// #define SD_SCK 14
// #define SD_MISO 2
// #define SD_MOSI 15
#define RF_PACONFIG_PASELECT_PABOOST true
#define GPS_SERIAL 9600, SERIAL_8N1, GPIO_NUM_12, GPIO_NUM_15
#endif

#ifdef HELTEC_WIFI_LORA_32_V2
// Pin definition of Heltec WIFI LoRa 32 v2.0
// HelTec AutoMation 2019 support@heltec.cn
//
#define LED 25    // GPI025 -- SX127x's LED
// LoRa
#define SCK 5     // GPIO5  -- SX127x's SCK
#define MISO 19   // GPIO19 -- SX127x's LORA_MISO
#define MOSI 27   // GPIO27 -- SX127x's LORA_MOSI
#define SS 18     // GPIO18 -- SX127x's CS
#define RST 14    // GPIO14 -- SX127x's RESET
#define DI0 26    // GPIO26 -- SX127x's IRQ(Interrupt Request)
#define PABOOST true
#define BAND 911250000  // Band setting ( 868E6,915E6 )
// OLED
//#define HAS_OLED       // Uncomment Board or Peripheral features
#define OLED_SCL 15    // GPIO15 -- OLED_SCL
#define OLED_SDA 4     // GPIO4  -- OLED_SDA
#define OLED_RST 16    // GPIO16 -- OLED_RST
// VEXT
#define VEXT 21   // GPIO21 -- VEXT_CONTROL
#endif

#ifdef HELTEC
#define LORA_IRQ 26
#define LORA_IO1 35
#define LORA_IO2 34
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_RST 14
#define LORA_CS 18
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#endif

#ifdef SPARKFUN
#define LORA_IRQ 26
#define LORA_IO1 33
#define LORA_IO2 32
#define LORA_SCK 14 SCK
#define LORA_MISO 12
#define LORA_MOSI 13
#define LORA_RST NOT_A_PIN
#define LORA_CS 16
//#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST NOT_A_PIN
#define OLED_WIDTH 128
#define OLED_HEIGHT 128
#endif

#ifdef LOPY4
// all necessary pins are defined in the arduino-esp32 variants
// https://github.com/espressif/arduino-esp32/blob/master/variants/lopy4/pins_arduino.h
// any additional board specifc settings should be added here
#define LORA_IRQ 23
#define LORA_CS 18
// to add second LoRa module connect it to these pins and specific DUAL_LORA as a build flag
#define LORA2_IRQ 26
#define LORA2_RST NOT_A_PIN
#define LORA2_CS 25
#endif