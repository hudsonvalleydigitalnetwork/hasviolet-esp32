/*
#
#   Client Javascript
#
#   20231231-1000
#
#
*/

/* -            
/* - VARIABLES    
/* -
*/

/* Object to hold imported JSON file */
var myRadio = {
	RADIO: {
		channel: 1,
		channelname: "HV1",
		rfmodule: "RFM9X",
		modemconfig: "Bw125Cr45Sf128",
		modem: 0,
		frequency: 911250000,
		spreadfactor: 7,
		codingrate4: 8,
		bandwidth: 125000,
		txpwr: 10
	},
	HID: {
		oled: "128x32"
	},
	CONTACT: {
		mycall: "NOCALL",
		myssid: "00",
		mybeacon: "quack quack",
		dstcall: "BEACON",
		dstssid: "99"
	},
	CMACROS: {
		C1: "BLAH",
		C2: "BLAH BLAH",
		C3: "BLAH BLAH BLAH",
		C4: "BLAH BLAH BLAH BLAH"
	},
	MACROS: {
		M1: "QRZ",
		M2: "QSL",
		M3: "73",
		M4: "TEST TEST TEST"
	},
	LORA: {
		spreadfactor: 7,
		codingrate4: 8,
		bandwidth: 125000,
		frequency: 911250000
	},
	CHANNELS: {
		0: {channelname: "HV0", modem: 0, frequency: 911250000},
		1: {channelname: "HV1", modem: 0, frequency: 911250000},
		2: {channelname: "HV2", modem: 2, frequency: 911250000},
		3: {channelname: "HV3", modem: 3, frequency: 911250000},
		4: {channelname: "HV4", modem: 4, frequency: 911250000},
		5: {channelname: "HV5", modem: 1, frequency: 911250000},
		6: {channelname: "HV6", modem: 2, frequency: 911300000},
		7: {channelname: "HV7", modem: 3, frequency: 911300000},
		8: {channelname: "HV8", modem: 0, frequency: 911300000},
		9: {channelname: "HV9", modem: 1, frequency: 911300000}
	},
	MODEMS: {
		0:{modemconfig: "Bw125Cr45Sf128", modemname: "MEDIUM", spreadfactor: 7, codingrate4: 8, bandwidth: 125000},
		1:{modemconfig: "Bw500Cr45Sf128", modemname: "FAST-SHORT", spreadfactor: 7, codingrate4: 5, bandwidth: 500000},
		2:{modemconfig: "Bw31_25Cr48Sf512", modemname: "SLOW-LONG1", spreadfactor: 7, codingrate4: 8, bandwidth: 31250},
		3:{modemconfig: "Bw125Cr48Sf4096", modemname: "SLOW-LONG2", spreadfactor: 12, codingrate4: 8, bandwidth: 125000},
		4:{modemconfig: "Bw125Cr45Sf2048", modemname: "SLOW-LONG3", spreadfactor: 8, codingrate4: 5, bandwidth: 125000}
	}
}

/* Programmable Channels */
var loraChannels = [0,1,2,3,4,5,6,7,8,9];

/* Selectable LoRa modems */
var loraModems = [0,1,2,3,4];

/* Programmable Commands C1-C4 */
var myCMacros = ["SET:RADIO:RESET","SET:MODE:LORA","SET:MODE:FSK","SET:RADIO:EXPERT"];

/* Programmable Macros M1-M4 */
var myMacros = ["QRZ","QSL","73","TEST TEST TEST"];

/* Last Seven Calls heard */
var myHeard = [ "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS", "NOCALLS"];

/* Current MSG Exchange*/
var myPacket = {
	header: "NOCALL-00>BEACON-99",
	options: "",
	message: ""
}

/* Radio and Msg Module Commands */
var radioGUI = 0;                                                //  0 = Basic, 1 = Expert
var radioFUNCT = 0;                                              //  0 = Listen, 1 = Repeat, 2 = Beacon
var radioBEACON = 0;                                             //  0 = Beacon OFF, 1 = Beacon ON  
var radioTXPWR = 5;                                              //  TX Power can be 5 to 21    
var radioLOG = 0;                                                //  0 = Logging OFF, 1 = Logging ON
var radioMODE = 0;                                               //  0 = LORA, 1 = XARPS, 2 = FSK
var msgENTERED = "";				   	        			     //  Typed into TX_Window      
var msgPARSED = [];					      		    			 //  For processing two-part command
var msgSUBPARSED = [];			     	     					 //  For processing two-part command  
var msgPARTICLE = [];		    			    				 //  For processing two-part command  
var previous_operation = "";
var current_operation = "";
var previous_entry = "";
var current_entry = "";
var myHostname = location.hostname;
var wsURI = "wss://" + myHostname + ":8000/wss";		         //  TEST Websocket URI 
var urlJSON = "https://" + myHostname + ":8000/config.json";    //  JSON file location
var getHasJson = new XMLHttpRequest();	     					 //  Holds JSON from Radio
var rxDisplay = [];							     				 //  Holds whole RX window as 27 lines of text
var rxDispY = 26;								    			 //  Numbe of rowa for RX Window

