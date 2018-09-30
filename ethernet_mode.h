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

#include <Ethernet2.h> 										// Or #include <Ethernet.h> depending if it crashes or not

EthernetClient client;

//boolean networkMode = 0; 									// Network mode: false = Ethernet and true = WIFI 
const byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; 	// If you have multiple devices you may need to change this per CD Unit (MAC conflicts on same network???)
IPAddress ip(192, 168, 0, 200); 							// Set the static IP address to use if the DHCP fails to get assign
IPAddress dnServer(192, 168, 0, 1); 						// The DNS Server IP
IPAddress gateway(192, 168, 0, 1); 							// The Router's Gateway Address
IPAddress subnet(255, 255, 255, 0); 						// The Network Subnet

extern SPISettings wiznet_SPI_settings;
