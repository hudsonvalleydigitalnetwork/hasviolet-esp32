//
// HASviolet_cmdmsgs.h
//
// ** Release:  20210311-1330
//

//
// RPi
//

//  Websocket client commands
//  All commands are echoed back by server prefixed with ACK replacing SET and GET
#define RADIO_RESET "SET:RADIO:RESET"
#define GET_LORA "GET:LORA"
#define BEACON_ON "SET:BEACON:ON"
#define BEACON_OFF "SET:BEACON:OFF"
#define TX_LOW "SET:TXPWR:LOW"
#define TX_MEDIUM "SET:TXPWR:MEDIUM"
#define TX_HIGH "SET:TXPWR:HIGH"
#define SET_TUNER "SET:TUNER:"       // args are modemconfig and frequency
#define TX_MESSAGE "TX:"             // "TX: blablahblah"

//  Websocket server originated messages
//  Broadcasted to all connected clients
#define RX_MESSAGE "RX:"             // "RX: blahblahblah"


//
// ESP32
//

//  Websocket client commands
//  All commands are echoed back by server prefixed with "ACK:"All commands are echoed back by server prefixed with ACK replacing SET and GET
#define RADIO_RESET "SET:RADIO:RESET"
#define GET_LORA "GET:LORA"
#define BEACON_ON "SET:BEACON:ON"
#define BEACON_OFF "SET:BEACON:OFF"
#define TX_LOW "SET:TXPWR:LOW"
#define TX_MEDIUM "SET:TXPWR:MEDIUM"
#define TX_HIGH "SET:TXPWR:HIGH"
#define SET_TUNER "SET:TUNER:"       // args are freq, SF, CR4, and BW
#define TX_MESSAGE "TX:"             // "TX: blablahblah"

//  Websocket server originated messages
//  Broadcasted to all connected clients
#define RX_MESSAGE "RX:"             // "RX: blahblahblah |RSSI: "
#define BEACON_BROADCAST "BEACON:"   // "BEACON: blahblahblah"

