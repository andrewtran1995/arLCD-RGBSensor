// EV-CAL
// Uses an ISL29125 to print RGB data onto the arLCD screen similarly to a NIT meter
// NOTE: touching the top-left corner of the screen will convert input to ADC
//
// ADC converted to cd/m^2 using linear regression of y=0.228x-1.4621 w/ R^2=0.998
// Conversion from cd/m^2 to fL uses constant 0.29186
// Color temperature code currently commented out pending possible implementation
//
// Last modified by Andrew Tran on 9/17/2014
//
#include <ezLCDLib.h>
#include <SFE_ISL29125.h>
#include <Wire.h>

// declare objects
ezLCD3 lcd;
SFE_ISL29125 RGB_sensor;

// declare global variables
int mode=1, units=1, prevState=0; // color setting; units setting (0=ADC, 1=nits, 2=fL); min/max previous state
boolean change=true;// detects changes in mode and units
double red, green, blue, result=0, minValue=0, maxValue=0; // min and max values set at 0 if not activated

// global variables specifically for CCT (correlated color temperature) calculation
/*double n, x, y, z, xNorm, yNorm;
const float e = 2.71828;
const double xCCT = 0.3366, yCCT = 0.1735, a0 = -949.86315, a1 = 6253.80338, a2 = 28.70599, a3 = 0.00004, t1 = 0.92159, t2 = 0.20039, t3 = 0.07125;*/

void setup(){
  lcd.begin();
  lcd.cls(BLACK,WHITE);
  
  // intialize ISL29125 and displays message if initialization unsuccessful
  lcd.font("1");
  lcd.printString("\\[65x\\[70yInitializing Sensor ...");
  if(!RGB_sensor.init())
    lcd.printString("\\[20x\\[0yWarning: Sensor Initialization Unsuccessful");
  lcd.color(BLACK);
  lcd.rect(65,70,105,20,1);

  // set up buttons for WHT, RED, YEL, GRN, BLU
  lcd.string(1,"WHT"); // stringID 1
  lcd.string(2,"RED"); // stringID 2
  lcd.string(3,"YEL"); // stringID 3
  lcd.string(4,"GRN"); // stringID 4
  lcd.string(5,"BLU"); // stringID 5
  lcd.theme(1, GRAY, SILVER, BLACK, BLACK, BLACK, WHITE, WHITE, BLACK, WHITE, 0);
  lcd.theme(2, DARKRED, TOMATO, WHITE, WHITE, WHITE, RED, RED, RED, BLACK, 0);
  lcd.theme(3, GOLDENROD, LIGHTYELLOW, BLACK, BLACK, BLACK, YELLOW, YELLOW, YELLOW, BLACK, 0);
  lcd.theme(4, DARKGREEN, LIGHTGREEN, WHITE, WHITE, WHITE, GREEN, GREEN, GREEN, BLACK, 0);
  lcd.theme(5, DARKBLUE, LIGHTBLUE, WHITE, WHITE, WHITE, BLUE, BLUE, BLUE, BLACK, 0);
  lcd.button(1,  0,179,60,60,1,0,20,1,1); // WHT
  lcd.button(2, 64,179,60,60,1,0,20,2,2); // RED
  lcd.button(3,129,179,60,60,1,0,20,3,3); // YEL
  lcd.button(4,194,179,60,60,1,0,20,4,4); // GRN
  lcd.button(5,259,179,60,60,1,0,20,5,5); // BLU

  // set up min/max button
  lcd.string(6,"MIN/\\nMAX");
  lcd.theme(6, DARKVIOLET, LAVENDER, WHITE, VIOLET, WHITE, PURPLE, PURPLE, PURPLE, BLACK, 0);
  lcd.button(6,259,65,60,110,4,0,20,6,6);

  // set up units button
  lcd.string(7,"UNITS");
  lcd.button(7,259,0,60,60,1,0,20,6,7);

  // set up touch zone to show ADC
  lcd.touchZone(8,0,0,60,60,1);

  // set up main digital meter
  lcd.digitalMeter(10,80,50,100,30,14,0,8,0,1);

  // set up min/max widgets
  lcd.theme(7, BLACK, BLACK, DARKVIOLET, DARKVIOLET, DARKVIOLET, WHITE, BLACK, BLACK, BLACK, 0);
  lcd.digitalMeter(11,108,115,90,23,1,0,8,0,7);
  lcd.digitalMeter(12,108,135,90,24,1,0,8,0,7);
  lcd.string(8,"MIN:\\nMAX:");
  lcd.staticText(13,65,115,43,50,1,7,8);

  // set up font for readings
  lcd.font("0");
}

