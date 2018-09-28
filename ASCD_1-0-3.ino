// Arduino Smart Charger / Discharger 
// Version 1.0.3
// ---------------------------------------------------------------------------
// Created by Brett Watt on 20/07/2018
// Copyright 2018 - Under creative commons license 3.0:
//
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
// 
// @brief 
// This is the Arduino 8x 18650 Smart Charger / Discharger Code
//
// Current implementation: 
// TP4056, Rotary Encoder KY-040 Module, Temp Sensor DS18B20
// Ethernet Module W5500, Mini USB Host Shield (Barcode Scanner), 
// LCD 2004 20x4 with IIC/I2C/TWI Serial, Discharge (MilliAmps and MillOhms)
//
// @author Email: info@vortexit.co.nz 
//       Web: www.vortexit.co.nz


static const uint8_t batteryVolatgePins[] =     {A0,A2,A4,A6,A8,A10,A12,A14};
static const uint8_t batteryVolatgeDropPins[] = {A1,A3,A5,A7,A9,A11,A13,A15};
static const uint8_t chargeMosfetPins[] =       {22,25,28,31,34,37,40,43};
static const uint8_t chargeLedPins[] =          {23,26,29,32,35,38,41,44};
static const uint8_t dischargeMosfetPins[] =    {24,27,30,33,36,39,42,45};

#define TEMPERATURE_PRECISION 9

//Includes 
#include <SPI.h>
#include <Wire.h> // Comes with Arduino IDE
#include <LiquidCrystal_I2C.h> 
#include "Timer.h"
#include "Encoder_Polling.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <hidboot.h> //Barcode Scanner
#include <usbhub.h> //Barcode Scanner

//#include <MemoryFree.h>

// Comment out the version that are not your PCB version
#include "ascd_pcb_ver_1-1.h" 	// Version 1.1 PCB PIN definitions
//#include "ascd_pcb_ver_2-0.h"	// Version 2.0 PCB PIN definitions 

// Comment out the Network type that you are not going to use
#include "ethernet_mode.h" 	// Use the W5500 Ethernet Module
//#include "wifi_mode.h"	// Use the ESP8266 Wifi Module

//Objects
LiquidCrystal_I2C lcd(0x27,20,4); // set the LCD address to 0x27 for a 20 chars and 4 line display
Timer timerObject;
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

extern SPISettings wiznet_SPI_settings;

//--------------------------------------------------------------------------
// Custom Header File for PCB and USER specific settings
#include "custom_settings.h"	// User and Board Custom Settings

//--------------------------------------------------------------------------
// Constant Variables
const byte modules = 8; // Number of Modules
const char server[] = "submit.vortexit.co.nz";    // Server to connect to send and recieve data

//--------------------------------------------------------------------------
// Initialization
unsigned long longMilliSecondsCleared[modules];
int intSeconds[modules];
int intMinutes[modules];
int intHours[modules];
//--------------------------------------------------------------------------
// Cycle Variables
byte cycleState[modules];
byte cycleStateLast = 0;
byte rotaryOverride = 0;
boolean rotaryOverrideLock = 0;
boolean barcodeOverride = false;
boolean buttonState = 0;
boolean lastButtonState = 0;
boolean mosfetSwitchState[modules];
byte cycleStateCycles = 0;
//int connectionAttempts[8];
float batteryVoltage[modules];
byte batteryFaultCode[modules];
byte batteryInitialTemp[modules];
byte batteryHighestTemp[modules];
byte batteryCurrentTemp[modules];
byte tempCount[modules];
byte tempCountAmbient = 0;
byte tempAmbient = 0;
char lcdLine2[25];
char lcdLine3[25];
const float batteryVolatgeLeak = 2.00;

// readPage Variables
char serverResult[32]; // string for incoming serial data
int stringPosition = 0; // string index counter readPage()
bool startRead = false; // is reading? readPage()

// Check Battery Voltage
byte batteryDetectedCount[modules];
//bool batteryDetected[modules];

// Get Barcode
char barcodeString[25] = "";
bool barcodeStringCompleted = false;
byte pendingBarcode = 255;
char batteryBarcode[modules][25];
//bool barcodeDuplicateFound[modules];

// Cutoff Voltage / Get Barcode
bool batteryGetCompleted[modules];
bool pendingBatteryRecord[modules];

// Charge / Recharge Battery
float batteryInitialVoltage[modules];
byte batteryChargedCount[modules];

// Check Battery Milli Ohms
byte batteryMilliOhmsCount[modules];
float tempMilliOhmsValue[modules];
float milliOhmsValue[modules];

// Discharge Battery
int intMilliSecondsCount[modules];
unsigned long longMilliSecondsPreviousCount[modules];
unsigned long longMilliSecondsPrevious[modules];
unsigned long longMilliSecondsPassed[modules];
float dischargeMilliamps[modules];
float dischargeVoltage[modules];
float dischargeAmps[modules];
float batteryCutOffVoltage[modules];
bool dischargeCompleted[modules];
bool dischargeUploadCompleted[modules];

int dischargeMinutes[modules];
bool pendingDischargeRecord[modules];

// Completed
//byte mosfetSwitchCount[modules];

// USB Host Shield - Barcode Scanner
class KbdRptParser : public KeyboardReportParser
{

protected:
	virtual void OnKeyDown  (uint8_t mod, uint8_t key);
	virtual void OnKeyPressed(uint8_t key);
};

