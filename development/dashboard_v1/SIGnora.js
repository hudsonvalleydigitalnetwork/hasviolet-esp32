//
//
//  SIGnora
//
//  RELEASE: 20231220-1700
//
//

//            
// VARIABLES    
//

/* Object to hold imported SIGnora.json */
var myRadio = {
	RADIO: {
		channel: "1",
		channelname: "HVDN",
		frequency: 911250,
		spreadfactor: 7,
		codingrate4: 8,
		bandwidth: 125000,
		txpwr: 7
	},
	CHANNELS: {
		0: {
			channelname: "HAS 0",
	        frequency: 911250000,
    	    spreadfactor: 7,
        	codingrate4: 8,
        	bandwidth: 125000,
	  	},
	  	1: {
        	channelname: "HAS 1",
         	frequency: 911250000,
         	spreadfactor: 7,
         	codingrate4: 8,
         	bandwidth: 125000,
	  	},
	  	2: {
         	channelname: "HAS 2",
         	frequency: 911250000,
         	spreadfactor: 7,
         	codingrate4: 8,
         	bandwidth: 125000,
	  	},
	  	3: {
         	channelname: "HAS 3",
         	frequency: 911250000,
         	spreadfactor: 7,
         	codingrate4: 8,
         	bandwidth: 125000,
	  	},
	  	4: {
         	channelname: "HAS 4",
         	frequency: 911250000,
         	spreadfactor: 7,
         	codingrate4: 8,
         	bandwidth: 125000,
	  	},
	  	5: {
         	channelname: "HAS 5",
         	frequency: 911250000,
         	spreadfactor: 7,
         	codingrate4: 8,
         	bandwidth: 125000,
	  	},
	  	6: {
         	channelname: "HAS 6",
         	frequency: 911250000,
         	spreadfactor: 7,
         	codingrate4: 8,
         	bandwidth: 125000,
	  	},
	  	7: {
         	channelname: "HAS 7",
         	frequency: 911250000,
         	spreadfactor: 7,
         	codingrate4: 8,
         	bandwidth: 125000,
	  	},
	  	8: {
         	channelname: "HAS 8",
         	frequency: 911250000,
         	spreadfactor: 7,
         	codingrate4: 8,
         	bandwidth: 125000,
	  	},
	  	9: {
         	channelname: "HAS 9",
         	frequency: 911250000,
         	spreadfactor: 7,
         	codingrate4: 8,
         	bandwidth: 125000,
	  	}
	},
	CONTACT: {
		mycall: "NOCALL",
		myssid: "99",
		mybeacon: "quack quack",
		dstcall: "BEACON",
		dstssid: "99"
	},
	MACROS: {
		M1: "QRZ",
    	M2: "QSL",
      	M3: "73",
      	M4: "TEST TEST TEST"
   	}
}

// Current MSG Exchange
var myContact = {
	mycall: "NOCALL",
	myssid: "00",
	mybeacon: "TEST TEST TEST TEST",
	dstcall: "BEACON",
	dstssid: "99",
	header: "NOCALLS-00>BEACON-99",
	sent: "",
	received: ""
}

// Programmable Channels
var myChannels = [0,1,2,3,4,5,6,7,8,9];

// Programmable Macros M1-M4
var myMacros = ["","","",""];

// Last Seven Calls heard (NO IMPLEMENTED YET
var myHeard = [ "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS"];

// Radio and Msg Module Commands
var radioBEACON = 0;                                    //  0 = Beacon OFF, 1 = Beacon ON  
var radioTXPWR = 5;                                     //  TX Power can be        5-21    
var radioLOG = 0;                                       //  0 = Beacon OFF, 1 = Beacon ON
var radioMODE = "LORA";                                 //  LORA, (futures FSK, FX.25, ...
var msgENTERED = "";									//  Typed into TX_Window      
var msgPARSED = [];										//  For processing two-part command
var msgSUBPARSED = [];									//  For processing two-part command  
var msgPARTICLE = [];									//  For processing two-part command  
var msgSENT = "";										//  msgINPUT that was TXed  
var currCHAN = 0;										//  Current Channel Number
var lastCHAN = 0;										//  Last Channel Number
var current_channel = "";								//  Current Channel Name
var previous_channel = "";								//  Last Channel Name
var previous_operation = "";
var current_operation = "";
var previous_entry = "";
var current_entry = "";
var myHostname = location.hostname;
var wsURI = "ws://" + myHostname + ":8000/ws";			//  TEST Websocket URI 
var url = "http://" + myHostname + "/SIGnora.json";	//  JSON file location
var getHasJson = new XMLHttpRequest();					//  Holds JSON from Radio
var rxDisplay = [];										//  Holds whole RX window as 27 lines of text
var rxDispY = 26;										//  Numbe of rowa for RX Window