/* Dashboard Page Elements */
var tuner_setting_element = document.getElementById('tuner-setting');
var spreadfactor_setting_element = myRadio.LORA.spreadfactor;
var codingrate4_setting_element = myRadio.LORA.codingrate4;
var bandwidth_setting_element = myRadio.LORA.bandwidth;
var channelname_setting_element = document.getElementById('channelname-setting');

/* -            
/* - FUNCTIONS
/* -
*/

// --- Tuner-Module ---

function updateDisplay() {
	tuner_setting_element = document.getElementById('tuner-setting');
	tuner_setting_element.innerText = (myRadio.RADIO.frequency/1000000) + " MHz";
	spreadfactor_setting_element = document.getElementById('spreadfactor-setting');
	spreadfactor_setting_element.innerText = "SF " + myRadio.LORA.spreadfactor;
	codingrate4_setting_element = document.getElementById('codingrate4-setting');
	codingrate4_setting_element.innerText = "CR " + myRadio.LORA.codingrate4;
	bandwidth_setting_element = document.getElementById('bandwidth-setting');
	bandwidth_setting_element.innerText= "BW " + (myRadio.LORA.bandwidth/1000);
	channelname_setting_element = document.getElementById('channelname-setting');
	if (radioGUI == 0) {
		channelname_setting_element.innerText= "CH " + myRadio.RADIO.channelname;
	}
	else {
		channelname_setting_element.innerText= " ";
	}
	console.log("LOCAL:DISPLAY:UPDATED");
}

function updateFreqDisplay(myNum) {
	var el = document.getElementById('tuner-setting');
	el.textContent = myNum;
	console.log("LOCAL:DISPLAY:TUNER:" + myNum);
}

function updateTuner() {
	msgSOX = "SET:RADIO:FREQ:" + myRadio.RADIO.frequency;
	socket.send(msgSOX);
	console.log(msgSOX);
	msgSOX = "SET:LORA:BANDWIDTH:" + myRadio.LORA.bandwidth;
	socket.send(msgSOX);
	console.log(msgSOX);
	msgSOX = "SET:LORA:SPREAD:" + myRadio.LORA.spreadfactor;
	socket.send(msgSOX);
	console.log(msgSOX);
	msgSOX = "SET:LORA:CODING:" + myRadio.LORA.codingrate4;
	socket.send(msgSOX);
	console.log(msgSOX);
}

function btnNUM(numberPad) {
	if (radioGUI == 0) {
		myRadio.RADIO.channelname  = myRadio.CHANNELS[numberPad].channelname;
		myRadio.RADIO.modem = myRadio.CHANNELS[numberPad].modem;
		myRadio.RADIO.frequency = myRadio.CHANNELS[numberPad].frequency;
		myRadio.LORA.codingrate4 = myRadio.MODEMS[myRadio.CHANNELS[numberPad].modem].codingrate4;
		myRadio.LORA.bandwidth = myRadio.MODEMS[myRadio.CHANNELS[numberPad].modem].bandwidth;
		myRadio.LORA.spreadfactor = myRadio.MODEMS[myRadio.CHANNELS[numberPad].modem].spreadfactor;
		console.log("SET:RADIO:CHANNEL:" + myRadio.RADIO.channelname);
		updateTuner();
		updateDisplay();
	}
	if (radioGUI == 1) {
		current_entry = numberPad;
		if (previous_operation !== "DIGIT") {
			//Fresh Display
			previous_entry = numberPad;
			previous_operation = "DIGIT";
			console.log("LOCAL:NUMPAD: first number " + numberPad);
			updateFreqDisplay();
		}
		else if (previous_operation == "DIGIT") {
			//Existing Digit
			current_entry += null;
			current_entry = `${previous_entry}${current_entry}`;
			previous_entry = current_entry;
			console.log("LOCAL:NUMPAD: next number " + numberPad);
			updateFreqDisplay(current_entry);
		}
		else {
			//Invalid
			current_entry = previous_entry;
			updateDisplay();
		}
	}
}

function btnDEC() {
	console.log("LOCAL:btnDEC:clicked");
}

function btnHASH() {
	console.log("LOCAL:btnHASH:clicked");
}

function btnFUN() {
	console.log("LOCAL:btnFUN:clicked");
}

function btnRESET() {
	if (radioGUI == 1) {
		getHasJson.open('GET', urlJSON, true);
		getHasJson.send(null);
		getHasJson.onload = function() {
			if (getHasJson.readyState === getHasJson.DONE && getHasJson.status === 200) {
				myRadio = JSON.parse(getHasJson.responseText);
				console.log("LOCAL:btnRESET");
			}
			previous_operation = "RESET";
			previous_entry = "";
			current_entry = "";
			updateDisplay(myRadio.CURRENT.channel);preadfactor-setting
		}
	}
}

