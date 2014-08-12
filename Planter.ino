// Basil Plant Water dispenser
// Samir Chorpa, July 2014

#include <LiquidCrystal.h>

// Pin definitions
#define SOIL_IN A0
#define LCD_D7 2
#define LCD_D6 3
#define LCD_D5 4
#define LCD_D4 5
#define LCD_EN 13
#define LCD_RS 12
#define LCD_BL 11
#define TANK_TRIG 6
#define TANK_ECHO 7
#define RED_BUTTON 8
#define MOTOR_BOT 9
#define MOTOR_TOP 10

// LCD definitions
#define LCD_WIDTH 16
#define LCD_ROWS 2
#define LCD_MAX_BRIGHTNESS 0  //0-255, 0 = on
#define LCD_MIN_BRIGHTNESS 235  //0-255, 255 = off

// Debug
#define BAUD_RATE 9600

// Variables
#define FEED_INTERVAL_MIN 360  
#define LOOP_INTERVAL_MIN 1
#define MIN_SOIL_LEVEL 40
#define MIN_WATER_LEVEL 10
#define BASE_PUMP_TIME_MS 4000
#define ADDITIONAL_PUMP_TIME_MS 30

int sensorValue = 0;  // variable to store the value coming from the soil sensor
int timePassedMin = 0;  // how much time in seconds has passed.
int tankEmpty = 0;
int soilSensorErr = 0;
int pumpErr = 0;
int fadeAmount = 5;
int LCDbrightness = 0;

float minimumRange = 3.0;
float maximumRange = 20.0;
  
char* message = (char *)malloc(sizeof(char) * 16);

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

int pump_en[] = {MOTOR_TOP, MOTOR_BOT};

// Pump Control routines:
void pump_off() {
  digitalWrite(pump_en[0], LOW); 
  digitalWrite(pump_en[1], LOW); 
  delay(25);
}

void pump_on() {
  digitalWrite(pump_en[0], HIGH); 
  digitalWrite(pump_en[1], LOW); 
}


// LED brightness control
void lcd_fadein() {
  while (LCDbrightness >= LCD_MAX_BRIGHTNESS) { 
    analogWrite(LCD_BL, LCDbrightness);
    LCDbrightness -= fadeAmount;
    delay(20);
  } 
}

void lcd_fadeout() {
  while (LCDbrightness <= LCD_MIN_BRIGHTNESS) { 
    LCDbrightness += fadeAmount;
    analogWrite(LCD_BL, LCDbrightness);   
    delay(20);
  }  
}


int readSoil(int num_samples) {
  // read soil moisure value, return a percentage
  // 99 = 0V, 0 = 5V
  
  #define SAMPLE_INTERVAL_MS 2000
  int sum = 0;
   
  // limit the number of samples 
  if (num_samples <= 0) num_samples = 1;
  if (num_samples >= 10) num_samples = 10;
   
  // reads multiple soil samples, returning the average
  for (int i=0; i<num_samples; i++) {
    sensorValue = analogRead(SOIL_IN);
    sum+=sensorValue;
    delay(SAMPLE_INTERVAL_MS);
  } 
  
  sum = sum/10; // scale down values to convert rane from 0-1024 to 0-102. 
  if (sum >= 99*num_samples) sum = 99*num_samples;  // change range to 0-99
  
   return (sum/num_samples);  // 0% - dry soil, 99% - very wet
}


int readWaterLevel() {
  /* This routine reads the distance from the top of the tank lid
     to the top level of water or the bottom of the tank, 
     and returns a percentage representing how much water is in the 
     tank.
  */
  
  // A distance >= minimumRange is a completely empty tank.
  // A distance <= maximumRange is a completely full tank.

  float duration = 0, distance = 0;
 
 /* The following trigPin/echoPin cycle is used to determine the
    distance of the nearest object by bouncing soundwaves off of it.  
    Sourced from: http://arduinobasics.blogspot.com/2012/11/arduinobasics-hc-sr04-ultrasonic-sensor.html
 */
 
 digitalWrite(TANK_TRIG, LOW); 
 delayMicroseconds(2); 
 digitalWrite(TANK_TRIG, HIGH);
 delayMicroseconds(10); 
 digitalWrite(TANK_TRIG, LOW);
 duration = pulseIn(TANK_ECHO, HIGH);
 
 //Calculate the distance (in cm) based on the speed of sound.
 distance = duration/58.2;
 
 if (distance >= maximumRange) return 0;  
 if (distance <= minimumRange) return 99;

 return 99 - (100*(distance-minimumRange))/(maximumRange-minimumRange);    
}


void display_soil_water(int soilLevel, int waterLevel) {
  // Display the soil and water levels one one or two lines
  // Sample: 
  //
  // Soil: 50   H2O: 50
  //
  
  lcd_fadein();
  lcd.clear();
  lcd.setCursor(0,0);
  sprintf(message, "Soil:%d   H2O:%d", soilLevel, waterLevel);
  lcd.print(message);
}

void display_next_feed(int time) {
    lcd.setCursor(0,1);
    if (time >= 60) {
      if (time >= 120) {
        sprintf(message, "Next feed: %d hrs", time/60);
      } else { 
        sprintf(message, "Next feed: %d hr ", time/60);
      }
    } else {
      if (time < 10) {
        sprintf(message, "Next feed: %d min", time);
      } else {
        sprintf(message, "Next feed: %d mn ", time);
      }
    }
    lcd.print(message);
}

