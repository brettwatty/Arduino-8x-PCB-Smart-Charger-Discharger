// Smart Charger 1.0.0
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
//			 Web: www.vortexit.co.nz


static const uint8_t batteryVolatgePins[] =     {A0,A2,A4,A6,A8,A10,A12,A14};
static const uint8_t batteryVolatgeDropPins[] = {A1,A3,A5,A7,A9,A11,A13,A15};
static const uint8_t chargeMosfetPins[] =       {22,25,28,31,34,37,40,43};
static const uint8_t chargeLedPins[] =          {23,26,29,32,35,38,41,44};
static const uint8_t dischargeMosfetPins[] =    {24,27,30,33,36,39,42,45};

#define ONE_WIRE_BUS 2 // Pin 2 Temperature Sensors | Pin 4 for Version 2.0
#define TEMPERATURE_PRECISION 9

//Includes 
#include <Wire.h> // Comes with Arduino IDE
#include <LiquidCrystal_I2C.h> 
#include "Timer.h"
#include "Encoder_Polling.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <Ethernet2.h>
#include <hidboot.h> //Barcode Scanner
#include <usbhub.h> //Barcode Scanner
#ifdef dobogusinclude //Barcode Scanner
#include <spi4teensy3.h> //Barcode Scanner
#endif

//Objects
LiquidCrystal_I2C lcd(0x27,20,4); // set the LCD address to 0x27 for a 20 chars and 4 line display
Timer timerObject;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

//--------------------------------------------------------------------------
// Constant Variables
const byte modules = 8; // Number of Modules
const float shuntResistor = 3.3; // In Ohms - Shunt resistor resistance
const float referenceVoltage = 4.95; // 5V Output of Arduino
const float defaultBatteryCutOffVoltage = 2.8; // Voltage that the discharge stops ---> Will be replaced with Get DB Discharge Cutoff
const byte restTimeMinutes = 1; // The time in Minutes to rest the battery after charge. 0-59 are valid
const int lowMilliamps = 1000; //  This is the value of Milli Amps that is considered low and does not get recharged because it is considered faulty
const int highMilliOhms = 500; //  This is the value of Milli Ohms that is considered high and the battery is considered faulty
const int offsetMilliOhms = 0; // Offset calibration for MilliOhms
const byte chargingTimeout = 8; // The timeout in Hours for charging
const byte tempThreshold = 7; // Warning Threshold in degrees above initial Temperature 
const byte tempMaxThreshold = 10; //Maximum Threshold in degrees above initial Temperature - Considered Faulty
const byte dischargerID[8] = {2,3,4,5,6,7,8,9}; // Discharger ID's from PHP Database Table Chargers
DeviceAddress tempSensorSerial[8]= {
	{0x28, 0xFF, 0x81, 0x90, 0x63, 0x16, 0x03, 0x4A},
	{0x28, 0xFF, 0xB8, 0xC1, 0x62, 0x16, 0x04, 0x42},
	{0x28, 0xFF, 0xBA, 0xD5, 0x63, 0x16, 0x03, 0xDF},
	{0x28, 0xFF, 0xE7, 0xBB, 0x63, 0x16, 0x03, 0x23},
	{0x28, 0xFF, 0x5D, 0xDC, 0x63, 0x16, 0x03, 0xC7},
	{0x28, 0xFF, 0xE5, 0x45, 0x63, 0x16, 0x03, 0xC4},
	{0x28, 0xFF, 0x0E, 0xBC, 0x63, 0x16, 0x03, 0x8C},
	{0x28, 0xFF, 0xA9, 0x9E, 0x63, 0x16, 0x03, 0x99}
};
// Add in an ambient air tempSensor
const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const char server[] = "submit.vortexit.co.nz";    // Server to connect

IPAddress ip(192, 168, 0, 177); // Set the static IP address to use if the DHCP fails to get assign
EthernetClient client;

//--------------------------------------------------------------------------
// Initialization
unsigned long longMilliSecondsCleared[8];
int intSeconds[8];
int intMinutes[8];
int intHours[8];
//--------------------------------------------------------------------------
// Cycle Variables
byte cycleState[8];
byte cycleStateLast = 0;
byte rotaryOveride = 0;
boolean rotaryOverideLock = 0;
boolean buttonState = 0;
boolean lastButtonState = 0;
boolean mosfetSwitchState[8];
byte cycleStateCycles = 0;
//int connectionAttempts[8];
float batteryVoltage[8];
byte batteryFaultCode[8];
byte batteryInitialTemp[8];
byte batteryHighestTemp[8];
byte batteryCurrentTemp[8];
char lcdLine2[25];
char lcdLine3[25];

