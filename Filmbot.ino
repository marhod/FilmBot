
/* LATCH VALUES
 *  0 = OFF
 *  1 = VALVE 1 (WATER)
 *  2 = VALVE 2 (DEV)
 *  4 = VALVE 3 (STOP)
 *  8 = VALVE 4 (FIX)
 *  16 = VALVE 5 (AIR)
 *  32 = VALVE 6 (WASTE)
 *  64 = PUMP OUT)
 *  128 = PUMP IN)
 */


#include "Wire.h"
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#define I2C_ADDR  0x20  // 0x20 is the address with all jumpers removed

unsigned long AgitationTime = 10000;
unsigned long AgitationFrequency = 60000;
unsigned long PumpTime;

unsigned long devTime = 0;
unsigned long StopTime = 90000;
unsigned long FixTime = 480000;
unsigned long RinseTime = PumpTime * 2.3 + AgitationTime + 60000;
int DevTemp = 38;

#define Water  1                       
#define Dev  2                       
#define Stop 4                       
#define Fix 8
#define Air 16
#define Waste 32
#define PumpOut 64
#define PumpIn  128

const int buttonLeft = 3;
const int buttonRight = 1;
const int buttonGo = 2;
const int buttonPin = 0;     // the number of the pushbutton pin
int buttonValue = 0;
int buttonLeftState = 0;
int buttonRightState = 0;
int buttonGoState = 0;
float tempC;
int reading;
int tempPin = 3;
unsigned long nextUpdate;
int menuRow = 0;
int menuColumn = 0;
int readkey;
int stateChanged = 1;
int rowSize;
int devOutput;

char menuSystem[][3][16]=
      {  {  "DEVELOP", "CLEAN"  },
         {  "RECIPE", "TIME"  },
         {  "","" },
         {  "WASTE","RECYCLE" },
         {  "MF 120","LF 4x5"},
         {  "START", "RESET" },
         {  "CLEAN", "RESET" } };

char recipeList[][16] = {"Tri-X Rod 1:25", "Tri-X Rod 1:50","Tri-X Microphen","Tri-X D76 1:3","HP5+ Rod 1:25","HP5+ Rod 1:50","HP5+ D76 1:1","HP5+ D76 1:3","Acros Rod 1:25","Acros Rod 1:50","Acros D76 1:1","Acros D76 1:3","D100 Rod 1:25","D100 Rod 1:50","D100 D76 1:1","D100 D76 1:3","D3200 D76 1:1","D3200 Microphen","ATO-X D76 1:3","STAND Rod 1:100"};
int recipeTime[] = {420,780,360,1500,360,660,780,1200,390,810,630,1020,540,840,660,1200,720,450,780,3600};
        
void setup() {
  pinMode(buttonLeft, INPUT);
  pinMode(buttonRight, INPUT);
  pinMode(buttonGo, INPUT);
  // put your setup code here, to run once:
  lcd.begin(16, 2);
  lcd.clear();
  // Print a message to the LCD.
  lcd.print("Initialising...");
  // Initialise Relays
  analogReference(INTERNAL);

  Wire.begin(); // Wake up I2C bus

  // Set I/O bank A to outputs
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(0x00); // IODIRA register
  Wire.write(0x00); // Set all of bank A to outputs
  Wire.endTransmission();
  
  Serial.begin(9600);
  Serial.println("Automated Film Developing Solution Starting Up");
  Serial.println(menuSystem[0][0]);
  delay(1000);
  //pinMode(buttonPin, INPUT);
  nextUpdate = millis();
    analogReference(INTERNAL);
}