KbdRptParser Prs;
USB     Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);


//--------------------------------------------------------------------------

void setup() 
{
	SPISettings settingsA(8000000, MSBFIRST, SPI_MODE3);
	wiznet_SPI_settings = settingsA;
	pinMode(5, INPUT_PULLUP); // Pin 5 Rotary Encoder Button (SW)
	for(byte i = 0; i < modules; i++)
	{ 
		pinMode(chargeLedPins[i], INPUT_PULLUP);
		pinMode(chargeMosfetPins[i], OUTPUT);
		pinMode(dischargeMosfetPins[i], OUTPUT);
		digitalWrite(chargeMosfetPins[i], LOW);
		digitalWrite(dischargeMosfetPins[i], LOW);
	}
	char lcdStartup0[25];
	char lcdStartup1[25];
	char lcdStartup2[25];
	char lcdStartup3[25];
	//Serial.begin(115200);
	//Serial.println("Started");
	//Startup Screen
	lcd.init();
	lcd.clear();
	lcd.backlight();// Turn on backlight
	sprintf(lcdStartup0, "%-20s", "SMART CHARGER 1.0.3");
	lcd.setCursor(0,0);
	lcd.print(lcdStartup0);
	sprintf(lcdStartup1, "%-20s", "INITIALIZING TP4056");
	lcd.setCursor(0,1);
	lcd.print(lcdStartup1);
	mosfetSwitchAll();
	sprintf(lcdStartup1, "%-20s", "STARTING USB & NET");
	lcd.setCursor(0,1);
	lcd.print(lcdStartup1);
	delay(1000);
	sprintf(lcdStartup3, "%-20s", Usb.Init() == -1 ? "USB: DID NOT START" : "USB: STARTED");
	HidKeyboard.SetReportParser(0, &Prs);
	lcd.setCursor(0,3);
	lcd.print(lcdStartup3);
	//char networkIP[16];
	// Comment out the Network type that you are not going to use
	
	// Ethernet Mode
	if (Ethernet.begin(mac) == 0) {
		// Try to congifure using a static IP address instead of DHCP:
		Ethernet.begin(mac, ip, dnServer, gateway, subnet); 
	}
	delay(1000);
	IPAddress networkIP = Ethernet.localIP();

	/*
	// Wifi Mode
	// Initialize ESP module
	Serial1.begin(WIFI_BAUD);
	WiFi.init(&Serial1);
	// Check for the presence of the shield
	if (WiFi.status() == WL_NO_SHIELD) {
		//Serial.println("WiFi shield not present");
		//Don't continue
		while (true);
	}
	// Attempt to connect to WiFi network
	while ( status != WL_CONNECTED) {
		// Connect to WPA/WPA2 network
		status = WiFi.begin(ssid, pass);
	}
	IPAddress networkIP = WiFi.localIP();
	*/
	
	sprintf(lcdStartup2, "IP:%d.%d.%d.%d%-5s", networkIP[0], networkIP[1], networkIP[2], networkIP[3], " ");
	lcd.setCursor(0,2);
	lcd.print(lcdStartup2);
	float checkConnectionResult = checkConnection();
	if(checkConnectionResult == 0)
	{
		sprintf(lcdStartup3, "%-20s", "DB/WEB: CONNECTED");
	} else if (checkConnectionResult == 2) {
		sprintf(lcdStartup3, "%-20s", "DB/WEB: TIME OUT");
	} else {
		sprintf(lcdStartup3, "%-20s", "DB/WEB: FAILED");
	}
	lcd.setCursor(0,3);
	lcd.print(lcdStartup3);
	sensors.begin(); // Start up the library Dallas Temperature IC Control
	encoder_begin(CLK, DT); // Start the decoder encoder Pin A (CLK) = 3 encoder Pin B (DT) = 4 | Pin 7 (CLK) and Pin 6 (DT) for Version 2.0
	delay(2000);
	// Timer Triggers
	timerObject.every(2, rotaryEncoder);
	timerObject.every(1000, cycleStateValues);
	timerObject.every(1000, cycleStateLCD);
	timerObject.every(60000, cycleStatePostWebData);
	//timerObject.every(5000, cycleStateGetWebData);
	lcd.clear();
}

void loop()
{
	//Nothing else should be in this loop. Use timerObject and Usb.Task only .
	Usb.Task(); //Barcode Scanner
	timerObject.update();
}

void processBarcode(int keyInput)
{
	//Barcode Scanner
	if (keyInput == 19) //Return Carriage ASCII Numeric 19
	{
		barcodeStringCompleted = true;
		cycleStateValues();
		cycleStateLCD(); 
	} else {
		sprintf(barcodeString, "%s%c", barcodeString, (char)keyInput);
		barcodeStringCompleted = false;
	}
}

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)  
{
	//Barcode Scanner
	uint8_t c = OemToAscii(mod, key);
	if (c) OnKeyPressed(c);
}

void KbdRptParser::OnKeyPressed(uint8_t key)  
{
	//Barcode Scanner
	processBarcode(key);
};