function btnCLEAR() {
	if (radioGUI == 1) {
    	var el = document.getElementById('tuner-setting');
		el.textContent = 0;
		previous_entry = "";
		previous_operation == "CLEAR";
		console.log("LOCAL:btnCLEAR");
	}
}

function btnENTER() {
	if (radioGUI == 1) {
		myRadio.RADIO.frequency = current_entry;
		console.log("LOCAL:btnENTER");
		previous_operation = "ENTER";
		previous_entry = "";
		current_entry = "";
		updateTuner(myRadio.RADIO.frequency);
		updateDisplay();
	}
}


/* --- Radio Panel-Module --- */

function btnRADIO() {
	console.log("LOCAL:btnRADIO:clicked");
	var el_btnRADIO = document.getElementById('btnRADIO');
	if (radioFUNCT == 0) {
		radioFUNCT = 1;
		el_btnRADIO.style.background = "Orange";
		el_btnRADIO.innerHTML = "REPT";
		socket.send("SET:RADIO:REPEAT");
		console.log("SET:RADIO:REPEAT");
		previous_entry = 1;
		previous_operation = "SET:RADIO:REPEAT";
	 }
	 else if (radioFUNCT == 1){
		radioFUNCT = 2;
		el_btnRADIO.style.background = "MediumSeaGreen";
		el_btnRADIO.innerHTML = "BECN";
		socket.send("SET:RADIO:BEACON");
		console.log("SET:RADIO:BEACON");
		previous_entry = 2;
		previous_operation = "SET:RADIO:BEACON";
	}
	else if (radioFUNCT == 2){
		radioFUNCT = 0;
		el_btnRADIO.style.background = "Black";
		el_btnRADIO.innerHTML = "TRX";
		socket.send("SET:RADIO:TRX");
		console.log("SET:RADIO:TRX");
		previous_entry = 0;
		previous_operation = "SET:RADIO:TRX";
	}
 }

function btnTXPWR() {
	console.log("LOCAL:btnTXPWR:clicked");
	var el_btnTXPWR = document.getElementById('btnTXPWR');
	if (radioTXPWR == 5) {
		radioTXPWR = 10;
		el_btnTXPWR.style.background = "MediumSeaGreen";
		el_btnTXPWR.innerHTML = "MEDM";
		socket.send("SET:RADIO:PWR:MEDIUM");
		console.log("SET:RADIO:PWR:MEDIUM");
		previous_entry = 0;
		previous_operation = "SET:RADIO:TXPWR:MEDIUM";
	 }
	 else if (radioTXPWR == 10){
		 radioTXPWR = 20;
		 el_btnTXPWR.style.background = "Orange";
		 el_btnTXPWR.innerHTML = "HIGH";
		 socket.send("SET:RADIO:TXPWR:HIGH");
		 console.log("SET:RADIO:TXPWR:HIGH");
		 previous_entry = 0;
		 previous_operation = "SET:RADIO:TXPWR:HIGH";
	 }
	 else if (radioTXPWR == 20){
		radioTXPWR = 5;
		el_btnTXPWR.style.background = "Black";
		el_btnTXPWR.innerHTML = "LOW";
		socket.send("SET:RADIO:TXPWR:LOW");
		console.log("SET:RADIO:TXPWR:LOW");
		previous_entry = 0;
		previous_operation = "SET:RADIO:TXPWR:LOW";
	}
}

function btnMODE() {
	console.log("LOCAL:btnMODE:clicked");
	var el_btnMODE = document.getElementById('btnMODE');
	if (radioMODE == 0) {
		radioMODE = 1;
		el_btnMODE.style.background = "Orange";
		el_btnMODE.innerHTML = "XARPS";
		socket.send("SET:RADIO:XARPS");
		console.log("SET:RADIO:XARPS");
		previous_entry = 1;
		previous_operation = "SET:RADIO:XARPS";
	 }
	 else if (radioMODE == 1){
		radioMODE = 2;
		el_btnMODE.style.background = "MediumSeaGreen";
		el_btnMODE.innerHTML = "FSK";
		socket.send("SET:RADIO:FSK");
		console.log("SET:RADIO:FSK");
		previous_entry = 2;
		previous_operation = "SET:RADIO:FSK";
	}
	else if (radioMODE == 2){
		radioMODE = 0;
		el_btnMODE.style.background = "Black";
		el_btnMODE.innerHTML = "LORA";
		socket.send("SET:RADIO:LORA");
		console.log("SET:RADIO:LORA");
		previous_entry = 0;
		previous_operation = "SET:RADIO:LORA";
	}
}

