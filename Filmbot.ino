
//
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
#include <avr/pgmspace.h>
#include <SPFD5408_Adafruit_GFX.h>    // Core graphics library
#include <SPFD5408_Adafruit_TFTLCD.h> // Hardware-specific library
#include <SPFD5408_TouchScreen.h>     // Touch library

#define I2C_ADDR  0x20  // 0x20 is the address with all jumpers removed
 
// Calibrates value
#define SENSIBILITY 300
#define MINPRESSURE 10
#define MAXPRESSURE 1000

//These are the pins for the shield!
#define YP A1 
#define XM A2 
#define YM 7  
#define XP 6 

short TS_MINX=140;
short TS_MINY=135;
short TS_MAXX=977;
short TS_MAXY=892;
// Init TouchScreen:

TouchScreen ts = TouchScreen(XP, YP, XM, YM, SENSIBILITY);

// LCD Pin

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET 10 // CHANGE TO TEN WHEN USING A SHIELD

// Assign human-readable names to some common 16-bit color values:
#define WHITE   0x0000
#define RED     0xF800
#define BLUE    0x001F
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define BLACK   0xFFFF

// Init LCD
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// Dimensions
uint16_t width = 0;
uint16_t height = 0;

// Buttons
#define BUTTONS 26
#define TEMPUP 3
#define TEMPDOWN 2
#define TEMPSELECT 4

Adafruit_GFX_Button buttons[BUTTONS];

unsigned short AgitationTime = 10000;
unsigned short AgitationFrequency = 30000;
long PumpTime;
long devTime = 0;
long StopTime = 90000;
long FixTime = 480000;

long CycleStartTime;
long CycleTotalTime;
long DevStartTime;
long DevTotalTime;
 
#define Water  1                       
#define Dev  2                       
#define Stop 4                       
#define Fix 8
#define Air 16
#define Waste 32
#define PumpOut 64
#define PumpIn  128

float manualTemp = 24.0;
// int tempPin = 3;
uint8_t devOutput;
uint8_t Stage = 1;

uint16_t buttonx[18] = {160,265,55,265,160,55,160,265,55,160,265,160,160,160,160,160,265,265};
uint16_t buttony[18] = {195,195,110,110,110,110,110,110,195,195,195,110,195,110,195,195,195,195};
uint16_t buttonw = 100;
uint16_t buttonh = 80;
char buttonlabels[11][8] = {"Develop", "Clean","-","+","24","450ML","1000ML","Reuse","Waste","Develop","Reset"};
char films[][7] = {"Tri-X","HP5+","TMY400","Acros","D3200","Atomic"};
char developers[][8] = {"Rod 25","Rod 50","MPX","D76 1:1","D76 1:3","ROD 100"};
const int PROGMEM filmrecipes[][6]= { {420,780,360,0,1500,3600},{360,660,0,780,1200,3600},{0,0,0,0,900,3600},{390,810,0,390,810,3600},{0,0,1020,630,0,3600},{0,0,0,0,780,3600} };

        
void setup() {
  Serial.begin(9600);

  Wire.begin(); // Wake up I2C bus
  Wire.beginTransmission(I2C_ADDR);
  Wire.write(0x00); // IODIRA register
  Wire.write(0x00); // Set all of bank A to outputs
  Wire.endTransmission(); 
  initializeButtons();
  tft.reset();
  tft.begin(0x9341);
  tft.setRotation(3); // Need for the Mega, please changed for your choice or rotation initial
  width = tft.width() - 1;
  height = tft.height() - 1;
  splashScreen();
}