void loop() {

  reading = analogRead(tempPin);  
  tempC = reading / 9.31;
  if (stateChanged == 1) {
    if (menuRow  == 2) {
      RecipeMenu(menuColumn);
      Serial.println(devTime);
    } else {
      TopMenu(menuColumn, menuRow);
    }

  delay(100);
  stateChanged = 0;
  //Serial.print(menuRow);
  //Serial.print(",");
  //Serial.println(menuColumn);
  }

  if (stateChanged == 0) {
    buttonLeftState = digitalRead(3);
    buttonRightState = digitalRead(10);
    buttonGoState = digitalRead(2);
    /*
    Serial.print(buttonLeftState);
    Serial.print(buttonGoState);
    Serial.println(buttonRightState);
    */
    delay(100);
    if (buttonRightState == HIGH) {
    // RIGHT
      menuColumn++;
      stateChanged = 1;
    }
    else if (buttonLeftState == HIGH) {
    // LEFT
      if (menuColumn > 0) {
        menuColumn--;
      }
      stateChanged = 1;
    }  else if(buttonGoState == HIGH) {
     
      if (menuRow == 0) {
        lcd.clear();
        lcd.print("SELECT FUNCTION");  
      } else if (menuRow == 1) {
        lcd.clear();
        lcd.print("SELECT METHOD");         
      } else if (menuRow == 2) {
        lcd.clear();
        lcd.print("SELECT OPTIONS");         
      } else if (menuRow == 3) {
        lcd.clear();
        lcd.print("SELECT FILM");         
      } else if (menuRow == 5) {
        lcd.clear();
        lcd.print("DRAIN DEV TO");         
      } else if (menuRow == 4) {
        lcd.clear();
        lcd.print("PRESS TO START");         
      } else if (menuRow == 6) {
        lcd.clear();
        lcd.print("PRESS TO START");         
      }
      if (String(menuSystem[menuRow][menuColumn]) == "START") {
        // START
        Serial.println(devTime);
        devTime = devTime * 1000;
        RunDevCycle();
        menuRow=0;
        menuColumn=0;
      } else if (String(menuSystem[menuRow][menuColumn]) == "RECYCLE") {
        // RECYCLE DEV
        devOutput = 0;
        menuRow++;
      } else if (String(menuSystem[menuRow][menuColumn]) == "WASTE") {
        // RECYCLE DEV
        devOutput = 1;
        menuRow++;
      } else if (String(menuSystem[menuRow][menuColumn]) == "MF 120") {
        // RECYCLE DEV
        PumpTime = 33000; // 600 ml
        menuRow++;
      } else if (String(menuSystem[menuRow][menuColumn]) == "LF 4x5") {
        PumpTime = 43000; // 1000 ml
        menuRow++;
      } else if (String(menuSystem[menuRow][menuColumn]) == "CLEAN") {
        // CLEAN
        CleanSystem();
      } else if (String(menuSystem[menuRow][menuColumn]) == "RESET") {
        // RESET
        menuRow=0;
        menuColumn=0;   
      } else {
        menuRow++;
      }
      menuColumn = 0;
      stateChanged = 1;
    }
  }

}

unsigned long RecipeMenu (int menuColumn) {
  reading = analogRead(tempPin);  
  tempC = reading / 9.31;
  //lcd.setCursor(10, 0);
  // lcd.print("T:");
  // lcd.print(tempC);
  devTime = CalcDevTime(recipeTime[menuColumn],tempC);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(recipeList[menuColumn]);
  lcd.setCursor(0, 1);
  lcd.print(RealTime(devTime));
  lcd.print(" at ");
  lcd.print(tempC);
}

void TopMenu (int menuColumn, int menuRow) {
  lcd.clear();
  lcd.setCursor(0, 1);
  for (int i=0;i < 2; i++) {
    if (menuColumn == i) {
      lcd.print("<");
      lcd.print(menuSystem[menuRow][i]);
      lcd.print(">");
    } else {
      lcd.print(" ");
      lcd.print(menuSystem[menuRow][i]);
      lcd.print(" ");
    }
  }
}

String RealTime (int timeInSeconds) {
  int Mins = timeInSeconds / 60;
  int Secs = timeInSeconds % 60;
  String RealTime;
  if (Mins < 10) {
    RealTime = "0";
  }
  RealTime = RealTime + Mins + ":";
  if (Secs < 10) {
    RealTime = RealTime + "0";
  }
    RealTime = RealTime + Secs;
  return RealTime;
}


