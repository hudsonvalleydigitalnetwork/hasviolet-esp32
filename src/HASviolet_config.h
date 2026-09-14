//
// HASviolet_config.h
//
// ** Release:  201210129-1330
//
// WiFi AP Creds
#define WIFI_APSSID "HASviolet"
// Must be 8-63 chars -- WPA2-PSK's minimum. Confirmed on real hardware (a
// Heltec V3) that WiFi.softAP() silently rejects anything shorter
// ("passphrase too short!" on Serial) and initWiFi() didn't check its return
// value, so the board would carry on claiming "WiFi AP initialized" with no
// AP actually up. "purple" (6 chars) was the previous value.
#define WIFI_APKEY "purple99"

//Uncomment and edit the next two lines if connecting to existing WiFi network
#define WIFI_SSID "HomeWAN"
#define WIFI_KEY "2125551212"

//Client credentials
#define WWW_USER "radio"
#define WWW_KEY "radio"