void rotaryEncoder()
{
	int rotaryEncoderDirection = encoder_data(); // Check for rotation
	if(rotaryEncoderDirection != 0)              // Rotary Encoder
	{
		if (cycleStateLast - rotaryEncoderDirection >= 0 && cycleStateLast - rotaryEncoderDirection < modules)
		{
			cycleStateLast = cycleStateLast - rotaryEncoderDirection;
		} else if (cycleStateLast - rotaryEncoderDirection == modules) {
			cycleStateLast = 0;
		} else if (cycleStateLast - rotaryEncoderDirection < 0) {
			cycleStateLast = modules - 1;
		}
		rotaryOverride = 60; // x (1 min) cycles of cycleStateLCD() decrements - 1 each run until 0
		lcd.clear();
		cycleStateLCD();
	} else { // Button
		buttonState = digitalRead(5);
		if (buttonState != lastButtonState) 
		{
			if (buttonState == LOW) 
			{
				if (rotaryOverride == 255)
				{
					barcodeOverride = true;
					rotaryOverrideLock = 0;
				} else {
					if (rotaryOverrideLock == 1)
					{
						rotaryOverrideLock = 0;
						rotaryOverride = 0;
					} else {
						if (rotaryOverride > 0) rotaryOverrideLock = 1;
					}
				}
			}
		}
		lastButtonState = buttonState;
	}
}

void cycleStatePostWebData()
{
	bool postData;
	for(byte i = 0; i < modules; i++) // Check if any data needs posting
	{ 
		if(pendingDischargeRecord[i] == true)
		{
			postData = true;
			break;
		}
	}
	if (postData == true) 
	{
		byte connectionResult = checkConnection();
		if (connectionResult == 0) // Check Connection
		{
			for(byte i = 0; i < modules; i++)
			{ 
				if(pendingDischargeRecord[i] == true)
				{
					if (addDischargeRecord(i) == 0) pendingDischargeRecord[i] = false;
				}
			}   
		} else if (connectionResult == 1){
			// Error Connection
			
		} else if (connectionResult == 2){
			// Time out Connection
			
		}
	} 
}

/*
void cycleStateGetWebData()
{
bool postData;
for(byte i = 0; i < modules; i++) // Check if any data needs posting
{ 
	if(pendingBatteryRecord[i] == true)
	{
	postData = true;
	break;
	}
}
if (postData == true) 
{
	byte connectionResult = checkConnection();
	if (connectionResult == 0) // Check Connection
	{
	for(byte i = 0; i < modules; i++)
	{ 
		if(pendingBatteryRecord[i] == true)
		{
		float cutOffVoltageResult = getCutOffVoltage(i);
		if (cutOffVoltageResult > 100)
		{
			batteryCutOffVoltage[i] = cutOffVoltageResult - 100;
			pendingBatteryRecord[i] = false;
			break;
		} else {
			switch ((int) cutOffVoltageResult) 
			{
			case 0: 
			//SUCCESSFUL = 0
			batteryCutOffVoltage[i] = defaultBatteryCutOffVoltage; // No DB in Table tblbatterytype for BatteryCutOffVoltage (NULL record)
			pendingBatteryRecord[i] = false;
			break;
			case 1:
			//FAILED = 1
			//Need to retry
			pendingBatteryRecord[i] = true;
			break;
			case 2:
			//TIMEOUT = 2 
			//Need to retry
			pendingBatteryRecord[i] = true;
			break;
			case 5: 
			//ERROR_NO_BARCODE_DB = 5
			batteryCutOffVoltage[i] = defaultBatteryCutOffVoltage;
			pendingBatteryRecord[i] = false;
			// What happens next as not in DB???
			break;    
			} 
		}
		}
	}   
	} else if (connectionResult == 1){
	// Error Connection
	
	} else if (connectionResult == 2){
	// Time out Connection
	
	}
}
}
*/

void cycleStateLCD()
{
	if (rotaryOverride > 0)
	{
		cycleStateLCDOutput(cycleStateLast ,0 ,1);
		if (rotaryOverrideLock == 0 && rotaryOverride != 255) rotaryOverride--;
	} else {
		cycleStateLCDOutput(cycleStateLast ,0 ,1);
		if (cycleStateLast == modules - 1)
		{
			cycleStateLCDOutput(0 ,2 ,3);
		} else {
			cycleStateLCDOutput(cycleStateLast + 1 ,2 ,3);
		}
		if(cycleStateCycles == 4) // 4 = 4x 1000ms (4 seconds) on 1 screen -> Maybe have const byte for 4 -> Menu Option?
		{
			if (cycleStateLast >= modules - 2)
			{
				cycleStateLast = 0;
			} else {
				cycleStateLast += 2;
			}
			cycleStateCycles = 0;
		} else {
			cycleStateCycles++;
		}
	}
}