//            
// FUNCTIONS
//

// --- Tuner-Module ---
function updateTuner(localChannelnum) {
	msgSOX = "SET:TUNER:" + myRadio.CHANNELS[localChannelnum].frequency + ":"
						 + myRadio.CHANNELS[localChannelnum].spreadfactor + ":"
						 + myRadio.CHANNELS[localChannelnum].codingrate4 + ":"
						 + myRadio.CHANNELS[localChannelnum].bandwidth;
	socket.send(msgSOX);
	var el = document.getElementById('tuner-setting');
	console.log(msgSOX);
	el.textContent = myRadio.CHANNELS[localChannelnum].channelname;
	//console.log("TUNER: changed to " + myRadio.CHANNELS[localChannelnum].channelname);
}

function btnNUM(localChannelnum) {
	previous_channel = current_channel;
	lastCHAN = currCHAN;
	currCHAN = localChannelnum;
	current_channel = myRadio.CHANNELS[localChannelnum].channelname;
	console.log("CHANNEL: change to " + current_channel);
	updateTuner(localChannelnum);
}

// --- Radio Panel-Module ---
function btnRADIO() {
	console.log("RADIO: btnRADIO: clicked");
	var el_btnRADIO = document.getElementById('btnRADIO');
	socket.send("SET:RADIO:RESET");
	console.log("RADIO: RESET");
	previous_entry = 0;
	previous_operation = "RADIO_RESET";
}

function btnTXPWR() {
	console.log("RADIO: btnTXPWR: clicked");
	var el_btnTXPWR = document.getElementById('btnTXPWR');
	if (radioTXPWR == 5) {
		radioTXPWR = 10;
		el_btnTXPWR.style.background = "MediumSeaGreen";
		el_btnTXPWR.innerHTML = "MEDM";
		socket.send("SET:TXPWR:MEDIUM");
		console.log("RADIO: btnTXPWR: MEDIUM");
		previous_entry = 0;
		previous_operation = "TXPWR_MEDIUM";
	 }
	 else if (radioTXPWR == 10){
		 radioTXPWR = 20;
		 el_btnTXPWR.style.background = "Orange";
		 el_btnTXPWR.innerHTML = "HIGH";
		 socket.send("SET:TXPWR:HIGH");
		 console.log("RADIO: btnTXPWR: HIGH");
		 previous_entry = 0;
		 previous_operation = "TXPWR_HIGH";
	 }
	 else if (radioTXPWR == 20){
		radioTXPWR = 5;
		el_btnTXPWR.style.background = "Black";
		el_btnTXPWR.innerHTML = "PA";
		socket.send("SET:TXPWR:LOW");
		console.log("RADIO: btnTXPWR: LOW");
		previous_entry = 0;
		previous_operation = "TXPWR_Low";
	}
}

function btnMODE() {
	console.log("RADIO: btnMODE: clicked");
}

function btnBEACON() {
	console.log("RADIO: btnBEACON: clicked");
	var el_btnBEACON = document.getElementById('btnBEACON');
	if (radioBEACON == 0) {
		radioBEACON = 1;
		el_btnBEACON.style.background = "MediumSeaGreen";
		socket.send("SET:BEACON:ON");
		console.log("RADIO: btnBEACON: Beacon ON");
		previous_entry = 0;
		previous_operation = "BEACON_ON";
	 }
	 else if (radioBEACON == 1){
		 radioBEACON = 0;
		 el_btnBEACON.style.background = "Black";
		 socket.send("SET:BEACON:OFF");
		 console.log("RADIO: btnBEACON: Beacon OFF");
		 previous_entry = 0;
		 previous_operation = "BEACON_OFF";
	 }
}

// --- Macros Module ---
function btnM1() {
	console.log("MACRO: btnM1: clicked");
	msgENTERED = msgENTERED + myMacros[0];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "";
	previous_operation == "M1";
	console.log("MACRO: btnM1: " + myMacros[0]);
}

function btnM2() {
	console.log("MACRO: btnM2: clicked");
	msgENTERED = msgENTERED + myMacros[1];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "";
	previous_operation == "M2";
	console.log("MACRO: btnM2: " + myMacros[1]);
}

function btnM3() {
	console.log("MACRO: btnM3: clicked");
	msgENTERED = msgENTERED + myMacros[2];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "";
	previous_operation == "M3";
	console.log("MACRO: btnM3: " + myMacros[2]);
}

