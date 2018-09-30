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
// version 2.0 PCB PIN definitions
//
// @author Email: info@vortexit.co.nz 
//       Web: www.vortexit.co.nz

#define RX 3
#define TX 2
#define WIFI_BAUD 9600

#include "WiFiEsp.h"
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(RX, TX); 	// RX, TX
#endif

char ssid[] = "YOUR_SSID";			// Your Network SSID (name)
char pass[] = "YOUR_PASSWORD";	// Your Network Password
int status = WL_IDLE_STATUS;     	// The Wifi Radio's Status

WiFiEspClient client; 				// ESP8266 Wifi Client