void loop() {
  uint8_t Stage1;
  uint8_t Stage2;
  uint8_t Stage3;
  uint8_t Stage4;
  uint8_t Stage5;
  uint8_t Stage6;
  uint8_t Stage7;

  if (Stage == 1) {
    tft.fillRect(01,01,319,239,BLACK);
    tft.setTextSize (2);
    tft.setCursor (5, 5);
    tft.println(F("//select_program"));
    int bmin = 0;
    int bmax = 1;
    drawButtons(bmin, bmax);
    Stage1 = findTouch(bmin,bmax);
    if (Stage1 == 0) {
      Stage = 2;
      tft.fillRect(01,01,319,239,BLACK); 
    } else {
      
      PumpTime = 10000;
      devTime = 25000;
      StopTime = 25000;
      FixTime = 25000;
      devOutput = 1;
      RunDevCycle();
      StopTime = 90000;
      FixTime = 480000;
      tft.fillRect(01,01,319,239,BLACK);
      tft.setCursor (5, 5);
      tft.println(F("//cleaning_completed"));
      waitOneTouch();
      Stage = 1;


    }
  }

  if (Stage == 2) {
    buttonw = 100;
    tft.setTextSize (2);
    tft.setCursor (5, 5);
    tft.println(F("//ambient_temperature"));
    // SELECT TEMP
    int bmin = 2;
    int bmax = 3;
    drawButtons(bmin,bmax);
    drawTempButtons(TEMPSELECT);
    Stage2 = findTouch(bmin,bmax+1);
    if (Stage2 == TEMPUP) {
      manualTemp = manualTemp + 0.1;
    } else if (Stage2 == TEMPDOWN) {
      manualTemp = manualTemp - 0.1;
    } else if (Stage2 == TEMPSELECT) {
      Stage = 3;
      tft.fillRect(01,01,319,239,BLACK);
    }
  }

  if (Stage == 3) {
    // SELECT FILM
    int bmin = 5;
    int bmax = 10;
    tft.setTextSize (2);
    tft.setCursor (5, 5);
    tft.println(F("//film_stock"));
    drawButtons(bmin,bmax);
    Stage3 = findTouch(bmin,bmax);
    Stage = 4;
    tft.fillRect(01,01,319,239,BLACK);
  }

    if (Stage == 4) {
      // SELECT CHEMS
      int bmin = 11;
      int bmax = 16;
      tft.setTextSize (2);
      tft.setCursor (5, 5);
      tft.println(F("//developer_solution"));
      drawButtons(bmin,bmax);
      Stage4 = findTouch(bmin,bmax);
      tft.fillRect(01,01,319,239,BLACK);
      Stage++;
    }
    
    if (Stage == 5) {
      int bmin = 17;
      int bmax = 18;
      tft.setTextSize (2);
      tft.setCursor (5, 5);
      tft.println(F("//film_container_size"));
      drawButtons(bmin,bmax);
      Stage5 = findTouch(bmin,bmax);
      tft.fillRect(01,01,319,239,BLACK);
      if (Stage5-12 == 5) {
        PumpTime = 28000; // 600 ml  update this for the tiny tank if we get it working
      } else {
        PumpTime = 47000; // 1000 ml
      }
      Serial.print(F("Pump Time Required"));
      Serial.println(PumpTime);
      Stage++;
    }

    if (Stage == 6) {
      int bmin = 19;
      int bmax = 20;
      tft.setTextSize (2);
      tft.setCursor (5, 5);
      tft.println(F("//developer_waste"));
      drawButtons(bmin,bmax);
      Stage6 = findTouch(bmin,bmax);
      tft.fillRect(01,01,319,239,BLACK);
      devOutput = (Stage6-12);
      Stage++;

    }

    if (Stage == 7) {
      
      tft.setTextSize (2);
      tft.setCursor (5, 5);
      tft.println(F("//development_settings"));
      tft.setCursor (5, 25);
      tft.print(F("Temp: "));
      tft.println(manualTemp);
      tft.setCursor (5, 45);
      tft.print(F("Film: "));
      tft.println(films[Stage3-5]);
      tft.setCursor (5, 65);
      tft.print(F("Soln: "));
      tft.println(developers[Stage4-11]);
      tft.setCursor (160,25);
      tft.print(F("Time: "));
      long FilmDevTime = pgm_read_word(&(filmrecipes[Stage3-5][Stage4-11]));   
      devTime = CalcDevTime(FilmDevTime ,manualTemp);
      tft.println(devTime);
      devTime = devTime * 1000;
      tft.setCursor (160,45);
      tft.print(F("Tank: "));
      tft.println(buttonlabels[Stage5-12]);
      tft.setCursor (160,65);
      tft.print(F("Waste: "));
      tft.println(buttonlabels[devOutput]);
      int bmin = 21;
      int bmax = 22;
      drawButtons(bmin,bmax);
      Stage7 = (findTouch(bmin,bmax)) - 21;
      if (Stage7 == 0) {
        RunDevCycle();
      } else {
        Stage = 1;
      }
      tft.fillRect(01,01,319,239,BLACK);
      tft.setCursor (5, 5);
      tft.println(F("//development_completed"));
      waitOneTouch();
      Stage = 1;
    }


}