function btnM4() {
	console.log("MACRO: btnM4: clicked");
	msgENTERED = msgENTERED + myMacros[3];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "";
	previous_operation == "M4";
	console.log("MACRO: btnM4: " + myMacros[3]);
}

// --- MSG Panel-Module ---
function btnLOG() {
	console.log("MSG: btnLOG: clicked");
	var el_btnLOG = document.getElementById('btnLOG');
	if (radioLOG == 0) {
		radioLOG = 1;
		el_btnLOG.style.background = "MediumSeaGreen";
		socket.send("LOG:ON");
		console.log("MSG: btnLOG: Logging ON");
		previous_entry = 0;
		previous_operation = "LOG_ON";
	 }
	 else if (radioLOG == 1){
		 radioLOG = 0;
		 el_btnLOG.style.background = "Black";
		 socket.send("LOG:OFF");
		 console.log("MSG: btnLOG: Logging OFF");
		 previous_entry = 0;
		 previous_operation = "LOG_OFF";
	 }
}

function btnRCLR() {
	console.log("MSG: btnRCLR: clicked");
	msgENTERED = "";
	msgPARSED = "";
	previous_operation = "RCLR";
	rxwinRCLR();
}

function btnCALL() {
	console.log("MSG: btnCALL: clicked");
	msgENTERED = msgENTERED + myRadio.CONTACT.mycall + "-" + myRadio.CONTACT.myssid;
	document.getElementById('msgENTRY').value = msgENTERED;
}

function btnDEST() {
	console.log("MSG: btnDEST: clicked");
	msgENTERED = msgENTERED + myRadio.CONTACT.dstcall + "-" + myRadio.CONTACT.dstssid;
	document.getElementById('msgENTRY').value = msgENTERED;
}

function btnHEAD() {
	console.log("MSG: btnHEAD: clicked");
	myContact.header = myRadio.CONTACT.mycall + "-" + myRadio.CONTACT.myssid + ">" + myRadio.CONTACT.dstcall + "-" + myRadio.CONTACT.dstssid + "|";
	msgENTERED = msgENTERED + myContact.header;
	document.getElementById('msgENTRY').value = msgENTERED;
}

function btnHELP() {
	console.log("MSG: btnHELP: clicked");
	rxwinMSGhelp();
	rxwinMSG('-');
	previous_operation = "COMMAND";
}

function btnMCLR() {
	console.log("MSG: btnMCLR: clicked");
	msgENTERED = "";
	msgPARSED = "";
	previous_operation = "MCLR";
	document.getElementById('msgENTRY').value = "";
}