unsigned long CalcDevTime(unsigned long OriginalTime, float tempC) {
  unsigned long RevisedTime;
  if (OriginalTime != 3600) {
    RevisedTime = OriginalTime * exp(-0.081 * (tempC - 20));
  } else {
    RevisedTime = OriginalTime;
  }
  return RevisedTime;
}

void RunDevCycle() {
  // HeatTanks();
  // Pre Dev Rinse
  lcd.clear();
  lcd.print("PRE-WASH");
  lcd.print("          ");
  Serial.println("1:RINSE");
  FluidCycle(RinseTime,Water,Waste);
  
  // Develop
  lcd.clear();
  lcd.print("DEVELOP");
  lcd.print("          ");
  Serial.println("STAGE 2 - DEV");
  if (devOutput > 0) {
    FluidCycle(devTime,Dev,Waste);
  } else {
    FluidCycle(devTime,Dev,Dev);
  }
  // Stop
  lcd.clear();
  lcd.print("STOP");
  lcd.print("          ");
  Serial.println("STAGE 3 - STOP");
  FluidCycle(StopTime,Stop,Stop);
  
  // Fix
  lcd.clear();
  lcd.print("FIX");
  lcd.print("          ");
  Serial.println("STAGE 4 - FIX");
  FluidCycle(FixTime,Fix,Fix);

  // Wash
  lcd.clear();
  lcd.print("WASH");
  Serial.println("STAGE 5 - WASH");
  FluidCycle(RinseTime,Water,Waste);
  
  lcd.clear();
  lcd.print("WASH 2");
  FluidCycle(RinseTime*1.25,Water,Waste);
  
  lcd.clear();
  lcd.print("WASH 3");
  FluidCycle(RinseTime*1.5,Water,Waste);

  lcd.clear();
  lcd.print("COMPLETE");
  lcd.setCursor(0, 1);
  lcd.print("Remove Film");
}

void HeatTanks () {
  // Code for heating tanks to 38 deg goes here.
  reading = analogRead(tempPin);
  tempC = reading / 9.31;
  //lcd.setCursor(10, 0);
  //lcd.print("T:");
  //lcd.print(tempC);
  if (tempC < DevTemp) {
    // Switch on heater
    //Serial.println("Switching on heater");
  } else {
    // Switch off heater
    //Serial.println("Switching off heater");
  }
}

void Agitate (int AgitationTime) {
  unsigned long AgiStartTime;
  unsigned long AgiFinishTime;
  unsigned long AgiTimeRemaining;
  AgiStartTime = millis();
  AgiFinishTime = AgiStartTime + AgitationTime;
  sendValueToLatch(PumpIn+Air);
  lcd.setCursor(0, 1);
  while ((millis()) < AgiFinishTime) {
    AgiTimeRemaining = AgiFinishTime - millis();
    lcd.setCursor(0, 1);
    lcd.print("Agitating: ");
    lcd.print(RealTime(AgiTimeRemaining/1000));
    lcd.print("          ");
    HeatTanks();
    delay(500);
  }
  sendValueToLatch(0);
}

void sendValueToLatch(int latchValue)
{
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(0x12);        // Select GPIOA
  Wire.write(latchValue);  // Send value to bank A
  Wire.endTransmission();
}