void display_tank_empty() {
    lcd.setCursor(0,1);
    sprintf(message, "Water Tank Empty");
    lcd.print(message);
}

void display_soil_err() {
    lcd.setCursor(0,1);
    sprintf(message, "Soil Sensor Err ");
    lcd.print(message);
}

void display_feeding() {
    lcd.setCursor(0,1);
    sprintf(message, "Feeding Plant.. ");
    lcd.print(message);
}

void display_checking() {
    lcd.setCursor(0,1);
    sprintf(message, "Checking Plant.. ");
    lcd.print(message);
}

void display_soilok() {
    lcd.setCursor(0,1);
    sprintf(message, "Soil is moist.  ");
    lcd.print(message);
}

void display_pump_err() {
    lcd.setCursor(0,1);
    sprintf(message, "Water pump error");
    lcd.print(message);
}

void activate_pump(int waterLevel) {
   // the pump is turned on for a time reletive to the water level
   pump_on();
   delay(BASE_PUMP_TIME_MS + (100-waterLevel)*ADDITIONAL_PUMP_TIME_MS);
   pump_off();
}


// ------ Initialization ------

void setup() {

  int soilLevel = 0;
  int waterLevel = 0;
  
  // declare input/output pins
  pinMode (SOIL_IN, INPUT);
  pinMode (LCD_D7, OUTPUT);
  pinMode (LCD_D6, OUTPUT);
  pinMode (LCD_D5, OUTPUT);
  pinMode (LCD_D4, OUTPUT);
  pinMode (LCD_EN, OUTPUT);
  pinMode (LCD_RS, OUTPUT);
  pinMode (LCD_BL, OUTPUT);
  pinMode (TANK_TRIG, OUTPUT);
  pinMode (TANK_ECHO, INPUT);
  pinMode (RED_BUTTON, INPUT);
  pinMode (MOTOR_BOT, OUTPUT);
  pinMode (MOTOR_TOP, OUTPUT);
    
  // LCD init
  lcd.begin(LCD_WIDTH,LCD_ROWS);
       
  // General init
  
  // Turn on LCD to dim, display Welcome messgae
  LCDbrightness = LCD_MIN_BRIGHTNESS;
  analogWrite(LCD_BL, LCDbrightness);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd_fadein();  
  lcd.print("Hello!");
  lcd.setCursor(0,1);
  lcd_fadein();  
  lcd.print("Initializing... ");
  
  // ------ Initialize Water level Sensor ------   
  // ------ Initialize Soil Sensor ------ 
  
  // Turn LCD on, display current soil and water levels 
  soilLevel = readSoil(5);
  waterLevel = readWaterLevel();
  
  display_soil_water(soilLevel, waterLevel);
  delay(2000);
 }


// ------ Main program loop ------
void loop() {
  int soilLevel = 0;
  int waterLevel = 0;

  // Check if acceptible time has passed, otherwise wait for LOOP_INTERVAL minutes and continue
  if (timePassedMin < FEED_INTERVAL_MIN) {
    if (tankEmpty == 0 && soilSensorErr == 0 && pumpErr == 0) {
      // display on the second line when the next feeding is
      display_next_feed(FEED_INTERVAL_MIN-timePassedMin);
      //delay(2000);
      lcd_fadeout();
    }
    
   if (soilSensorErr == 1) {
      // check to see if the soil sensor returns a nonzero value, remove err message if so
      soilLevel = readSoil(5);
      if (soilLevel != 0) {
        soilSensorErr = 0;
        display_soil_water(soilLevel, waterLevel);
      }
    }
    if (tankEmpty == 1) {
      // check to see if the water tank was filled, remove err message if so..
      waterLevel = readWaterLevel();
      if (waterLevel >= MIN_WATER_LEVEL) {
        tankEmpty = 0;
        display_soil_water(soilLevel, waterLevel);
      }
    }
    
    delay((long)LOOP_INTERVAL_MIN*(long)1000 * (long)60);
    timePassedMin = timePassedMin + LOOP_INTERVAL_MIN;
  } else {
    // Check if max number of feedings have taken place in last n hours  TODO

    // Turn LCD on, display current soil and water levels 
    lcd_fadein();
    display_checking();
    soilLevel = readSoil(5);
    waterLevel = readWaterLevel();
    int oldSoilLevel = soilLevel;
    display_soil_water(soilLevel, waterLevel);
    
    if (soilLevel < MIN_SOIL_LEVEL && pumpErr == 0) {
      // Make sure the soil sensor does not read zero, this means it is disconnected.
      if (soilLevel == 0) {
        soilSensorErr = 1;
        display_soil_err();  
      } else {
        // if the tank is empty, display message that feeding cant happen now.
        if (waterLevel < MIN_WATER_LEVEL) {
          display_tank_empty();
          tankEmpty = 1;
        } else {
          tankEmpty = 0;
          display_feeding();
          delay(1000);
          activate_pump(waterLevel);
          
          // wait 10 seconds for water to distribute
          delay(10000);
      
          // recheck soil/water levels
          soilLevel = readSoil(5);
          waterLevel = readWaterLevel();  
          display_soil_water(soilLevel, waterLevel);
   
          delay(1000);
          
          if (oldSoilLevel == soilLevel) {
            // water was not delievered to the plant.  display message
            display_pump_err();
            pumpErr = 1;
          } else {
            pumpErr = 0; 
          }
        }
      }   
    } else {
      if (pumpErr == 0) {
        display_soilok();
        delay(2000);
        lcd_fadeout(); 
      }
    }
    timePassedMin = 0;
  }
}