function btnSEND() {
	console.log("MSG: btnSEND: clicked");
	msgENTERED = document.getElementById('msgENTRY').value;
	console.log("PANEL: btnSEND", msgENTERED);

	// --- Sort through commands first ---
	msgPARSED = msgENTERED.split(" ");
		
	if (msgPARSED[0] === ".msgBEACON") {					//Set BEACON message
		msgSUBPARSED = msgENTERED.replace('msgBEACON ', '')
		myContact.mybeacon = msgSUBPARSED;
		myRadio.CURRENT.mybeacon = myContact.mybeacon;
		message = "BEACON = " + myContact.mybeacon;
		rxwinMSG(message);
		console.log("CMD: set: Beacon MSG = " + myContact.mybeacon);
		previous_operation = "COMMAND";
	
	} else if (msgPARSED[0] === ".myCALL") {				//Set myCALL and mySSID
		msgSUBPARSED = msgPARSED[1];
		msgPARTICLE = msgSUBPARSED.split("-");
		myRadio.CONTACT.mycall = msgPARTICLE[0];
		myRadio.CONTACT.myssid = msgPARTICLE[1];
		message = "CALL = " + myRadio.CONTACT.mycall + "-" + myRadio.CONTACT.myssid;
		rxwinMSG(message);
		console.log("CMD: set: CALL =" + myRadio.CONTACT.mycall + "-" + myRadio.CONTACT.myssid);
		previous_operation = "COMMAND";
	
	} else if (msgPARSED[0] === ".dstCALL") {				//Set myRadio.CONTACT.dstcall and myRadio.CONTACT.dstssid
		msgSUBPARSED = msgPARSED[1];
		msgPARTICLE = msgSUBPARSED.split("-");
		myRadio.CONTACT.dstcall = msgPARTICLE[0];
		myRadio.CONTACT.dstssid = msgPARTICLE[1];
		message = "DEST = " + myRadio.CONTACT.dstcall + "-" + myRadio.CONTACT.dstssid;
		rxwinMSG(message);
		console.log("CMD: set: DEST =" + myRadio.CONTACT.dstcall + "-" + myRadio.CONTACT.dstssid);
		previous_operation = "COMMAND";
	
	} else if (msgPARSED[0] === ".dumpLORA") {				//  dump LoRa resgisters 
		socket.send("GET:LORA" + msgENTERED);
		rxwinMSG("GET:LORA");
		console.log("CMD: GET:LORA");
		previous_operation = "COMMAND";
	
	} else if (msgPARSED[0] === ".macro1") {				//  Set Macro Button 1 
		msgSUBPARSED = msgENTERED.replace('.macro1 ', '');
		myMacros[0] = msgSUBPARSED;
		message = "MACRO1 = " + myMacros[0];
		rxwinMSG(message);
		console.log("CMD: set: M1 =" + myMacros[0]);
		previous_operation = "COMMAND";
	
	} else if (msgPARSED[0] === ".macro2") {				//  Set Macro Button 2  
		msgSUBPARSED = msgENTERED.replace('.macro2 ', '');
		myMacros[1] = msgSUBPARSED;
		message = "MACRO3 = " + myMacros[1];
		rxwinMSG(message);
		console.log("CMD: set: PANEL: btnM2: M2 = " + myMacros[1]);
		previous_operation = "COMMAND";
	
	} else if (msgPARSED[0] === ".macro3") {				//  Set Macro Button 3  
		msgSUBPARSED = msgENTERED.replace('.macro3 ', '');
		myMacros[2] = msgSUBPARSED;
		message = "MACRO1 = " + myMacros[2];
		rxwinMSG(message);
		console.log("CMD: set: M3 = " + myMacros[2]);
		previous_operation = "COMMAND";
		
	} else if (msgPARSED[0] === ".macro4") {				//  Set Macro Button 4  
		msgSUBPARSED = msgENTERED.replace('.macro4 ', '');
		myMacros[3] = msgSUBPARSED;
		message = "MACRO1 = " + myMacros[3];
		rxwinMSG(message);
		console.log("CMD: set: M4 = " + myMacros[3]);
		previous_operation = "COMMAND";

	// Anything else is a message to be TX
	} else {
		socket.send("TX:" + msgENTERED);
		rxwinMSG("TX:"+ msgENTERED);
		console.log("CMD: TX:" + msgENTERED);
		previous_operation = "SENT";
	}

	msgENTERED = "";
	msgPARSED = "";
	previous_operation = "SENT";
	document.getElementById('msgENTRY').value = "";

}

// --- Receiver-Window --- 
function rxwinRCLR() {
	console.log("rxWIN: RCLR");
	for (i = rxDispY; i > 0; i=i-1) {
		rxDisplay[i] = "-";
		var rxWINid = "rxWIN" + i;
		var el_RXwin = document.getElementById(rxWINid);
		el_RXwin.innerHTML = rxDisplay[i];
	  }
	  rxDisplay[0] = "-";
	  var el_RXwin = document.getElementById("rxWIN0");
	  el_RXwin.innerHTML = rxDisplay[0];
}

function rxwinMSG(message) {
  for (i = rxDispY; i > 0; i=i-1) {
	rxDisplay[i] = rxDisplay[i-1];
	var rxWINid = "rxWIN" + i;
    var el_RXwin = document.getElementById(rxWINid);
	el_RXwin.innerHTML = rxDisplay[i];
  }
  rxDisplay[0] = message;
  var el_RXwin = document.getElementById("rxWIN0");
  el_RXwin.innerHTML = message;
}

function rxwinMSGhelp() {
	rxDisplay [26] = "-"
	rxDisplay [25] = "----=======---- SIGnora Instructions ----=======----"
	rxDisplay [24] = "-"
	rxDisplay [23] = "- KEYPAD:"
	rxDisplay [22] = "- 1 through 9 = HVDN RF Channels 1 through 9 "
	rxDisplay [21] = "-"
	rxDisplay [20] = "- BUTTONS:"
	rxDisplay [19] = "- RADIO = Radio on/off"
	rxDisplay [18] = "- BECN = Beacon msg on/off"
	rxDisplay [17] = "- M1 through M4 = Macros 1 though 4"
	rxDisplay [16] = "- CALL = myCall Macro ***** LOG = Log on/off (toggle)"
  	rxDisplay [15] = "- DEST = dstCall Macro **** RCLR = Clear RX Window"
  	rxDisplay [14] = "- HEAD = Message Header *** MCLR = TX window clear"
  	rxDisplay [13] = "- HELP = This screen ****** SEND = Send whats in TX Window"
  	rxDisplay [12] = "- "
	rxDisplay [11] = "- COMMAND WINDOW"
	rxDisplay [10] = "- .myCALL NOCALL-00 = Changes CALL macro to hold NOCALL-00"
  	rxDisplay [9] = "- .dstCALL BL0B-50 = Changes DEST macro to hold BL0B-50"
  	rxDisplay [8] = "- .macro1 blah blah =  Changes M1 button to hold blah blah"
  	rxDisplay [7] = "-"
	rxDisplay [6] = "- TO SEND A MESSAGE:"
	rxDisplay [5] = "- Via Buttons -- click HEAD, M1, then SEND"
	rxDisplay [4] = "- Via CMDline -- YourCall>DestCall|MESSAGE then SEND"
	rxDisplay [3] = "-"
	rxDisplay [2] = "----=======---- ---=====---=---=====--- ----=======----"
	rxDisplay [1] = " "
	rxDisplay [0] = " "
}