void splashScreen () {
  tft.fillRect(0,0,320,240,BLACK);
  tft.setCursor (25, 25);
  tft.setTextSize (4);
  tft.setTextColor(RED);
  tft.println(F("FilmBot"));
  tft.setTextSize (2);
  tft.setTextColor(WHITE);
  tft.setCursor (25, 75);
  tft.println(F("Automated"));
  tft.setCursor (25, 100);
  tft.println(F("Film"));
  tft.setCursor (25, 125);
  tft.println(F("Development"));
  tft.setCursor (25, 150);
  tft.println(F("Solution"));
  tft.setCursor (25, 175);
  tft.setTextSize (1);
  tft.println(F("Touch to proceed"));

}
  
int findTouch(int bmin, int bmax) {
  TSPoint p;
  p = waitOneTouch();
  p.x = 320-mapXValue(p);
  p.y = mapYValue(p);
  int menuItem;
  for (uint8_t b=bmin; b<=bmax; b++) {
    if (buttons[b].contains(p.x, p.y)) {
      menuItem = b;
      break;
    }
  }
    return menuItem;
}

void drawButtons(int bmin, int bmax) {
  for (uint8_t b=bmin; b<=bmax; b++) {
    buttons[b].drawButton();
  }
}

void drawTempButtons(int b) {
  //tft.fillRect(130,60,160,180,BLACK);
  char manualTempString[5];
  int buttonw = 100;
  dtostrf(manualTemp,2,1,manualTempString);
  buttons[b].initButton(&tft,                           // TFT object
                  buttonx[b],  buttony[b],                  // x, y,
                  buttonw, buttonh, BLACK, RED, WHITE,    // w, h, outline, fill, 
                  manualTempString, 3);             // text
  buttons[b].drawButton();
}

void initializeButtons() {
  for (uint8_t b=0; b<BUTTONS; b++) {
    if (b > 4 && b < 11) {
      buttons[b].initButton(&tft,                           // TFT object
                  buttonx[b],  buttony[b],                  // x, y,
                  buttonw, buttonh, BLACK, RED, WHITE,    // w, h, outline, fill, 
                  films[b-5], 2);             // text     
    } else if (b > 10 && b < 17) {
       buttons[b].initButton(&tft,                           // TFT object
                    buttonx[b-6],  buttony[b-6],                  // x, y,
                    buttonw, buttonh, BLACK, RED, WHITE,    // w, h, outline, fill, 
                    developers[b-11], 2);             // text     
    } else if (b > 16 && b < 25) {
       buttons[b].initButton(&tft,                           // TFT object
                    buttonx[b-6],  buttony[b-6],                  // x, y,
                    buttonw, buttonh, BLACK, RED, WHITE,    // w, h, outline, fill, 
                    buttonlabels[b-12], 2);             // text     
    } else {
      buttons[b].initButton(&tft,                           // TFT object
                  buttonx[b],  buttony[b],                  // x, y,
                  buttonw, buttonh, BLACK, RED, WHITE,    // w, h, outline, fill, 
                  buttonlabels[b], 2);             // text
    }
  }
}

TSPoint waitOneTouch() {
  TSPoint p;
  do {
    p= ts.getPoint(); 
    pinMode(XM, OUTPUT); //Pins configures again for TFT control
    pinMode(YP, OUTPUT); 
  } while((p.z < MINPRESSURE )|| (p.z > MAXPRESSURE));
  return p;
}

uint16_t mapXValue(TSPoint p) {
  uint16_t x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  return x;
}

uint16_t mapYValue(TSPoint p) {
  uint16_t y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  return y;
}

String RealTime (long timeInSeconds) {
  timeInSeconds = timeInSeconds / 1000;
  int Mins = timeInSeconds / 60;
  int Secs = timeInSeconds % 60;
  String RealTime;
  if (Mins < 10) {
    RealTime = "0";
  }
  RealTime = RealTime + Mins + ":";
  if (Secs < 10 && Secs >= 0) {
    RealTime = RealTime + "0";
  }
  RealTime = RealTime + Secs;
  if (timeInSeconds < 0) {
    RealTime = "00:00";
  }
  return RealTime;
}

long CalcDevTime(long OriginalTime, float tempC) {
  long RevisedTime;
  if (OriginalTime != 3600) {
    RevisedTime = OriginalTime * exp(-0.081 * (tempC - 20));
  } else {
    RevisedTime = OriginalTime;
  }
  return RevisedTime;
}