// readPage Variables
char serverResult[32]; // string for incoming serial data
int stringPosition = 0; // string index counter readPage()
bool startRead = false; // is reading? readPage()

// Check Battery Voltage
byte batteryDetectedCount[8];
//bool batteryDetected[8];

// Get Barcode
char barcodeString[25] = "";
bool barcodeStringCompleted = false;
byte pendingBarcode = 255;
char batteryBarcode[8][25];
//bool barcodeDuplicateFound[8];

// Cutoff Voltage / Get Barcode
bool batteryGetCompleted[8];
bool pendingBatteryRecord[8];

// Charge Battery
float batteryInitialVoltage[8];
byte batteryChargedCount[8];
boolean preDischargeCompleted[8];

// Check Battery Milli Ohms
byte batteryMilliOhmsCount[8];
float tempMilliOhmsValue[8];
float milliOhmsValue[8];

// Discharge Battery
int intMilliSecondsCount[8];
unsigned long longMilliSecondsPreviousCount[8];
unsigned long longMilliSecondsPrevious[8];
unsigned long longMilliSecondsPassed[8];
float dischargeMilliamps[8];
float dischargeVoltage[8];
float dischargeAmps[8];
float batteryCutOffVoltage[8];
bool dischargeCompleted[8];
bool dischargeUploadCompleted[8];

int dischargeMinutes[8];
bool pendingDischargeRecord[8];

// Recharge Battery
byte batteryRechargedCount[8];

// Completed
byte batteryNotDetectedCount[8];
byte mosfetSwitchCount[8];

