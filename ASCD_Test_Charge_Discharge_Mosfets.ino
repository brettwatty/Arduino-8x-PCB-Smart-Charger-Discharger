// Test Code for Mosfets Charge and Discharge
//
// Use your multimeter in continuity mode. 
// One probe on GND and the other on the Drain pin of the Mosfet. 
// The code cycles 1 second LOW then 1 second HIGH. 
// You should hear a beep every second.
//
// @author Email: info@vortexit.co.nz 
//       Web: www.vortexit.co.nz

static const uint8_t chargeMosfetPins[] =       {22,25,28,31,34,37,40,43};
static const uint8_t dischargeMosfetPins[] =    {24,27,30,33,36,39,42,45};

const byte modules = 8; // Number of Modules

void setup() 
{
  for(byte i = 0; i < modules; i++)
  { 
    pinMode(chargeMosfetPins[i], OUTPUT);
    pinMode(dischargeMosfetPins[i], OUTPUT);
    digitalWrite(chargeMosfetPins[i], LOW);
    digitalWrite(dischargeMosfetPins[i], LOW);
  }
}

void loop()
{
  for(byte i = 0; i < modules; i++)
  { 
    digitalWrite(chargeMosfetPins[i], HIGH);
    digitalWrite(dischargeMosfetPins[i], HIGH);
  }
  delay(1000);
  for(byte i = 0; i < modules; i++)
  { 
    digitalWrite(chargeMosfetPins[i], LOW);
    digitalWrite(dischargeMosfetPins[i], LOW);
  }
  delay(1000);
}

