//
// api.h
//
// 20231231-1000
//

//
//  Websocket API
// 
//  Client initiates commands with SET and GET. Server acknowledges commands
//  by sending the command received replacing SET and GET with ACK.
//

//
//   RADIO
//
#define RADIO_RESET "SET:RADIO:RESET"
#define RADIO_LOG_ON "SET:RADIO:LOG:ON"
#define RADIO_LOG_OFF "SET:RADIO:LOG:OFF"
//
#define RADIO_FREQ_SET "SET:RADIO:FREQ:"          // ex.  SET:RADIO:FREQ:911250000  (Freq in Hz)
#define RADIO_FREQ_GET "GET:RADIO:FREQ"
//
#define RADIO_PWR_LOW "SET:RADIO:PWR:LOW"
#define RADIO_PWR_MED "SET:RADIO:PWR:MEDIUM"
#define RADIO_PWR_HIGH "SET:RADIO:PWR:HIGH"
#define RADIO_PWR_GET "GET:RADIO:PWR"
//
#define RADIO_BEACON_ON "SET:RADIO:BEACON:ON"
#define RADIO_BEACON_OFF "SET:RADIO:BEACON:OFF"
#define RADIO_BEACON_MSG "SET:RADIO:BEACON:MSG:"  // ex. SET:RADIO:BEACON:MSG:10-4 Good Buddy
//
#define RADIO_LORA "SET:RADIO:LORA"
#define RADIO_FSK "SET:RADIO:FSK"

//
//  LORA
//
#define LORA_SF_SET "SET:LORA:SPREAD:"            // ex.  SET:LORA:SF:7  
#define LORA_SF_GET "GET:LORA:SPREAD"
#define LORA_CR_SET "SET:LORA:CODING:"            // ex.  SET:LORA:CR:8
#define LORA_CR_GET "GET:LORA:CODING"
#define LORA_BW_SET "SET:LORA:BANDWIDTH:"         // ex.  SET:LORA:BW:125000   (Freq in Hz)
#define LORA_BW_GET "SET:LORA:BANDWIDTH" 

//
//  HEADER
//
#define HEADER_SOURCE "SET:HEAD:SOURCE:"          // ex.  SET:HEAD:SOURCE:N0CALL
#define HEADER_DEST "SET:HEAD:DEST:"              // ex.  SET:HEAD:DEST:W1AW  
#define HEADER_TYPE "SET:HEAD:TYPE:"     

//
// MESSAGES
//
#define TX_MESSAGE "TX:"                          // "TX: blablahblah"
#define RX_MESSAGE "RX:"                          // "RX: blahblahblah |RSSI: "