function btnGUI() {
	console.log("LOCAL:btnGUI:clicked");
	var el_btnGUI = document.getElementById('btnGUI');
	var el_btnFREQ = document.getElementById('btnFREQ');
	var el_btnSF = document.getElementById('btnSF');
	var el_btnBW = document.getElementById('btnBW');
	var el_btnCR = document.getElementById('btnCR');
	if (radioGUI == 0) {
		radioGUI = 1;
		el_btnGUI.style.background = "Orange";
		el_btnGUI.innerHTML = "EXP";
		el_btnFREQ.style.background = "Black";
		channelname_setting_element.innerText= " ";
		el_btnSF.style.background = "Black";
		el_btnBW.style.background = "Black";
		el_btnCR.style.background = "Black";
		el_btnFREQ.innerHTML = " ";
		el_btnSF.innerHTML = "SF";
		el_btnBW.innerHTML = "BW";
		el_btnCR.innerHTML = "CR";
		console.log("LOCAL:btnGUI:EXPERT");
		previous_entry = 1;
		previous_operation = "LOCAL:btnGUI:clicked";
	} 
	else {
		radioGUI = 0;
		el_btnGUI.style.background = "Black";
		el_btnGUI.innerHTML = "BASIC";
		el_btnFREQ.style.background = "Black";
		channelname_setting_element.innerText= "CH " + myRadio.RADIO.channelname;
		el_btnSF.style.background = "Black";
		el_btnBW.style.background = "Black";
		el_btnCR.style.background = "Black";
		el_btnFREQ.innerHTML = " ";
		el_btnSF.innerHTML = " ";
		el_btnBW.innerHTML = " ";
		el_btnCR.innerHTML = " ";
		console.log("LOCAL:btnGUI:BASIC");
		previous_entry = 1;
		previous_operation = "LOCAL:btnGUI:clicked";
	}
}

function btnFREQ() {
	console.log("LOCAL:btnFREQ:clicked");
}

function btnBEACON() {
	console.log("LOCAL:btnBEACON:clicked");
	var el_btnBEACON = document.getElementById('btnBEACON');
	if (radioBEACON == 0) {
		radioBEACON = 1;
		el_btnBEACON.style.background = "MediumSeaGreen";
		socket.send("SET:RADIO:BEACON:ON");
		console.log("SET:RADIO:BEACON:ON");
		previous_entry = 0;
		previous_operation = "SET:RADIO:BEACON:ON";
	 }
	 else if (radioBEACON == 1){
		 radioBEACON = 0;
		 el_btnBEACON.style.background = "Black";
		 socket.send("SET:RADIO:BEACON:OFF");
		 console.log("SET:RADIO:BEACON:OFF");
		 previous_entry = 0;
		 previous_operation = "SET:RADIO:BEACON:OFF";
	 }
}

function btnSF() {
	console.log("LOCAL:btnSF:clicked");
	if (radioGUI == 1) {
		myRadio.LORA.spreadfactor++;
		if (myRadio.LORA.spreadfactor > 12) {
			myRadio.LORA.spreadfactor = 7
		}
		socket.send("SET:LORA:SPREAD:" + myRadio.LORA.spreadfactor);
		console.log("SET:LORA:SPREAD:" + myRadio.LORA.spreadfactor);
		previous_entry = 1;
		previous_operation = "SET:LORA:SPREAD:" + myRadio.LORA.spreadfactor;
		updateDisplay()
	}
}

function btnBW() {
	console.log("LOCAL:btnBW: clicked");
	if (radioGUI == 1) {
		if (myRadio.LORA.bandwidth == 7800) {
			myRadio.LORA.bandwidth = 10400;
		}
		else if (myRadio.LORA.bandwidth == 10400) {
			myRadio.LORA.bandwidth = 15600;
		}
		else if (myRadio.LORA.bandwidth == 15600) {
			myRadio.LORA.bandwidth = 20800;
		}
		else if (myRadio.LORA.bandwidth == 20800) {
			myRadio.LORA.bandwidth = 31250;
		}
		else if (myRadio.LORA.bandwidth == 31250) {
			myRadio.LORA.bandwidth = 41700;
		}
		else if (myRadio.LORA.bandwidth == 41700) {
			myRadio.LORA.bandwidth = 62500;
		}
		else if (myRadio.LORA.bandwidth == 62500) {
			myRadio.LORA.bandwidth = 125000;
		}
		else if (myRadio.LORA.bandwidth == 125000) {
			myRadio.LORA.bandwidth = 250000;
		}
		else if (myRadio.LORA.bandwidth == 250000) {
			myRadio.LORA.bandwidth = 500000;
		}
		else if (myRadio.LORA.bandwidth == 512000) {
				myRadio.LORA.bandwidth = 7800;
		}
		socket.send("SET:LORA:BANDWIDTH:" + myRadio.LORA.bandwidth);
		console.log("SET:LORA:BANDWIDTH: " + myRadio.LORA.bandwidth);
		previous_entry = 1;
		previous_operation = ("SET:LORA:BANDWIDTH:" + myRadio.LORA.bandwidth);
		updateDisplay()
	}
}