void DrawProgress (int Ypos, float ProgressPercent, long timeLeft) {
  
  // DEV
  float DevProgressPercent =  (millis()-(float)DevStartTime)/ float(DevTotalTime);
  long DevTimeRemaining = DevTotalTime - millis();
  tft.setTextColor(WHITE,BLACK);
  tft.setCursor(240,70);
  tft.println(RealTime(DevTimeRemaining));

  DevProgressPercent = (296 * DevProgressPercent);
  tft.fillRect(12,(47+(1 * 60)),DevProgressPercent,6,WHITE);

  // PHASE
  tft.setCursor(240,130);
  tft.println(RealTime(CycleTotalTime - millis()));
  float PhaseProgressPercent =  (millis()-(float)CycleStartTime)/ float(CycleTotalTime);
  PhaseProgressPercent = (296 * PhaseProgressPercent);
  tft.fillRect(12,(47+(2 * 60)),PhaseProgressPercent,6,WHITE);
  
  // TASK  
  tft.setCursor(240,190);
  tft.println(RealTime(timeLeft));
  ProgressPercent = (296 * ProgressPercent);
  tft.fillRect(12,(47+(Ypos * 60)),ProgressPercent,6,WHITE);
}

void DrawOutline(int Ypos, String stageText) {
  tft.fillRect(0,(0+(Ypos * 60)),320,60,BLACK);
  tft.fillRect(10,(45+(Ypos * 60)),300,10,WHITE);
  tft.fillRect(11,(46+(Ypos * 60)),298,8,BLACK);
  tft.setCursor(10,(10+Ypos*60));
  tft.println(stageText);
}

void RunDevCycle() {
  long RinseTime = PumpTime * 2.3 + AgitationTime;
  DevStartTime = millis();
  DevTotalTime = devTime + StopTime + FixTime + RinseTime + RinseTime*1.25 + RinseTime*1.5;
  tft.fillRect(0,0,320,60,BLACK);
  tft.setCursor(10,10);
  tft.println(F("Developing Film..."));
  DrawOutline(1,F("Total Progress"));
  DrawOutline(2,F("Pre-rinse Phase"));
  FluidCycle(RinseTime,Water,Waste);
  
  DrawOutline(2,F("Development Phase"));
  if (devOutput > 0) {
    FluidCycle(devTime,Dev,Waste);
  } else {
    FluidCycle(devTime,Dev,Dev);
  }

  DrawOutline(2,F("Stop Phase"));
  FluidCycle(StopTime,Stop,Stop);
  
  DrawOutline(2,F("Fix Phase"));
  FluidCycle(FixTime,Fix,Fix);

  DrawOutline(2,F("Back Wash"));
  BackWashSystem();

  DrawOutline(2,F("Wash 1 Phase"));
  FluidCycle(RinseTime,Water,Waste);
  
  DrawOutline(2,F("Wash 2 Phase"));
  FluidCycle(RinseTime*1.25,Water,Waste);
  
  DrawOutline(2,F("Wash 3 Phase"));
  FluidCycle(RinseTime*1.5,Water,Waste);

  // COMPLETE
}