// Temp for testing 
byte j = 0;
char lcd0[25];
char lcd1[25];
char lcd2[25];
char lcd3[25];

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
	sensors.begin(); // Start up the library Dallas Temperature IC Control
	encoder_begin(3, 4); // Start the decoder encoder Pin A (CLK) = 3 encoder Pin B (DT) = 4 | Pin 7 (CLK) and Pin 6 (DT) for Version 2.0
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
	sprintf(lcdStartup0, "%-20s", "Smart Charger 1.0.0");
	lcd.setCursor(0,0);
	lcd.print(lcdStartup0);
	sprintf(lcdStartup1, "%-20s", "Initializing TP4056");
	lcd.setCursor(0,1);
	lcd.print(lcdStartup1);
	mosfetSwitchAll();
	sprintf(lcdStartup1, "%-20s", "Initializing W5500..");
	lcd.setCursor(0,1);
	lcd.print(lcdStartup1);
	if (Ethernet.begin(mac) == 0) {
		// Try to congifure using a static IP address instead of DHCP:
		Ethernet.begin(mac, ip);
	}
	char ethIP[16];
	IPAddress ethIp = Ethernet.localIP();
	sprintf(lcdStartup1, "IP:%d.%d.%d.%d%-20s", ethIp[0], ethIp[1], ethIp[2], ethIp[3], " ");
	lcd.setCursor(0,1);
	lcd.print(lcdStartup1);
	float checkConnectionResult = checkConnection();
	if(checkConnectionResult == 0)
	{
		sprintf(lcdStartup2, "%-20s", "DB/WEB: Connected");
	} else if (checkConnectionResult == 2) {
		sprintf(lcdStartup2, "%-20s", "DB/WEB: Time Out");
	} else {
		sprintf(lcdStartup2, "%-20s", "DB/WEB: Failed");
	}
	lcd.setCursor(0,2);
	lcd.print(lcdStartup2);
	lcd.setCursor(0,3);
	sprintf(lcdStartup3, "%-20s", Usb.Init() == -1 ? "USB: Did not start" : "USB: Started");
	lcd.setCursor(0,3);
	lcd.print(lcdStartup3);
	HidKeyboard.SetReportParser(0, &Prs);
	delay(2000);
	// Timer Triggers
	timerObject.every(2, rotaryEncoder);
	timerObject.every(1000, cycleStateValues);
	timerObject.every(1000, cycleStateLCD);
	timerObject.every(60000, cycleStatePostWebData);
	timerObject.every(5000, cycleStateGetWebData);
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
		rotaryOveride = 60; // x (1 min) cycles of cycleStateLCD() decrements - 1 each run until 0
		lcd.clear();
		cycleStateLCD();
	} else { // Button
		buttonState = digitalRead(5);
		if (buttonState != lastButtonState) 
		{
			if (buttonState == LOW) 
			{
				if (rotaryOverideLock == 1)
				{
					rotaryOverideLock = 0;
					rotaryOveride = 0;
				} else {
					if (rotaryOveride > 0) rotaryOverideLock = 1;
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

void cycleStateLCD()
{
	if (rotaryOveride > 0)
	{
		cycleStateLCDOutput(cycleStateLast ,0 ,1);
		if (rotaryOverideLock == 0) rotaryOveride--;
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
	//if (rotaryOveride > 0 && strcmp(lcdLine2, "                    ") != 0 && strcmp(lcdLine3, "                    ") != 0)
	if (rotaryOveride > 0)
	{
		sprintf(lcdLine2, "%-20s", " ");
		sprintf(lcdLine3, "%-18s%02d", " ", rotaryOveride);
		lcd.setCursor(0,2);
		lcd.print(lcdLine2);
		lcd.setCursor(0,3);
		lcd.print(lcdLine3); 
	}
	if (rotaryOveride > 0 && rotaryOverideLock == 1)
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
		sprintf(lcdLine1, "%-20s", strcmp(batteryBarcode[j], "") == 0 ? "NO BARCODE SCANNED!" : batteryBarcode[j]);
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
			if(batteryCheck(i)) batteryDetectedCount[i]++;
			if (batteryDetectedCount[i] == 5)
			{
				batteryCurrentTemp[i] = getTemperature(i);
				batteryInitialTemp[i] = batteryCurrentTemp[i];
				batteryHighestTemp[i] = batteryCurrentTemp[i];
				// Re-Initialization
				batteryDetectedCount[i] = 0;
				preDischargeCompleted[i] = false;
				batteryNotDetectedCount[i] = 0;
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
				mosfetSwitchCount[i] = 0;
				clearSecondsTimer(i);
				getBatteryVoltage(i); // Get battery voltage for Charge Cycle
				batteryInitialVoltage[i] = batteryVoltage[i];
				cycleState[i] = 1; // Check Battery Voltage Completed set cycleState to Get Battery Barcode   
			}
			break;
			
		case 1: // Get Battery Barcode  
			if(strcmp(batteryBarcode[i], "") == 0)
			{
				if (pendingBarcode == 255) pendingBarcode = i;
				// HTTP Check if the Battery Barcode is in the DB
				if (barcodeStringCompleted == true && pendingBarcode == i) 
				{
					//byte connectionResult = checkBatteryExists(i);
					//if (connectionResult == 0 || connectionAttempts[i] >= 10)
					//{
					//connectionAttempts[i] = 0;
					strcpy(batteryBarcode[i], barcodeString);
					/*
			for(byte k = 0; k < modules; k++)
			{
			if (strcmp(batteryBarcode[i], batteryBarcode[k]) == 0 && i != k) 
			{
			barcodeDuplicateFound[i] = true;
			//Barcode Duplicate exists in batteryBarcode ARRAY - (Same barcode scanned twice??)
			}
			}
			*/
					strcpy(barcodeString, "");
					barcodeStringCompleted = false;
					rotaryOveride = 0;
					pendingBarcode = 255;
					cycleState[i] = 2; // Get Battery Barcode Completed set cycleState to Charge Battery
					clearSecondsTimer(i);
					//} else {
					//  connectionAttempts[i]++;
					// Not in DB or connection error
					//}
				} else if (pendingBarcode == i) {
					cycleStateLast = i; // Gets priority and locks cycleLCD Display
					rotaryOveride = 1; // LCD Cycle on this until barcode is scanned
				} 
			}
			//Check if battery has been removed
			if(!batteryCheck(i)) batteryNotDetectedCount[i]++;
			if (batteryNotDetectedCount[i] == 5) cycleState[i] = 0; // Completed and Battery Removed set cycleState to Check Battery Voltage
			break;
			
		case 2: // Charge Battery
			//if (batteryVoltage[i] >= 3.95 && batteryVoltage[i] < 4.18 && intMinutes[i] < 15 && preDischargeCompleted == false) 
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
			if (batteryMilliOhmsCount[i] == 16)
			{
				milliOhmsValue[i] = tempMilliOhmsValue[i] / 16;
				if (milliOhmsValue[i] > highMilliOhms) // Check if Milli Ohms is greater than the set high Milli Ohms value
				{
					batteryFaultCode[i] = 3; // Set the Battery Fault Code to 3 High Milli Ohms
					cycleState[i] = 7; // Milli Ohms is high battery is considered faulty set cycleState to Completed
				} else {
					if (intMinutes[i] <= 1) // No need to rest the battery if it is already charged
					{ 
						//cycleState[i] = 5; // Check Battery Milli Ohms Completed set cycleState to Discharge Battery
						cycleState[i] = 4; // Check Battery Milli Ohms Completed set cycleState to Discharge Battery
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
				batteryRechargedCount[i] = batteryRechargedCount[i] + chargeCycle(i);
				if (batteryRechargedCount[i] == 2)
				{
					digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
					batteryRechargedCount[i] = 0; // ????
					clearSecondsTimer(i);
					cycleState[i] = 7; // Recharge Battery Completed set cycleState to Completed
				} 
			}
			if (intHours[i] == chargingTimeout) // Charging has reached Timeout period. Either battery will not hold charge, has high capacity or the TP4056 is faulty
			{
				digitalWrite(chargeMosfetPins[i], LOW); // Turn off TP4056
				batteryFaultCode[i] = 9; // Set the Battery Fault Code to 9 Charging Timeout
				cycleState[i] = 7; // Charging Timeout. Battery is considered faulty set cycleState to Completed
			}
			getBatteryVoltage(i);
			break;
			
		case 7: // Completed
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
			if (!batteryCheck(i)) batteryNotDetectedCount[i]++;
			if (batteryNotDetectedCount[i] == 5) cycleState[i] = 0; // Completed and Battery Removed set cycleState to Check Battery Voltage
			break;
		}
		secondsTimer(i);
	} 
}

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

int getTemperature(byte j)
{
	sensors.requestTemperaturesByAddress(tempSensorSerial[j]);  
	float tempC = sensors.getTempC(tempSensorSerial[j]);
	return (int) tempC;
}

byte processTemperature(byte j, byte currentTemperature)
{
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
		if(dischargeVoltage[j] >= batteryCutOffVoltage[j])
		{
			digitalWrite(dischargeMosfetPins[j], HIGH);
			dischargeAmps[j] = (dischargeVoltage[j] - batteryShuntVoltage) / shuntResistor;
			longMilliSecondsPassed[j] = millis() - longMilliSecondsPrevious[j];
			dischargeMilliamps[j] = dischargeMilliamps[j] + (dischargeAmps[j] * 1000.0) * (longMilliSecondsPassed[j] / 3600000.0);
			longMilliSecondsPrevious[j] = millis();
		}  
		intMilliSecondsCount[j] = 0;
		if(dischargeVoltage[j] < batteryCutOffVoltage[j])
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
	if (batteryVoltage[j] <= 2.1) 
	{
		return false;
	} else {
		return true;
	}
}

void getBatteryVoltage(byte j)
{
	float batterySampleVoltage = 0.00;
	for(byte i = 0; i < 100; i++)
	{
		batterySampleVoltage = batterySampleVoltage + analogRead(batteryVolatgePins[j]);
	}
	batterySampleVoltage = batterySampleVoltage / 100; 
	batteryVoltage[j] = batterySampleVoltage * referenceVoltage / 1023.0; 
}

byte checkConnection()
{
	if (client.connect(server, 80)) 
	{
		client.print("GET /check_connection.php");
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

byte checkBatteryExists(byte j)
{
	if (client.connect(server, 80)) 
	{
		client.print("GET /check_battery_barcode.php?");
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

byte addDischargeRecord(byte j)
{
	if (client.connect(server, 80)) 
	{
		client.print("GET /battery_discharge_post.php?");
		client.print("BatteryBarcode=");
		client.print(batteryBarcode[j]);
		client.print("&");
		client.print("DischargerID=");
		client.print(dischargerID[j]);
		client.print("&");
		client.print("MilliOhms=");
		client.print((int) milliOhmsValue[j]);  
		client.print("&");
		client.print("DischargeMilliamps=");
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