function btnCR() {
	console.log("LOCAL:btnCR:clicked");
	if (radioGUI == 1) {
		myRadio.LORA.codingrate4++;
		if (myRadio.LORA.codingrate4 > 8) {
			myRadio.LORA.codingrate4 = 5
		}
		socket.send("SET:LORA:CODING:" + myRadio.LORA.codingrate4);
		console.log("SET:LORA:CODING: " + myRadio.LORA.codingrate4);
		previous_entry = 1;
		previous_operation = "SET:LORA:CODING:" + myRadio.LORA.codingrate4;
		updateDisplay()
	}
}

/* --- Macros Module --- */

function btnC1() {
	msgENTERED = msgENTERED + myCMacros[0];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "C1";
	previous_operation == "CLICK";
	console.log("MACRO: btnC1: " + myCMacros[0]);
}

function btnC2() {
	msgENTERED = msgENTERED + myCMacros[1];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "C2";
	previous_operation == "CLICK";
	console.log("MACRO: btnC2: " + myCMacros[1]);
}

function btnC3() {
	msgENTERED = msgENTERED + myCMacros[2];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "C3";
	previous_operation == "CLICK";
	console.log("MACRO: btnC3: " + myCMacros[2]);
}

function btnC4() {
	msgENTERED = msgENTERED + myCMacros[3];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "C4";
	previous_operation == "CLICK";
	console.log("MACRO: btnC4: " + myCMacros[3]);
}

function btnM1() {
	msgENTERED = msgENTERED + myMacros[0];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "M1";
	previous_operation == "CLICK";
	console.log("MACRO: btnM1: " + myMacros[0]);
}

function btnM2() {
	msgENTERED = msgENTERED + myMacros[1];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "M2";
	previous_operation == "CLICK";
	console.log("MACRO: btnM2: " + myMacros[1]);
}

function btnM3() {
	msgENTERED = msgENTERED + myMacros[2];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "M3";
	previous_operation == "CLICK";
	console.log("MACRO: btnM3: " + myMacros[2]);
}

function btnM4() {
	msgENTERED = msgENTERED + myMacros[3];
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "M4";
	previous_operation == "CLICK";
	console.log("MACRO: btnM4: " + myMacros[3]);
}


/* --- MSG Panel-Module --- */

function btnLOG() {
	var el_btnLOG = document.getElementById('btnLOG');
	if (radioLOG == 0) {
		radioLOG = 1;
		el_btnLOG.style.background = "MediumSeaGreen";
		socket.send("SET:RADIO:LOG:ON");
		console.log("LOCAL:btnLOG:clicked");
		previous_entry = "SET:RADIO:LOG:ON";
		previous_operation = "SET:RADIO:LOG:ON";
	 }
	 else if (radioLOG == 1){
		 radioLOG = 0;
		 el_btnLOG.style.background = "Black";
		 socket.send("SET:RADIO:LOG:OFF");
		 console.log("LOCAL:btnLOG:clicked");
		 previous_entry = "SET:RADIO:LOG:OFF";
		 previous_operation = "SET:RADIO:LOG:OFF";
	 }
}

function btnRCLR() {
	console.log("LOCAL:btnRCLR:clicked");
	msgENTERED = "";
	msgPARSED = "";
	previous_entry = "RCLR";
	previous_operation == "CLICK";
	console.log("LOCAL:btnRCLR:clicked");
	rxwinRCLR();
}

function btnCALL() {
	console.log("LOCAL:btnCALL:clicked");
	msgENTERED = msgENTERED + myRadio.CONTACT.mycall + "-" + myRadio.CONTACT.myssid;
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "CALL";
	previous_operation == "CLICK";
	console.log("LOCAL:btnCALL:" + msgENTERED);
}

function btnDEST() {
	console.log("LOCAL:btnDEST:clicked");
	msgENTERED = msgENTERED + myRadio.CONTACT.dstcall + "-" + myRadio.CONTACT.dstssid;
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "DEST";
	previous_operation == "CLICK";
	console.log("LOCAL:btnDEST:" + msgENTERED);
}

function btnHEAD() {
	console.log("LOCAL:btnHEAD:clicked");
	myPacket.header = myRadio.CONTACT.mycall+ "-" + myRadio.CONTACT.myssid + ">" + myRadio.CONTACT.dstcall + "-" + myRadio.CONTACT.dstssid + "|";
	msgENTERED = msgENTERED + myPacket.header;
	document.getElementById('msgENTRY').value = msgENTERED;
	previous_entry = "HEAD";
	previous_operation == "CLICK";
	console.log("LOCAL:btnHEAD:" + msgENTERED);
}

function btnHELP() {
	if (radioGUI == 0) {
		rxwinMSGhelp();
		console.log("LOCAL:btnHELP:clicked");
		console.log("LOCAL:btnHELP:BASIC");
	}
	if (radioGUI == 1) {
		rxwinMSGexperthelp();
		console.log("LOCAL:btnHELP:clicked");
		console.log("LOCAL:btnHELP:EXPERT");
	}
	rxwinMSG('-');
	previous_entry = "HELP";
	previous_operation == "CLICK";
}

