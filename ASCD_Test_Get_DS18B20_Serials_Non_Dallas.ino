// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses -> Use DallasTemperature oneWireSearch.ino Example to get the DeviceAddress
DeviceAddress tempSensorSerial[9]= {{0x28, 0x0C, 0x01, 0x07, 0x52, 0x43, 0x01, 0x8C},
{0x28, 0x0C, 0x01, 0x07, 0x96, 0x38, 0x01, 0xD4},
{0x28, 0x02, 0x00, 0x07, 0xC0, 0x20, 0x01, 0x3A},
{0x28, 0x02, 0x00, 0x07, 0xB6, 0x43, 0x01, 0x0B},
{0x28, 0x02, 0x00, 0x07, 0x09, 0x3D, 0x01, 0x82},
{0x28, 0x02, 0x00, 0x07, 0x5D, 0x2A, 0x01, 0xE5},
{0x28, 0x07, 0x00, 0x07, 0x1C, 0x58, 0x01, 0xB3},
{0xA8, 0x2A, 0x00, 0x07, 0x34, 0x56, 0x01, 0x96}, 
{0x38, 0x0C, 0x11, 0x07, 0x74, 0x3E, 0x01, 0xBC}};

void setup()
{
  Serial.begin(9600);
  sensors.begin(); // Start up the library Dallas Temperature IC Control
}

void loop()
{ 
  for(byte i = 0; i < 9; i++)
  {   
    
    Serial.print(i + 1);
    Serial.print(" - ");
    Serial.println(getTemperature(i));    
  }
  delay(10000);
}

int getTemperature(int j)
{
  sensors.requestTemperaturesByAddress(tempSensorSerial[j]);  
  float tempC = sensors.getTempC(tempSensorSerial[j]);
  return (int) tempC;
}