void cycleStateLCDOutput(byte j, byte LCDRow0 ,byte LCDRow1)
{
	char lcdLine0[25];
	char lcdLine1[25];
	if (rotaryOverride > 0)
	{
		sprintf(lcdLine2, "%-20s", " ");
		if (rotaryOverride == 255)
		{
			sprintf(lcdLine3, "%-20s", "PRESS BUTTON TO SKIP");
		} else {
			sprintf(lcdLine3, "%-18s%02d", " ", rotaryOverride);
		}
		lcd.setCursor(0,2);
		lcd.print(lcdLine2);
		lcd.setCursor(0,3);
		lcd.print(lcdLine3); 
	}
	if (rotaryOverride > 0 && rotaryOverrideLock == 1 && rotaryOverride != 255)
	{
		sprintf(lcdLine3, "%-20s", "   SCREEN LOCKED");
		lcd.setCursor(0,3);
		lcd.print(lcdLine3); 
	}

	switch (cycleState[j]) 
	{
	case 0: // Check Battery Voltage
		sprintf(lcdLine0, "%02d%-18s", j + 1, "-BATTERY CHECK");
		sprintf(lcdLine1, "%-15s%d.%02dV", batteryDetectedCount[j] > 0 ? "DETECTED....." : "NOT DETECTED!", (int)batteryVoltage[j], (int)(batteryVoltage[j]*100)%100);
		break;
	case 1: // Get Battery Barcode
		sprintf(lcdLine0, "%02d%-18s", j + 1, "-BARCODE");
		sprintf(lcdLine1, "%-15s%d.%02dV", "SCAN BARCODE", (int)batteryVoltage[j], (int)(batteryVoltage[j]*100)%100);
		break;
	case 2: // Charge Battery
		sprintf(lcdLine0, "%02d%-10s%02d:%02d:%02d", j + 1, "-CHARGING ", intHours[j], intMinutes[j], intSeconds[j]);
		sprintf(lcdLine1, "%d.%02dV %02d%c D%02d%c %d.%02dV", (int)batteryInitialVoltage[j], (int)(batteryInitialVoltage[j]*100)%100, batteryInitialTemp[j], 223, (batteryCurrentTemp[j] - batteryInitialTemp[j]) > 0 ? (batteryCurrentTemp[j] - batteryInitialTemp[j]) : 0, 223,(int)batteryVoltage[j], (int)(batteryVoltage[j]*100)%100); 
		break;
	case 3: // Check Battery Milli Ohms
		sprintf(lcdLine0, "%02d%-18s", j + 1, "-RESISTANCE CHECK");
		sprintf(lcdLine1, "%-14s%04dm%c", "MILLIOHMS", (int) milliOhmsValue[j], 244);
		break;
	case 4: // Rest Battery
		sprintf(lcdLine0, "%02d%-10s%02d:%02d:%02d", j + 1, "-RESTING", intHours[j], intMinutes[j], intSeconds[j]);
		if (strlen(batteryBarcode[j]) >= 15)
		{
			sprintf(lcdLine1, "%-20s", batteryBarcode[j]);
		} else if (strlen(batteryBarcode[j]) >= 10) {
			sprintf(lcdLine1, "%-15s%d.%02dV", batteryBarcode[j], (int)batteryVoltage[j], (int)(batteryVoltage[j]*100)%100);
		} else {
			sprintf(lcdLine1, "%-10s %02d%c %d.%02dV", batteryBarcode[j], batteryCurrentTemp[j], 223, (int)batteryVoltage[j], (int)(batteryVoltage[j]*100)%100);
		}
		break;
	case 5: // Discharge Battery       
		sprintf(lcdLine0, "%02d%-7s%d.%02dA %d.%02dV", j + 1, "-DCHG", (int)dischargeAmps[j], (int)(dischargeAmps[j]*100)%100, (int)dischargeVoltage[j], (int)(dischargeVoltage[j]*100)%100);
		sprintf(lcdLine1, "%02d:%02d:%02d %02d%c %04dmAh", intHours[j], intMinutes[j], intSeconds[j], batteryCurrentTemp[j], 223, (int) dischargeMilliamps[j]);
		break;
	case 6: // Recharge Battery
		sprintf(lcdLine0, "%02d%-10s%02d:%02d:%02d", j + 1, "-RECHARGE ", intHours[j], intMinutes[j], intSeconds[j]);
		sprintf(lcdLine1, "%d.%02dV %02d%c D%02d%c %d.%02dV", (int)batteryInitialVoltage[j], (int)(batteryInitialVoltage[j]*100)%100, batteryInitialTemp[j], 223, (batteryCurrentTemp[j] - batteryInitialTemp[j]) > 0 ? (batteryCurrentTemp[j] - batteryInitialTemp[j]) : 0, 223, (int)batteryVoltage[j], (int)(batteryVoltage[j]*100)%100);     
		break;
	case 7: // Completed
		switch (batteryFaultCode[j]) 
		{
		case 0: // Finished
			sprintf(lcdLine0, "%02d%-14sH%02d%c", j + 1, "-FINISHED", batteryHighestTemp[j], 223);
			break;
		case 3: // High Milli Ohms
			sprintf(lcdLine0, "%02d%-11s %c H%02d%c", j + 1, "-FAULT HIGH", 244, batteryHighestTemp[j], 223);
			break;    
		case 5: // Low Milliamps
			sprintf(lcdLine0, "%02d%-14sH%02d%c", j + 1, "-FAULT LOW Ah", batteryHighestTemp[j], 223);
			break; 
		case 7: // High Temperature
			sprintf(lcdLine0, "%02d%-15s%02d%c", j + 1, "-FAULT HIGH TMP", batteryHighestTemp[j], 223);
			break; 
		case 9: // Charge Timeout
			sprintf(lcdLine0, "%02d%-14sH%02d%c", j + 1, "-FAULT C TIME", batteryHighestTemp[j], 223);
			break; 
		} 
		sprintf(lcdLine1, "%04dm%c %d.%02dV %04dmAh",(int) milliOhmsValue[j], 244,(int)batteryVoltage[j], (int)(batteryVoltage[j]*100)%100, (int) dischargeMilliamps[j]);
		break;
	}
	lcd.setCursor(0,LCDRow0);
	lcd.print(lcdLine0);
	lcd.setCursor(0,LCDRow1);
	lcd.print(lcdLine1);  
}