function btnMCLR() {
	msgENTERED = "";
	msgPARSED = "";
	previous_entry = "MCLR";
	previous_operation == "CLICK";
	document.getElementById('msgENTRY').value = "";
	console.log("LOCAL:btnMCLR:clicked");
}

function btnSEND() {
	msgENTERED = document.getElementById('msgENTRY').value;
	previous_entry = "CMDLINE";
	previous_operation == "CLICK";
	console.log("LOCAL:btnSEND:" + msgENTERED);

	/* --- Sort through commands first --- */
	
	msgPARSED = msgENTERED.split(" ");
		
	if (msgPARSED[0] === ".msgBECN") {                    /*  Set BEACON message  */
		msgSUBPARSED = msgENTERED.replace('msgBECN ', '')
		myRadio.RADIO.mybeacon = msgSUBPARSED;
		message = "BEACON = " + myRadio.RADIO.mybeacon;
		rxwinMSG(message);
		console.log("SET:RADIO:BEACON:MSG:" + myRadio.RADIO.mybeacon);
		previous_operation = "SET:RADIO:BEACON:MSG:"  + myRadio.RADIO.mybeacon;
	
	} else if (msgPARSED[0] === ".myCALL") {                  /*  Set myCALL and mySSID  */
		msgSUBPARSED = msgPARSED[1];
		msgPARTICLE = msgSUBPARSED.split("-");
		myRadio.CONTACT.mycall = msgPARTICLE[0];
		myRadio.CONTACT.myssid = msgPARTICLE[1];
		message = "CALL = " + myRadio.CONTACT.myssid + "-" + myRadio.CONTACT.myssid;
		rxwinMSG(message);
		console.log("SET: CALL: " + myRadio.CONTACT.myssid + "-" + myRadio.CONTACT.myssid);
		previous_operation = "myCALL";
	
	} else if (msgPARSED[0] === ".dstCALL") {                 /*  Set myRadio.CONTACT.dstcall and myRadio.CONTACT.dstssid  */
		msgSUBPARSED = msgPARSED[1];
		msgPARTICLE = msgSUBPARSED.split("-");
		myRadio.CONTACT.dstcall = msgPARTICLE[0];
		myRadio.CONTACT.dstssid = msgPARTICLE[1];
		message = "DEST = " + myRadio.CONTACT.dstcall + "-" + myRadio.CONTACT.dstssid;
		rxwinMSG(message);
		console.log("SET: DEST: " + myRadio.CONTACT.dstcall + "-" + myRadio.CONTACT.dstssid);
		previous_operation = "dstCALL";
	
	} else if (msgPARSED[0] === ".macro1") {                 /*  Set Macro Button 1  */
		msgSUBPARSED = msgENTERED.replace('.macro1 ', '')
		myMacros[0] = msgSUBPARSED;
		message = "MACRO1 = " + myMacros[0];
		rxwinMSG(message);
		console.log("SET: MACRO:1: " + myMacros[0]);
		previous_operation = "M1";
	
	} else if (msgPARSED[0] === ".macro2") {                 /*  Set Macro Button 2  */
		msgSUBPARSED = msgENTERED.replace('.macro2 ', '')
		myMacros[1] = msgSUBPARSED;
		message = "MACRO3 = " + myMacros[1];
		rxwinMSG(message);
		console.log("SET: MACRO:2: " + myMacros[1]);
		previous_operation = "M2";
	
	} else if (msgPARSED[0] === ".macro3") {                 /*  Set Macro Button 3  */
		msgSUBPARSED = msgENTERED.replace('.macro3 ', '')
		myMacros[2] = msgSUBPARSED;
		message = "MACRO1 = " + myMacros[2];
		rxwinMSG(message);
		console.log("SET: MACRO: 3: " + myMacros[2]);
		previous_operation = "M3";
		
	} else if (msgPARSED[0] === ".macro4") {                 /*  Set Macro Button 4  */
		msgSUBPARSED = msgENTERED.replace('.macro4 ', '')
		myMacros[3] = msgSUBPARSED;
		message = "MACRO1 = " + myMacros[3];
		rxwinMSG(message);
		console.log("SET: MACRO: 4: " + myMacros[3]);
		previous_operation = "M4";

	/* Anything else is a message to be TX */
	} else {
		socket.send("TX:" + msgENTERED);
		rxwinMSG("TX: "+ msgENTERED);
		console.log("TX: " + msgENTERED);
		previous_operation = "SENT";
	}

	msgENTERED = "";
	msgPARSED = "";
	previous_operation = "NULL";
	document.getElementById('msgENTRY').value = "";

}

/* --- Receiver-Window --- */

