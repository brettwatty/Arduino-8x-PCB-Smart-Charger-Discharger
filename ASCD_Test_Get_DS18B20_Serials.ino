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

// arrays to hold device addresses
//DeviceAddress insideThermometer, outsideThermometer;
DeviceAddress tempSensorSerial[9];

byte deviceCount = 9; // 9 in the Arduino Charger / Discharger
float sensorTempAverage = 0;
bool tempSensorSerialCompleted[10];
bool detectionComplete = FALSE;
byte tempSensorSerialOutput[9]; //Sensors 1 - 9
byte pendingDetection = 0; // This will be from Battery 1 to 8 and then 9 for the ambient temperature

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature Detection");

  // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  if (sensors.getDeviceCount() != deviceCount)
  {
    Serial.println("");
    Serial.print("ERROR did no detect all the Sensors ");
    Serial.print(sensors.getDeviceCount(), DEC);  
    Serial.print(" found. ");
    Serial.print(deviceCount);
    Serial.println(" Sensors should exist.");
    while(1);
  }
  //deviceCount = sensors.getDeviceCount();
  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  oneWire.reset_search();

  for (uint8_t i = 0; i < deviceCount; i++)
  {
    if (!oneWire.search(tempSensorSerial[i])) 
    {
      Serial.print("Unable to find address for ");
      Serial.println(i);
    }
    Serial.print("Device ");
    Serial.print(i);
    Serial.print(" Address: "); 
    printAddress(tempSensorSerial[i]);
    Serial.println();
    sensors.setResolution(tempSensorSerial[i], TEMPERATURE_PRECISION);
    
  }

  sensors.requestTemperatures();
  for (uint8_t i = 0; i < deviceCount; i++)
  {
    sensorTempAverage += sensors.getTempC(tempSensorSerial[i]);
  }
  sensorTempAverage = sensorTempAverage / deviceCount;

}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  Serial.print("{");
  for (uint8_t i = 0; i < 8; i++)
  {
    Serial.print("0x");
    if (deviceAddress[i] < 0x10) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("},"); 
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  //Serial.print(" Temp F: ");
  //Serial.print(DallasTemperature::toFahrenheit(tempC));
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();    
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}


void loop(void)
{ 
  if(detectionComplete == FALSE)
  {
    //Serial.print("Average Temp: ");
    //Serial.println(sensorTempAverage);
    Serial.print("-------------------------------------");  
    Serial.print("Heat Up Battery Sensor: ");
    Serial.print(pendingDetection + 1); 
    Serial.println("-------------------------------------");  
    sensors.requestTemperatures();
    for (uint8_t i = 0; i < deviceCount; i++)
    {
      if(tempSensorSerialCompleted[i] == FALSE)
      {
        if (pendingDetection != (deviceCount - 1)) 
        {
          Serial.print("Sensor ");
          Serial.print(i);
          Serial.print(" Temp: ");
          Serial.println(sensors.getTempC(tempSensorSerial[i]));      
          if(sensors.getTempC(tempSensorSerial[i]) > (2.5 + sensorTempAverage))
          {
            Serial.print("Detected Battery: ");
            Serial.println(pendingDetection + 1);
            tempSensorSerialCompleted[i] = TRUE;
            tempSensorSerialOutput[pendingDetection] = i;
            pendingDetection++; //If not greater than number of devices - last one = ambiant
          }
        } else {
          Serial.println("");
          Serial.println("");
          Serial.println("");
          Serial.println("");
          Serial.print("-------------------------------------");  
          Serial.print("Detected Ambient Sensor Completed");
          Serial.println("-------------------------------------");  
          tempSensorSerialCompleted[i] = TRUE;
          // Got the last one we are done
          tempSensorSerialOutput[pendingDetection] = i;
          detectionComplete = TRUE;
        }
      }
    }
    delay(4000);
  } else {
    //else show serials
    Serial.println("");
    Serial.println("");
    Serial.println("");
    Serial.println("");
    Serial.println("-------------------------------------");  
    Serial.println("Copy and Paste these Addresses into the Arduino Charger / Discharger Sketch");
    Serial.println("-------------------------------------");  
    for (uint8_t i = 0; i < deviceCount; i++)
    {
      printAddress(tempSensorSerial[tempSensorSerialOutput[i]]);
    }
    Serial.println("-------------------------------------");  
    while(1);
    
  }
}