void cycleStateValues()
{
	byte tempProcessed = 0;
	for(byte i = 0; i < modules; i++)
	{ 
		switch (cycleState[i]) 
		{
		case 0: // Check Battery Voltage
			digitalWrite(chargeMosfetPins[i], LOW);
			digitalWrite(dischargeMosfetPins[i], LOW);
			if(batteryCheck(i)) batteryDetectedCount[i]++;
			if (batteryDetectedCount[i] == 5)
			{
				batteryCurrentTemp[i] = getTemperature(i);
				batteryInitialTemp[i] = batteryCurrentTemp[i];
				batteryHighestTemp[i] = batteryCurrentTemp[i];
				// Re-Initialization
				batteryDetectedCount[i] = 0;
				strcpy(batteryBarcode[i], "");
				batteryChargedCount[i] = 0;
				batteryMilliOhmsCount[i] = 0;
				tempMilliOhmsValue[i] = 0;
				milliOhmsValue[i] = 0;
				intMilliSecondsCount[i] = 0;
				longMilliSecondsPreviousCount[i] = 0;
				longMilliSecondsPrevious[i] = 0;
				longMilliSecondsPassed[i] = 0;
				dischargeMilliamps[i] = 0.0;
				dischargeVoltage[i] = 0.00;
				dischargeAmps[i] = 0.00;
				dischargeCompleted[i] = false;
				batteryFaultCode[i] = 0;
				//mosfetSwitchCount[i] = 0;
				clearSecondsTimer(i);
				getBatteryVoltage(i); // Get battery voltage for Charge Cycle
				batteryInitialVoltage[i] = batteryVoltage[i];
				cycleState[i] = 1; // Check Battery Voltage Completed set cycleState to Get Battery Barcode   
			}
			break;
			
		case 1: // Get Battery Barcode  
			//getBatteryVoltage(i);
			if(strcmp(batteryBarcode[i], "") == 0)
			{
				if (pendingBarcode == 255) pendingBarcode = i;
				// HTTP Check if the Battery Barcode is in the DB
				if ((barcodeStringCompleted == true && pendingBarcode == i) || barcodeOverride == true) 
				{
					if(barcodeOverride == true)
					{
						strcpy(batteryBarcode[i], "NO_BARCODE");
						barcodeOverride = false;
					} else {
						strcpy(batteryBarcode[i], barcodeString);
					}
					strcpy(barcodeString, "");
					barcodeStringCompleted = false;
					rotaryOverride = 0;
					pendingBarcode = 255;
					cycleState[i] = 2; // Get Battery Barcode Completed set cycleState to Charge Battery
					clearSecondsTimer(i);
				} else if (pendingBarcode == i) {
					cycleStateLast = i; // Gets priority and locks cycleLCD Display
					rotaryOverride = 255; // LCD Cycle on this until barcode is scanned
				} 
			}
			//Check if battery has been removed
			digitalWrite(chargeMosfetPins[i], HIGH); // Turn on TP4056
			digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
			if(!batteryCheck(i)) batteryDetectedCount[i]++;
			if (batteryDetectedCount[i] == 5) 
			{
				batteryDetectedCount[i] = 0;
				rotaryOverride = 0;
				cycleState[i] = 0; // Completed and Battery Removed set cycleState to Check Battery Voltage
			}
			break;
			
		case 2: // Charge Battery
			getBatteryVoltage(i);
			batteryCurrentTemp[i] = getTemperature(i);
			tempProcessed = processTemperature(i, batteryCurrentTemp[i]);
			if (tempProcessed == 2)
			{
				//Battery Temperature is >= MAX Threshold considered faulty
				digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
				batteryFaultCode[i] = 7; // Set the Battery Fault Code to 7 High Temperature
				cycleState[i] = 7; // Temperature is to high. Battery is considered faulty set cycleState to Completed        
			} else {
				digitalWrite(chargeMosfetPins[i], HIGH); // Turn on TP4056
				batteryChargedCount[i] = batteryChargedCount[i] + chargeCycle(i);
				if (batteryChargedCount[i] == 5)
				{
					digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
					//clearSecondsTimer(i);
					batteryChargedCount[i] = 0;
					cycleState[i] = 3; // Charge Battery Completed set cycleState to Check Battery Milli Ohms
				} 
			}
			if (intHours[i] == chargingTimeout) // Charging has reached Timeout period. Either battery will not hold charge, has high capacity or the TP4056 is faulty
			{
				digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
				batteryFaultCode[i] = 9; // Set the Battery Fault Code to 7 Charging Timeout
				cycleState[i] = 7; // Charging Timeout. Battery is considered faulty set cycleState to Completed
			}
			break;
			
		case 3: // Check Battery Milli Ohms
			batteryMilliOhmsCount[i] = batteryMilliOhmsCount[i] + milliOhms(i);
			tempMilliOhmsValue[i] = tempMilliOhmsValue[i] + milliOhmsValue[i];
			if (batteryMilliOhmsCount[i] == 4)
			{
				milliOhmsValue[i] = tempMilliOhmsValue[i] / 4;
				if (milliOhmsValue[i] > highMilliOhms) // Check if Milli Ohms is greater than the set high Milli Ohms value
				{
					batteryFaultCode[i] = 3; // Set the Battery Fault Code to 3 High Milli Ohms
					cycleState[i] = 7; // Milli Ohms is high battery is considered faulty set cycleState to Completed
				} else {
					if (intMinutes[i] <= 1) // No need to rest the battery if it is already charged
					{ 
						cycleState[i] = 5; // Check Battery Milli Ohms Completed set cycleState to Discharge Battery
					} else {
						cycleState[i] = 4; // Check Battery Milli Ohms Completed set cycleState to Rest Battery   
					}
					clearSecondsTimer(i);  
				}
			}
			break;
			
		case 4: // Rest Battery
			getBatteryVoltage(i);
			batteryCurrentTemp[i] = getTemperature(i);
			if (intMinutes[i] == restTimeMinutes) // Rest time
			{
				clearSecondsTimer(i);
				intMilliSecondsCount[i] = 0;
				longMilliSecondsPreviousCount[i] = 0;
				cycleState[i] = 5; // Rest Battery Completed set cycleState to Discharge Battery    
			}
			break;
			
		case 5: // Discharge Battery
			batteryCurrentTemp[i] = getTemperature(i);
			tempProcessed = processTemperature(i, batteryCurrentTemp[i]);
			if (tempProcessed == 2)
			{
				//Battery Temperature is >= MAX Threshold considered faulty
				digitalWrite(dischargeMosfetPins[i], LOW); 
				batteryFaultCode[i] = 7; // Set the Battery Fault Code to 7 High Temperature
				cycleState[i] = 7; // Temperature is high. Battery is considered faulty set cycleState to Completed
			} else {
				if (dischargeCompleted[i] == true)
				{
					//Set Variables for Web Post
					dischargeMinutes[i] = intMinutes[i] + (intHours[i] * 60);
					pendingDischargeRecord[i] = true;
					if (dischargeMilliamps[i] < lowMilliamps) // No need to recharge the battery if it has low Milliamps
					{ 
						batteryFaultCode[i] = 5; // Set the Battery Fault Code to 5 Low Milliamps
						cycleState[i] = 7; // Discharge Battery Completed set cycleState to Completed
					} else {
						getBatteryVoltage(i); // Get battery voltage for Recharge Cycle
						batteryInitialVoltage[i] = batteryVoltage[i]; // Reset Initial voltage
						cycleState[i] = 6; // Discharge Battery Completed set cycleState to Recharge Battery
					}
					clearSecondsTimer(i);           
				} else {
					if(dischargeCycle(i)) dischargeCompleted[i] = true;
				}
			}
			break;
			
		case 6: // Recharge Battery
			digitalWrite(chargeMosfetPins[i], HIGH); // Turn on TP4056
			batteryCurrentTemp[i] = getTemperature(i);
			tempProcessed = processTemperature(i, batteryCurrentTemp[i]);
			if (tempProcessed == 2)
			{
				//Battery Temperature is >= MAX Threshold considered faulty
				digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
				batteryFaultCode[i] = 7; // Set the Battery Fault Code to 7 High Temperature
				cycleState[i] = 7; // Temperature is high. Battery is considered faulty set cycleState to Completed
			} else {
				batteryChargedCount[i] = batteryChargedCount[i] + chargeCycle(i);
				if (batteryChargedCount[i] == 5)
				{
					digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
					batteryChargedCount[i] = 0;
					clearSecondsTimer(i);
					cycleState[i] = 7; // Recharge Battery Completed set cycleState to Completed
				} 
			}
			if (intHours[i] == chargingTimeout) // Charging has reached Timeout period. Either battery will not hold charge, has high capacity or the TP4056 is faulty
			{
				digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
				digitalWrite(dischargeMosfetPins[i], LOW); // Turn off Discharge Mosfet (Just incase)
				batteryFaultCode[i] = 9; // Set the Battery Fault Code to 9 Charging Timeout
				cycleState[i] = 7; // Charging Timeout. Battery is considered faulty set cycleState to Completed
			}
			getBatteryVoltage(i);
			break;
			
		case 7: // Completed
			/*
			if (mosfetSwitchCount[i] <= 9)
			{
				mosfetSwitchCount[i]++;
			} 
			else if (mosfetSwitchCount[i] == 10)
			{
				mosfetSwitch(i);
				mosfetSwitchCount[i]++;
			} else if (mosfetSwitchCount[i] >= 11) 
			{
				mosfetSwitch(i);
				mosfetSwitchCount[i] = 0;
			}
			*/
			digitalWrite(chargeMosfetPins[i], HIGH); // Turn on TP4056
			digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
			if (!batteryCheck(i)) batteryDetectedCount[i]++;
			if (batteryDetectedCount[i] == 2) 
			{
				batteryDetectedCount[i] = 0;
				cycleState[i] = 0; // Completed and Battery Removed set cycleState to Check Battery Voltage
				digitalWrite(chargeMosfetPins[i], LOW); 
				digitalWrite(dischargeMosfetPins[i], LOW);
			}
			break;
		}
		secondsTimer(i);
	} 
}