function rxwinRCLR() {
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
	rxDisplay [25] = ".................... Basic Instructions .............."
	rxDisplay [24] = "-"
	rxDisplay [23] = "............ KEYPAD .......... ........ CONTROL TOGGLES ........"
	rxDisplay [22] = "-"
	rxDisplay [21] = "----  0 thru 9 Selects --- ----- RADIO = RX/REPEAT/BEACON ---"
  	rxDisplay [20] = "--- different standard -- -------- PA = LOW/MED/HIGH ---"
  	rxDisplay [19] = "--- LoRa Modem configs -- ------ MODE = LORA/XARPS/FSK ---"
  	rxDisplay [18] = "- ----------------------- ------- BAS = BASIC/EXPERT ---"
	rxDisplay [17] = "-"
	rxDisplay [16] = "..................... MESSAGE CONTROLS ....................."
	rxDisplay [15] = "-"
  	rxDisplay [14] = "- CALL = myCall Macro ------- ---- LOG = Log on/off (toggle)"
  	rxDisplay [13] = "- DEST = dstCall Macro ------ --- RCLR = Clear RX Window"
  	rxDisplay [12] = "- HEAD = Message Header------ --- MCLR = TX window clear"
  	rxDisplay [11] = "- HELP = This screen -------- --- SEND = Sends TX Window"
  	rxDisplay [10] = "- "
	rxDisplay [9] = ".................... TX WINDOW COMMANDS ...................."
	rxDisplay [8] = "-"	  
  	rxDisplay [7] = "- .myCALL NOCALL-00     Changes CALL macro to hold NOCALL-00"
  	rxDisplay [6] = "- .dstCALL BL0B-50      Changes DEST macro to hold BL0B-50"
  	rxDisplay [5] = "- .macro1 blah blah     Changes M1 button to hold blah blah"
  	rxDisplay [4] = "-"
	rxDisplay [3] = "..... TO SEND A MESSAGE"
	rxDisplay [2] = "-"  
  	rxDisplay [1] = "- Via Macro buttons click HEAD, M1, then SEND"
  	rxDisplay [0] = "- Type YourCall>DestCall|MESSAGE in TX window then click SEND"
}

function rxwinMSGexperthelp() {
	rxDisplay [26] = "-"
	rxDisplay [25] = "....................  Expert Instructions ...................."
	rxDisplay [24] = "-"
	rxDisplay [23] = "............ KEYPAD .......... ........ CONTROL TOGGLES ........"
	rxDisplay [22] = "-"
	rxDisplay [21] = "- 0 thru 9 Tune Frequency ---- ----- RADIO = RX/REPEAT/BEACON ---"
  	rxDisplay [20] = "----- RST = Reset Keypad ----- -------- PA = LOW/MED/HIGH ---"
  	rxDisplay [19] = "----- CLR = Clear Display ---- ------ MODE = LORA/XARPS/FSK ---"
  	rxDisplay [18] = "----- ENT = Update Freq ------ ------- BAS = BASIC/EXPERT ---"
	rxDisplay [17] = "-"
	rxDisplay [16] = "..................... MESSAGE CONTROLS ....................."
	rxDisplay [15] = "-"
  	rxDisplay [14] = "- CALL = myCall Macro ------- ---- LOG = Log on/off (toggle)"
  	rxDisplay [13] = "- DEST = dstCall Macro ------ --- RCLR = Clear RX Window"
  	rxDisplay [12] = "- HEAD = Message Header------ --- MCLR = TX window clear"
  	rxDisplay [11] = "- HELP = This screen -------- --- SEND = Sends TX Window"
  	rxDisplay [10] = "- "
	rxDisplay [9] = ".................... TX WINDOW COMMANDS ...................."
	rxDisplay [8] = "-"	  
  	rxDisplay [7] = "- .myCALL NOCALL-00     Changes CALL macro to hold NOCALL-00"
  	rxDisplay [6] = "- .dstCALL BL0B-50      Changes DEST macro to hold BL0B-50"
  	rxDisplay [5] = "- .macro1 blah blah     Changes M1 button to hold blah blah"
  	rxDisplay [4] = "-"
	rxDisplay [3] = "..... TO SEND A MESSAGE"
	rxDisplay [2] = "-"  
  	rxDisplay [1] = "- Via Macro buttons click HEAD, M1, then SEND"
  	rxDisplay [0] = "- Type YourCall>DestCall|MESSAGE in TX window then click SEND"
}

/* --- Transmitter-Module --- */

function msgENTRY() {
	// Get the focus to the text input to enter a word right away.
	document.getElementById('msgENTRY').focus();

	
	// Getting the text from the input
	var msgENTERED = document.getElementById('msgENTRY').value;
  }


/* --- ROOT-Module --- */


/* -            
/* - MAIN
/* -
*/

// Get JSON file on page load
getHasJson.open('GET', urlJSON, true);
getHasJson.send(null);
getHasJson.onload = function() {
	if (getHasJson.readyState === getHasJson.DONE && getHasJson.status === 200) {
		myRadio = JSON.parse(getHasJson.responseText)
	}
}
console.log("INIT: JSON loading ...");
console.log(" ");

