Arduino-8x-PCB-Smart-Charger-Discharger
---------------------------------------------------------------------------
Created by Brett Watt on 20/07/2018
Copyright 2018 - Under creative commons license 3.0:

This software is furnished "as is", without technical support, and with no 
warranty, express or implied, as to its usefulness for any purpose.
 
This is the Arduino 8x 18650 Smart Charger / Discharger Code

Current implementation: 
TP4056, Rotary Encoder KY-040 Module, Temp Sensor DS18B20
Ethernet Module W5500, Mini USB Host Shield (Barcode Scanner), 
LCD 2004 20x4 with IIC/I2C/TWI Serial, Discharge (MilliAmps and MillOhms)

Email: info@vortexit.co.nz 
Web: www.vortexit.co.nz

Included Libraries:

Timer.h - https://github.com/JChristensen/Timer

Encoder_Polling.h - https://github.com/frodofski/Encoder_Polling/

DallasTemperature.h, OneWire.h - https://github.com/milesburton/Arduino-Temperature-Control-Library

Ethernet.h - http://www.arduino.cc/en/Reference/Ethernet

hidboot.h, usbhub.h, spi4teensy3.h - https://github.com/felis/USB_Host_Shield_2.0
Change  P10 to P8
//typedef MAX3421e<P10, P9> MAX3421E; // Official Arduinos (UNO, Duemilanove, Mega, 2560, Leonardo, Due etc.), Intel Edison, Intel Galileo 2 or Teensy 2.0 and 3.x - Original
typedef MAX3421e<P8, P9> MAX3421E; // Official Arduinos (UNO, Duemilanove, Mega, 2560, Leonardo, Due etc.), Intel Edison, Intel Galileo 2 or Teensy 2.0 and 3.x
on Line 43 of UsbCore.h in Arduino libraries\USB_Host_Shield_Library_2.0\UsbCore.h

LiquidCrystal_I2C.h - https://github.com/marcoschwartz/LiquidCrystal_I2C