/*
void mosfetSwitch(byte j)
{
	if (mosfetSwitchState[j])
	{
		digitalWrite(chargeMosfetPins[j], LOW);
		digitalWrite(dischargeMosfetPins[j], LOW);
		mosfetSwitchState[j] = false;
	} else {
		digitalWrite(chargeMosfetPins[j], HIGH);
		digitalWrite(dischargeMosfetPins[j], HIGH);
		mosfetSwitchState[j] = true;
	}
}
*/

void mosfetSwitchAll()
{
	for(byte i = 0; i < modules; i++)
	{ 
		for(byte j = 0; j < 3; j++)
		{ 
			digitalWrite(chargeMosfetPins[i], HIGH);
			digitalWrite(dischargeMosfetPins[i], HIGH);
			delay(100);
			digitalWrite(chargeMosfetPins[i], LOW);
			digitalWrite(dischargeMosfetPins[i], LOW);
			delay(100);
		}
	} 
}

/*
int getAmbientTemperature()
{
	if(tempCountAmbient >= 32 || tempAmbient == 0) // Read every 32x cycles
	{
		tempCountAmbient = 0;
		sensors.requestTemperaturesByAddress(tempSensorSerial[8]);  // Array [8] Ambient Sensor
		float tempC = sensors.getTempC(tempSensorSerial[8]);
		if (tempC > 99 || tempC < 0) tempC = 99;
		return (int) tempC;
	} else {
		tempCountAmbient++;
		return tempAmbient;
	}
}
*/