void FluidCycle (float CycleTime,int FluidIn, int FluidOut) {
  Serial.println("Started Cycle");
  Serial.print("Time for Stage : ");
  Serial.print(CycleTime);
  Serial.println(" ms");
  unsigned long PreviousMillis = 0;
  unsigned long ElapsedTime;
  unsigned long StartTime;
  unsigned long FinishTime;
  unsigned long TimeRemaining;
  unsigned long CycleAgitationFrequency; 
  unsigned long CycleAgitationTime;

  TimeRemaining=0;
  ElapsedTime = 0;
  StartTime = millis();
  FinishTime = StartTime + CycleTime - PumpTime;
  PumpFluid(FluidIn,PumpIn); 
  TimeRemaining = FinishTime - millis(); 
  if (CycleTime == RinseTime) {
    CycleAgitationFrequency = 15000;
    CycleAgitationTime = 10000;
  } else if (CycleTime == 3600000) {
    // FOR STAND DEVELOPMENT
    CycleAgitationFrequency = 4000000;
    CycleAgitationTime = 30000;
    Agitate(CycleAgitationTime);
  } else {
    CycleAgitationFrequency = AgitationFrequency;
    CycleAgitationTime = AgitationTime;
  }
  
  while ((millis()) < FinishTime) {
      lcd.setCursor(0, 1);
      lcd.print("Standing: ");
      lcd.print(RealTime((TimeRemaining + PumpTime) / 1000));
      lcd.print("          ");
      unsigned long CurrentMillis = millis();
      ElapsedTime = millis() - StartTime;
      TimeRemaining = FinishTime - millis();
      // Have I got enough time to Agitate?
      if (TimeRemaining > CycleAgitationTime) {
        Serial.println("Check 1");
        if (CurrentMillis - PreviousMillis >= CycleAgitationFrequency) {
          Serial.println("Check 2");
          PreviousMillis = CurrentMillis;
          Agitate(CycleAgitationTime);
        } 
      }
      ElapsedTime = millis() - StartTime;
      HeatTanks();
      delay(500);
  }

  PumpFluid(FluidOut,PumpOut);
  Serial.println("Completed Cycle");
  delay(1000);
}

void PumpFluid (int PumpFrom, int PumpDirection) {
  unsigned long PumpStartTime;
  unsigned long PumpFinishTime;
  unsigned long PumpTimeRemaining;
  unsigned long PumpTimeRequired;
  if (PumpDirection == 128) {
    // Deal with inbound pump speed being less than outbound.
    PumpTimeRequired = PumpTime * 1.25;
  } else {
    // Add an extra 2 seconds to ensure all has been evacuated
    PumpTimeRequired = PumpTime + 2000;
  }
  PumpStartTime = millis();
  PumpFinishTime = PumpStartTime + PumpTimeRequired;
  Serial.println("Pumping fluid");
  sendValueToLatch(PumpFrom+PumpDirection);
  while ((millis()) < PumpFinishTime) {
    PumpTimeRemaining = PumpFinishTime - millis();
    lcd.setCursor(0, 1);
    if (PumpDirection == 128) {
      lcd.print("PUMP IN: ");
    } else {
      lcd.print("PUMP OUT: ");
    }
    lcd.print(RealTime(PumpTimeRemaining/1000));
    lcd.print("          ");
    HeatTanks();
    delay(500);
  }
  sendValueToLatch(0);
  Serial.println("Completed pumping");
}

void CleanSystem () {
  lcd.clear();
  lcd.print("CLEANING SYSTEM");
  FluidCycle(RinseTime,Water,Dev);
  delay(5000);
  FluidCycle(RinseTime,Dev,Waste);
  delay(5000);
  FluidCycle(RinseTime,Water,Stop);
  delay(5000);
  FluidCycle(RinseTime,Stop,Waste);
  delay(5000);
  FluidCycle(RinseTime,Water,Fix);
  delay(5000);
  FluidCycle(RinseTime,Fix,Waste);
  delay(5000);
  FluidCycle(RinseTime,Water,Waste);
  delay(5000);
}

void TestSystem () {
  unsigned long PumpTime = 1000; // TEST AMOUNT ONLY
  lcd.clear();
  lcd.print("TESTING SYSTEM");
  FluidCycle(3,Water,Dev);
  delay(1000);
  FluidCycle(3,Dev,Waste);
  delay(1000);
  FluidCycle(3,Water,Stop);
  delay(1000);
  FluidCycle(3,Stop,Waste);
  delay(1000);
  FluidCycle(3,Water,Fix);
  delay(1000);
  FluidCycle(3,Fix,Waste);
  delay(1000);
  FluidCycle(3,Water,Waste);
  delay(1000);
}