// Modem assignments
loraModems = Object.keys(myRadio.MODEMS);
console.log("INIT:  RADIO SETTINGS");
console.log("RADIO:        Modem: ", myRadio.modem);
console.log("RADIO:   Modem Name: ", myRadio.MODEMS[0].modemname);
console.log("RADIO:  ModemConfig: ", myRadio.MODEMS[0].modemconfig);
console.log("RADIO: SpreadFactor: ", myRadio.MODEMS[0].spreadfactor);
console.log("RADIO:  CodingRate4: ", myRadio.MODEMS[0].codingrate4);
console.log("RADIO:    Bandwdith: ", myRadio.MODEMS[0].bandwidth);
console.log("RADIO:      Channel: ", myRadio.CHANNELS[0].channelname);
console.log(" ");


//LoRa settings
myRadio.LORA.codingrate4 = myRadio.MODEMS[0].codingrate4;
myRadio.LORA.bandwidth = myRadio.MODEMS[0].bandwidth;
myRadio.LORA.spreadfactor = myRadio.MODEMS[0].spreadfactor;

// Channel assignments
loraChannels = Object.keys(myRadio.CHANNELS);
console.log("INIT:  HVDN CHANNEL SETTINGS");
console.log("CHAN:      CH 0,1,2: ", myRadio.CHANNELS[0].channelname, myRadio.CHANNELS[1].channelname, myRadio.CHANNELS[2].channelname);
console.log("CHAN:      CH 3,4,5: ", myRadio.CHANNELS[3].channelname, myRadio.CHANNELS[4].channelname, myRadio.CHANNELS[5].channelname);
console.log("CHAN:      CH 6,7,8: ", myRadio.CHANNELS[6].channelname,  myRadio.CHANNELS[7].channelname, myRadio.CHANNELS[8].channelname);
console.log("CHAN:          CH 9: ", myRadio.CHANNELS[9].channelname);
console.log(" ");

// Set Macros M1 to M4
console.log("INIT: MACRO SETTINGS");
myMacros[0] = myRadio.CMACROS.C1;
myMacros[1] = myRadio.CMACROS.C2;
myMacros[2] = myRadio.CMACROS.C3;  
myMacros[3] = myRadio.CMACROS.C4;
console.log("MACRO:     Macro C1: ", myCMacros[0]);
console.log("MACRO:     Macro C2: ", myCMacros[1]);
console.log("MACRO:     Macro C3: ", myCMacros[2]);
console.log("MACRO:     Macro C4: ", myCMacros[3]);
myMacros[0] = myRadio.MACROS.M1;
myMacros[1] = myRadio.MACROS.M2;
myMacros[2] = myRadio.MACROS.M3;  
myMacros[3] = myRadio.MACROS.M4;
console.log("MACRO:     Macro M1: ", myMacros[0]);
console.log("MACRO:     Macro M2: ", myMacros[1]);
console.log("MACRO:     Macro M3: ", myMacros[2]);
console.log("MACRO:     Macro M4: ", myMacros[3]);
console.log(" ");

console.log("INIT: WEBSOCKET CONNECT");
console.log("SOCK:      Hostname: " + myHostname);
console.log("SOCK:     Websocket: " + wsURI);
console.log(" ");

// Load RX Window with Help Message
if (radioGUI == 0) {
	rxwinMSGhelp();
}
if (radioGUI == 1) {
	rxwinMSGexperthelp();
}
rxwinMSG('-');

console.log("INIT: COMPLETE");
console.log(" ");
previous_entry = "";  
previous_operation = "INIT";                    // Set last operation

/* Establish Websocket */
socket = new WebSocket(wsURI);

updateDisplay();

socket.onopen = function(e) {
  console.log("WSS: [open] Connection established");
  console.log("WSS: Sending to server");
  socket.send("CONNECT:" + myHostname);
};

socket.onmessage = function(event) {
	var wsRXD = event.data.split(':');
	if (wsRXD[0] == "RX") {
		rxwinMSG("RX: " + wsRXD[1]);
		console.log("WSS: RX: " + wsRXD[1]);
	} else if (wsRXD[0] == "RESPONSE") {
		rxwinMSG("RX: " + wsRXD[1]);
		console.log("WSS: RX: " + wsRXD[1]);
	};

	console.log(`WSS: [message] Data received from server: ${event.data}`);
};

socket.onclose = function(event) {
  if (event.wasClean) {
    console.log(`WSS: [close] Connection closed cleanly, code=${event.code} reason=${event.reason}`);
  } else {
    // e.g. server process killed or network down
    // event.code is usually 1006 in this case
    console.log('WSS: [close] Connection died');
  }
};

socket.onerror = function(error) {
	var rxWINerror = "WSS:ERROR connecting to " + wsURI;
	rxwinMSG(rxWINerror);
	console.log(`WSS: [error] ${error.message}`);
};