void Agitate (long int AgitationTime) {
  DrawOutline(3,F("Agitating"));
  long AgiStartTime;
  long AgiFinishTime;
  long AgiTimeRemaining;
  long AgiTimeCompleted;
  float PercentComplete;

  AgiStartTime = millis();
  AgiFinishTime = AgiStartTime + AgitationTime;
  sendValueToLatch(PumpIn+Air);
  String PhaseName = F("Agitating");
  while ((millis()) < AgiFinishTime) {
    
    AgiTimeRemaining = AgiFinishTime - millis();
    AgiTimeCompleted = AgitationTime - AgiTimeRemaining;
    PercentComplete = (float)AgiTimeCompleted/(float)AgitationTime;
    DrawProgress(3,PercentComplete,AgiTimeRemaining);
    delay(200);
  }
  DrawProgress(3,1,0);
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
  long PreviousMillis = 0;
  long ElapsedTime;
  long StartTime;
  long FinishTime;
  long TimeRemaining;
  long CycleAgitationFrequency; 
  long CycleAgitationTime;

  TimeRemaining=0;
  ElapsedTime = 0;
  StartTime = millis();
  Serial.print(F("Started Cycle at: "));
  Serial.println(StartTime);
  FinishTime = StartTime + CycleTime;
  CycleStartTime = StartTime;
  CycleTotalTime = CycleTime+PumpTime+8000;
  PumpFluid(FluidIn,PumpIn,PumpTime); 
  TimeRemaining = FinishTime - millis(); 
  if (CycleTime == 3600000) {
    // FOR STAND DEVELOPMENT
    CycleAgitationFrequency = 1200000;
    CycleAgitationTime = 15000;
    Agitate(CycleAgitationTime);
  } else {
    CycleAgitationFrequency = AgitationFrequency;
    CycleAgitationTime = AgitationTime;
  }
  
  if (FluidIn == Dev) {
    Agitate(30000);
  } else {
  }
  
  while ((millis()) < (FinishTime-(PumpTime+4000))) {
      long CurrentMillis = millis();
      ElapsedTime = millis() - StartTime;
      TimeRemaining = FinishTime - millis();
      // Have I got enough time to Agitate?
      if (TimeRemaining > CycleAgitationTime) {
        if (CurrentMillis - PreviousMillis >= CycleAgitationFrequency) {
          PreviousMillis = CurrentMillis;
          Agitate(CycleAgitationTime);
          DrawOutline(3,"Standing");
        } else {
          float StandPercent = ((float)CurrentMillis - (float)PreviousMillis) / (float) CycleAgitationFrequency;
          long StandRemaining = CycleAgitationFrequency - (CurrentMillis - PreviousMillis);
          DrawProgress(3,StandPercent,StandRemaining);
        }
      }
      ElapsedTime = millis() - StartTime;
      delay(200);
  }
  PumpFluid(FluidOut,PumpOut,PumpTime);
  delay(200);
  Serial.print(F("Antipated Cycle time: "));
  Serial.println(CycleTime);
  Serial.print(F("Total Cycle time: "));
  Serial.println(millis()-StartTime);
  
}

void PumpFluid (int PumpFrom, int PumpDirection, long PumpTimeRequired) {
  long PumpStartTime;
  long PumpFinishTime;
  long PumpTimeRemaining;
  long PumpTimeCompleted;
  float PercentComplete;
  if (PumpDirection == 128) {
    // Deal with inbound pump speed being less than outbound.
    DrawOutline(3, F("Pumping In"));
    PumpTimeRequired = PumpTimeRequired * 1.25;
  } else {
    // Add an extra 2 seconds to ensure all has been evacuated
    DrawOutline(3, F("Pumping Out"));
    PumpTimeRequired = PumpTimeRequired + 4000;
  }
  if (PumpFrom == 1) {
    PumpTimeRequired = PumpTimeRequired * 0.95;
  }
  PumpStartTime = millis();
  PumpFinishTime = PumpStartTime + PumpTimeRequired;
  sendValueToLatch(PumpFrom+PumpDirection);
  while ((millis()) < PumpFinishTime) {
    PumpTimeRemaining = PumpFinishTime - millis();
    PumpTimeCompleted = PumpTimeRequired-PumpTimeRemaining;
    PercentComplete = (float)PumpTimeCompleted/(float)PumpTimeRequired;
    DrawProgress(3,PercentComplete,PumpTimeRemaining);
    delay(200);
  }
  DrawProgress(3,1,0);
  sendValueToLatch(0);
}

void CleanSystem () {
  PumpTime = 500; // 1000 ml
  PumpFluid(Water,128,PumpTime);
  delay(500);
  PumpFluid(Dev,64,PumpTime);
  delay(500);
  PumpFluid(Dev,128,PumpTime);
  delay(500);
  PumpFluid(Stop,64,PumpTime);
  delay(500);
  PumpFluid(Stop,128,PumpTime);
  delay(500);
  PumpFluid(Fix,64,PumpTime);
  delay(500);
  PumpFluid(Fix,128,PumpTime);
  delay(500);
  PumpFluid(Waste,64,PumpTime);
}

void BackWashSystem () {
  int BackWashTime = 2000; // 1000 ml
  PumpFluid(Water,128,BackWashTime*5);
  delay(500);
  PumpFluid(Dev,64,BackWashTime);
  delay(500);
  PumpFluid(Stop,64,BackWashTime);
  delay(500);
  PumpFluid(Fix,64,BackWashTime);
  delay(500);
  PumpFluid(Waste,64,BackWashTime*3);
  delay(500);
}
