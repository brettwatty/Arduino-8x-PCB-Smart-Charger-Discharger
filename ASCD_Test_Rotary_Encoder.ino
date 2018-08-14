// Test Code for Rotary Encoder
// Install Library - Encoder_Polling.h - https://github.com/frodofski/Encoder_Polling/
// Open Serial Monitor in Arduino IDE and turn Rotary Encoder and Press the Button
//
// @author Email: info@vortexit.co.nz 
//       Web: www.vortexit.co.nz

#include "Encoder_Polling.h"

//Pin A (CLK) = 3 encoder Pin B (DT) = 4 for Version 1.1 | Pin 7 (CLK) and Pin 6 (DT) for Version 2.0
#define CLK 7
#define DT 6
#define BTN 5

boolean buttonState = 0;
boolean lastButtonState = 0;

void setup() 
{
  encoder_begin(CLK, DT); // Start the decoder encoder 
  Serial.begin(9600);
}
void loop()
{
  rotaryEncoder();
}

void rotaryEncoder()
{
  int rotaryEncoderDirection = encoder_data(); // Check for rotation
  if(rotaryEncoderDirection != 0)              // Rotary Encoder
  {
    Serial.println(rotaryEncoderDirection);
  } else { // Button
    buttonState = digitalRead(BTN);
    if (buttonState != lastButtonState) 
    {
      if (buttonState == LOW) 
      {
        Serial.println("Button Pressed");
      }
    }
    lastButtonState = buttonState;
  }
}