int getTemperature(byte j)
{
	if(tempCount[j] >= 16 || batteryCurrentTemp[j] == 0) // Read every 16x cycles
	{
		tempCount[j] = 0;
		sensors.requestTemperaturesByAddress(tempSensorSerial[j]);  
		float tempC = sensors.getTempC(tempSensorSerial[j]);
		if (tempC > 99 || tempC < 0) tempC = 99;
		return (int) tempC;
	} else {
		tempCount[j]++;
		return batteryCurrentTemp[j];
	}
}

byte processTemperature(byte j, byte currentTemperature)
{
	//byte currentAmbientTemperature = getAmbientTemperature();
	if (currentTemperature > batteryHighestTemp[j]) batteryHighestTemp[j] = currentTemperature; // Set highest temperature if current value is higher
	if ((currentTemperature - batteryInitialTemp[j]) > tempThreshold)
	{
		
		if ((currentTemperature - batteryInitialTemp[j]) > tempMaxThreshold)
		{
			//Temp higher than Maximum Threshold  
			return 2;
		} else {
			//Temp higher than Threshold <- Does nothing yet need some flag / warning
			return 1;  
		}
	} else {
		//Temp lower than Threshold  
		return 0;
	}
}

void secondsTimer(byte j)
{
	unsigned long longMilliSecondsCount = millis() - longMilliSecondsCleared[j];
	intHours[j] = longMilliSecondsCount / (1000L * 60L * 60L);
	intMinutes[j] = (longMilliSecondsCount % (1000L * 60L * 60L)) / (1000L * 60L);
	intSeconds[j] = (longMilliSecondsCount % (1000L * 60L * 60L) % (1000L * 60L)) / 1000;
}

void clearSecondsTimer(byte j)
{
	longMilliSecondsCleared[j] = millis();
	intSeconds[j] = 0;
	intMinutes[j] = 0;
	intHours[j] = 0;
}

byte chargeCycle(byte j)
{
	digitalWrite(chargeMosfetPins[j], HIGH); // Turn on TP4056
	if (digitalRead(chargeLedPins[j]) == HIGH)
	{
		return 1;
	} else {
		return 0;
	}
}

byte milliOhms(byte j)
{
	float resistanceAmps = 0.00;
	float voltageDrop = 0.00;
	float batteryVoltageInput = 0.00;
	float batteryShuntVoltage = 0.00;
	digitalWrite(dischargeMosfetPins[j], LOW);
	getBatteryVoltage(j);
	batteryVoltageInput = batteryVoltage[j];
	digitalWrite(dischargeMosfetPins[j], HIGH);
	getBatteryVoltage(j);
	batteryShuntVoltage = batteryVoltage[j];
	digitalWrite(dischargeMosfetPins[j], LOW);
	resistanceAmps = batteryShuntVoltage / shuntResistor;
	voltageDrop = batteryVoltageInput - batteryShuntVoltage;
	milliOhmsValue[j] = ((voltageDrop / resistanceAmps) * 1000) + offsetMilliOhms; // The Drain-Source On-State Resistance of the IRF504's
	if (milliOhmsValue[j] > 9999) milliOhmsValue[j] = 9999;
	return 1;
}

bool dischargeCycle(byte j)
{
	float batteryShuntVoltage = 0.00;
	intMilliSecondsCount[j] = intMilliSecondsCount[j] + (millis() - longMilliSecondsPreviousCount[j]);
	longMilliSecondsPreviousCount[j] = millis();
	if (intMilliSecondsCount[j] >= 5000 || dischargeAmps[j] == 0) // Get reading every 5+ seconds or if dischargeAmps = 0 then it is first run
	{
		dischargeVoltage[j] = analogRead(batteryVolatgePins[j]) * referenceVoltage / 1023.0;
		batteryShuntVoltage = analogRead(batteryVolatgeDropPins[j]) * referenceVoltage / 1023.0; 
		if(dischargeVoltage[j] >= defaultBatteryCutOffVoltage)
		{
			digitalWrite(dischargeMosfetPins[j], HIGH);
			dischargeAmps[j] = (dischargeVoltage[j] - batteryShuntVoltage) / shuntResistor;
			longMilliSecondsPassed[j] = millis() - longMilliSecondsPrevious[j];
			dischargeMilliamps[j] = dischargeMilliamps[j] + (dischargeAmps[j] * 1000.0) * (longMilliSecondsPassed[j] / 3600000.0);
			longMilliSecondsPrevious[j] = millis();
		}  
		intMilliSecondsCount[j] = 0;
		if(dischargeVoltage[j] < defaultBatteryCutOffVoltage)
		{
			digitalWrite(dischargeMosfetPins[j], LOW);
			return true;
		} 
	} 
	return false;
}

