// Arduino Smart Charger / Discharger
// ---------------------------------------------------------------------------
// Created by Brett Watt on 20/07/2018
// Copyright 2018 - Under creative commons license 3.0:
//
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
// 
// @brief 
// This is the header file for the Arduino Smart Charger / Discharger
// USB Host as Standard HID
//
// @author Email: info@vortexit.co.nz 
//       Web: www.vortexit.co.nz

#include <hidboot.h> //Barcode Scanner

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

//boolean standardUSBmode = true; // USB mode: true = HidKeyboard and false = HID HIDUniversal