// --- Transmitter-Module ---
function msgENTRY() {
	// Get the focus to the text input to enter a word right away.
	document.getElementById('msgENTRY').focus();

	
	// Getting the text from the input
	var msgENTERED = document.getElementById('msgENTRY').value;
  }


//
// MAIN
//

// Get SIGnora.json on page load
getHasJson.open('GET', url, true);
getHasJson.send(null);
console.log("INIT: radioCONFIG retrieved");
getHasJson.onload = function() {
	if (getHasJson.readyState === getHasJson.DONE && getHasJson.status === 200) {
		myRadio = JSON.parse(getHasJson.responseText)
	}
}
console.log("INIT: myRadio Object Loaded");

// Channel assignments
myChannelsIndex = Object.keys(myRadio.CHANNELS);	// Need Keys as Properties for easy function reference
//myRadio.CHANNELS.channel = myChannels[1];         // Set HOME button channel
//console.log("INIT: Channels CH1-CH3 set as " + myChannels[1], myChannels[2],  myChannels[3]);
//console.log("INIT: Channels CH4-CH6 set as " + myChannels[4], myChannels[5],  myChannels[6]);
//console.log("INIT: Channels CH7-CH9 set as " + myChannels[7], myChannels[8],  myChannels[9]);

//myRadio.CHANNELS.channel = myChannels[1];           // Set HOME button channel
console.log("INIT: Channels CH1-CH3 set as " + myRadio.CHANNELS[1].channelname, myRadio.CHANNELS[2].channelname, myRadio.CHANNELS[3].channelname);
console.log("INIT: Channels CH4-CH6 set as " + myRadio.CHANNELS[3].channelname, myRadio.CHANNELS[4].channelname, myRadio.CHANNELS[6].channelname);
console.log("INIT: Channels CH7-CH9 set as " + myRadio.CHANNELS[7].channelname, myRadio.CHANNELS[8].channelname, myRadio.CHANNELS[9].channelname);


// Set Macros M1 to M4
myMacros[0] = myRadio.MACROS.M1;   		            // Beacon Message
myMacros[1] = myRadio.MACROS.M2;  
myMacros[2] = myRadio.MACROS.M3;  
myMacros[3] = myRadio.MACROS.M4;
console.log("INIT: Macros M1-M4 set as " + myMacros[0] + "|" + myMacros[1] + "|" + myMacros[2] + "|" + myMacros[3]);

console.log("INIT: Hostname  :" + myHostname);
console.log("INIT: Websocket :" + wsURI);
console.log("INIT: COMPLETE");
previous_entry = "";  
previous_operation = "INIT";                        // Set last operation

// Load RX Window with Help Message
rxwinMSGhelp();
rxwinMSG('-');

// Establish Websocket 
socket = new WebSocket(wsURI);

socket.onopen = function(e) {
  console.log("ws: [open] Connection established");
  console.log("ws: Sending to server");
  socket.send("CONNECT:" + myHostname);
};

socket.onmessage = function(event) {
	var hasRXD = event.data.split(':');
	if (hasRXD[0] == "RX") {
		rxwinMSG("RX: " + hasRXD[1]);
		console.log("ws: RX: " + hasRXD[1]);
	};
	console.log(`ws: [message] Data received from server: ${event.data}`);
};

socket.onclose = function(event) {
  if (event.wasClean) {
    console.log(`ws: [close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
  } else {
    // e.g. server process killed or network down
    // event.code is usually 1006 in this case
    console.log('ws: [close] Connection died');
  }
};

socket.onerror = function(error) {
	var rxWINerror = "ws:ERROR connecting to " + wsURI;
	rxwinMSG(rxWINerror);
	console.log(`ws: [error] ${error.message}`);
};  
