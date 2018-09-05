// Test Code for LCD Screen
//
// @author Email: info@vortexit.co.nz 
//       Web: www.vortexit.co.nz

#include <LiquidCrystal_I2C.h> 
LiquidCrystal_I2C lcd(0x27,20,4); // set the LCD address to 0x27 for a 20 chars and 4 line display

void setup() 
{
  lcd.init();
  lcd.clear();
  lcd.backlight();// Turn on backlight
  lcd.setCursor(0,0);
  lcd.print("Testing LCD 20x4    ");
  lcd.setCursor(0,1);
  lcd.print("0123456789ABCDEFGHIJ");
  lcd.setCursor(0,2);
  lcd.print("0123456789ABCDEFGHIJ");
  lcd.setCursor(0,3);
  lcd.print("0123456789ABCDEFGHIJ");
}
  
void loop()
{
  
}