void loop(){
  // check for button presses
  lcd.wstack(0);
  if(lcd.currentInfo != 0)
  {
    if(lcd.currentWidget <= 5 && units != 3) // "units != 3" unneeded if no color temperature
    {
      mode = lcd.currentWidget;
      change = true;
    }
    if(lcd.currentWidget == 8)
    {
      units = 0;
      change = true;
    }
  }
  if(lcd.wvalue(7) == 1)
  {
    if(units == 1)
      units = 2;
    // else if(units == 2)
    // {
    //   units = 3;
    //   mode = 1;
    // }
    else
      units = 1;
    change = true;
  }
  while(lcd.wvalue(7) == 1);

  // read sensor values (16 bit integers)
  red = RGB_sensor.readRed();
  green = RGB_sensor.readGreen();
  blue = RGB_sensor.readBlue();

  // print/change reading and units depending on mode and units
  lcd.color(BLACK);
  switch (mode)
  {
  case 1: // white
    result = red + green + blue;
    convert();
    if(change)
    {
      minValue = maxValue = result;
      lcd.rect(85,85,150,30,1);
      if(units == 1)
        lcd.print("\\[3m\\[85x\\[85ycd/m2 (WHT)");
      else if(units == 2)
        lcd.print("\\[3m\\[85x\\[85yfL (WHT)");
      // else if(units == 3)
      //   lcd.print("\\[3m\\[85x\\[85ycolor temp. Kelvin");
      else
        lcd.print("\\[3m\\[85x\\[85yADC (WHT)");
      change=false;
    }
    break;
  case 2: // red
    result = red;
    convert();
    if(change)
    { 
      minValue = maxValue = result;
      lcd.rect(85,85,150,30,1); 
      if(units == 1)
        lcd.print("\\[4m\\[85x\\[85ycd/m2 (RED)");
      else if(units == 2)
        lcd.print("\\[4m\\[85x\\[85yfL (RED)");
      else
        lcd.print("\\[4m\\[85x\\[85yADC (RED)");
      change=false;
    }
    break;
  case 3: // yellow
    result = red + green;
    convert();
    if(change)
    {
      minValue = maxValue = result;
      lcd.rect(85,85,150,30,1);
      if(units == 1)
        lcd.print("\\[6m\\[85x\\[85ycd/m2 (YEL)");
      else if(units == 2)
        lcd.print("\\[6m\\[85x\\[85yfL (YEL)");
      else
        lcd.print("\\[6m\\[85x\\[85yADC (YEL)");
      change=false;
    }
    break;
  case 4: // green
    result = green;
    convert();
    if(change)
    {
      minValue = maxValue = result;
      lcd.rect(85,85,150,30,1);
      if(units == 1)
        lcd.print("\\[9m\\[85x\\[85ycd/m2 (GRN)");
      else if(units == 2)
        lcd.print("\\[9m\\[85x\\[85yfL (GRN)");
      else
        lcd.print("\\[9m\\[85x\\[85yADC (GRN)");
      change=false;
    }
    break;
  case 5: // blue
    result = blue;
    convert();
    if(change)
    {
      minValue = maxValue = result;
      lcd.rect(85,85,150,30,1);
      if(units == 1)
        lcd.print("\\[12m\\[85x\\[85ycd/m2 (BLU)");
      else if(units == 2)
        lcd.print("\\[12m\\[85x\\[85yfL (BLU)");
      else
        lcd.print("\\[12m\\[85x\\[85yADC (BLU)");
      change=false;
    }
    break;
  }
  lcd.wvalue(10,result);

  // check for min and max if needed
  if(lcd.wvalue(6) == 1)
  {
    if(prevState == 0)
      minValue = maxValue = result;
    else
    {
      if(minValue > result)
        minValue = result;
      if(maxValue < result)
        maxValue = result;
    }
    lcd.wvalue(11,minValue);
    lcd.wvalue(12,maxValue);
  }
  else if(maxValue != 0)
  {
    minValue = maxValue = 0;
    lcd.wvalue(11,minValue);
    lcd.wvalue(12,maxValue);
  }
  prevState = lcd.wvalue(6);

  delay(15);
}

void convert(){
  if(units == 1)
    result = result*0.228 - 1.4621;
  else if(units == 2)
    result = 0.29186*(result*0.228 - 1.4621);
  // else if(units == 3){
    // method 1: converts RGB to XYZ and uses approximation to return CCT (correlated color temperature)
    /*xNorm = x/(x+y+z);
    yNorm = y/(x+y+z);
    n = (xNorm - xCCT)/(yCCT - yNorm);
    result = a0 + a1*pow(e,-n/t1) + a2*pow(e,-n/t2) + a3*pow(e,-n/t3);*/
    // method 2: uses ratio of RGB from CIE 1931 color space to identify color temperature (x for version 1, y for version 2)
    /*x = blue/green;
    x = 948648*pow(x,6) - 3750782.35*pow(x,5) + 5629837.96*pow(x,4) - 3940150.18*pow(x,3) + 1257417.79*pow(x,2) - 139012.9*x + 1594.17;
    y = blue/red;
    y = 126004*pow(y,6) - 572376*pow(y,5) + 985787*pow(y,4) - 786032*pow(y,3) + 284046*pow(y,2) - 32397*y + 1627.2;
    result = y;*/
  // }
}