bool batteryCheck(byte j)
{
	getBatteryVoltage(j);
	if (batteryVoltage[j] <= batteryVolatgeLeak) 
	{
		return false;
	} else {
		return true;
	}
}

void getBatteryVoltage(byte j)
{
	float batterySampleVoltage = 0.00;
	for(byte i = 0; i < 10; i++)
	{
		batterySampleVoltage = batterySampleVoltage + analogRead(batteryVolatgePins[j]);
	}
	batterySampleVoltage = batterySampleVoltage / 10; 
	batteryVoltage[j] = batterySampleVoltage * referenceVoltage / 1023.0; 
}

byte checkConnection()
{
	if (client.connect(server, 80)) 
	{
		client.print("GET /check_connection.php?");
		client.print("UserHash=");
		client.print(userHash); 
		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(server);
		client.println("Connection: close");
		client.println();
		client.println();
		return readPage();
	} else {
		return 1;
	}
}

/*
byte checkBatteryExists(byte j)
{
	if (client.connect(server, 80)) 
	{
		client.print("GET /check_battery_barcode.php?");
		client.print("UserHash=");
		client.print(userHash);
		client.print("&");  
		client.print("BatteryBarcode=");
		client.print(batteryBarcode[j]);
		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(server);
		client.println("Connection: close");
		client.println();
		client.println();
		return readPage();
	} else {
		return 1;
	}
}

float getCutOffVoltage(byte j)
{
	if (client.connect(server, 80)) 
	{
		client.print("GET /get_cutoff_voltage.php?");
		client.print("UserHash=");
		client.print(userHash);
		client.print("&");  
		client.print("BatteryBarcode=");
		client.print(batteryBarcode[j]);
		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(server);
		client.println("Connection: close");
		client.println();
		client.println();
		return readPage();
	} else {
		return 1;
	}
}
*/

byte addDischargeRecord(byte j)
{
	if (client.connect(server, 80)) 
	{
		client.print("GET /battery_discharge_post.php?");
		client.print("UserHash=");
		client.print(userHash);
		client.print("&");
		client.print("CDUnitID=");
		client.print(CDUnitID);
		client.print("&");  
		client.print("BatteryBarcode=");
		client.print(batteryBarcode[j]);
		client.print("&");
		client.print("CDModuleNumber=");
		client.print(j + 1);
		client.print("&");
		client.print("MilliOhms=");
		client.print((int) milliOhmsValue[j]);  
		client.print("&");
		client.print("Capacity=");
		client.print((int) dischargeMilliamps[j]);
		client.print("&");
		client.print("BatteryInitialTemp=");
		client.print(batteryInitialTemp[j]);
		client.print("&");
		client.print("BatteryHighestTemp=");
		client.print(batteryHighestTemp[j]);
		client.print("&");
		client.print("DischargeMinutes=");
		client.print(dischargeMinutes[j]);   
		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(server);
		client.println("Connection: close");
		client.println();
		client.println();
		return readPage();
	} else {
		return 1;
	}
}

float readPage()
{
	stringPosition = 0;
	unsigned long startTime = millis(); 
	memset( &serverResult, 0, 32 ); //Clear serverResult memory
	while(true)
	{
		if (millis() < startTime + 10000) // Time out of 10000 milliseconds (Possible cause of crash on Ethernet)
		{
			if (client.available())
			{
				char c = client.read();
				if (c == '<' )  //'<' Start character
				{
					startRead = true; //Ready to start reading the part 
				} else if (startRead) 
				{
					if(c != '>') //'>' End character
					{
						serverResult[stringPosition] = c;
						stringPosition ++;
					} else {
						startRead = false;
						client.stop();
						client.flush();
						if(strstr(serverResult, "CUT_OFF_VOLTAGE_") != 0) 
						{
							char tempCutOffVoltage[10];
							strncpy(tempCutOffVoltage, serverResult + 16, strlen(serverResult) - 15); 
							return (atof(tempCutOffVoltage) + 100); //Battery Cut Off Voltage (+ 100 to avoid conflicts with return codes) 
						}           
						if(strcmp(serverResult, "SUCCESSFUL") == 0) return 0; //SUCCESSFUL
						if(strcmp(serverResult, "ERROR_DATABASE") == 0) return 3; //ERROR_DATABASE
						if(strcmp(serverResult, "ERROR_MISSING_DATA") == 0) return 4; //ERROR_MISSING_DATA
						if(strcmp(serverResult, "ERROR_NO_BARCODE_DB") == 0) return 5; //ERROR_NO_BARCODE_DB
						if(strcmp(serverResult, "ERROR_NO_BARCODE_INPUT") == 0) return 6; //ERROR_NO_BARCODE_INPUT
						if(strcmp(serverResult, "ERROR_DATABASE_HASH_INPUT") == 0) return 7; //ERROR_DATABASE_HASH_INPUT
						if(strcmp(serverResult, "ERROR_HASH_INPUT") == 0) return 8; //ERROR_HASH_INPUT
						// if(strcmp(serverResult, "ERROR_BATTERY_NOT_FOUND") == 0) return 9; //ERROR_BATTERY_NOT_FOUND
					}
				}
			}
		} else {
			client.stop();
			client.flush();
			return 2; //TIMEOUT
		}
	}                                                                                                        
}

