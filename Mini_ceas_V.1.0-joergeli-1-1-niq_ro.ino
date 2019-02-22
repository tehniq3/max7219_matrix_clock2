/***********************************************************************

Mini Clock v1.0, Jul 2014 by Nick Hall
Distributed under the terms of the GPL.

For help on how to build the clock see my blog:
http://123led.wordpress.com/

***********************************************************************


Modified to "Multi-Mode Digi Uhr" by joergeli
http://arduino.joergeli.de

Tested with Arduino-IDE v1.6.7 

Modifications and differences to the original:
Using generic MAX72xx Matrixes (therefore chars are rotated 90 degrees!)
Using DS3231 RTC-Module (-compensated = more accurate than DS1307)
Using Arduino-Nano V3.0
Translated daynames, monthnames, etc. to German
Added some "wipe"-effects
Added SmallSlide-Mode
Added Shift-Mode
Added automatic switching approx. every 2 minutes between Small-, Wordclock-, SmallSlide-, Slide- and Shift-mode. (Only when in circle-mode!)
      (No automatic-switching in Basic-Mode!)
Added automatic displaying of dayname, date, month-name, year and week of year  ( when second is 35 )
Modified buttonB as Toggle-Button, which toggles between "Display Date = On" and "Display Date = Off"
Added automatic Daylight Saving Time  (+/- 1 hour)

Reprogrammed code of Wordclock-Mode to look like this: http://arduino.joergeli.de/wordclock/wordclock.php
(shows German-time in steps of 5 minutes, bottom-line shows +1, +2, +3, +4 minutes )

Added DS18B20 Temp-Sensor and displaying it's temperature while in circle-mode when automatic changing of clock-mode occurs (approx. every 2 minutes).
Added Photoresistor (LDR) for automatic changing brightness (therefore brightness-menu removed)

Nicu FLORICA change temperature sensor as DHT22 - 10.020.2019, Craiova, Romania

***********************************************************************/

//include libraries:
#include "LedControl.h"                  // For assigning LED's
//#include <fontDigiClock.h>               // Font library
#include <FontLEDClock1.h>               // Font library
#include <Wire.h>                        // DS1307 clock
#include "RTClib.h"                      // DS1307 clock, works also with DS3231 clock
#include <Button.h>                      // Button library by Alexander Brevig
//#include <OneWire.h>                     // This library allows you to communicate with I2C                   
//#include <DallasTemperature.h>           // For Dallas DS18B20 Temp-Sensor
#include "DHT.h"                         // DHT library


//define constants
#define NUM_DISPLAY_MODES 5              // Number of clock-modes  (counting zero as the first mode)
#define NUM_SETTINGS_MODES 3             // Number of settings modes = 3 (conting zero as the first mode)
#define SLIDE_DELAY 55                   // The time in milliseconds for the slide effect per character in slide mode. Make this higher for a slower effect
#define cls   clear_display              // Clear display

#define LIGHT A0                         // Photoresistor (LDR) for steering brightness
//#define ONE_WIRE_BUS 4                   // Data wire is plugged into pin 4 on the Arduino
//OneWire oneWire(ONE_WIRE_BUS);           // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs) 
//DallasTemperature sensors(&oneWire);     // Pass our oneWire reference to Dallas Temperature.
//DeviceAddress tempDeviceAddress;         // We'll use this variable to store a found device address


#define datain 12
#define cloc 11
#define load 10
#define afisaje 4
#define buton1 2
#define buton2 3
#define buton3 4
#define ledalarma 8
#define senzor 9

//sets the 3 pins as 12, 11 & 10 and then sets 4 displays (max is 8 displays)
//LedControl lc = LedControl(12, 11, 10, 4);
LedControl lc = LedControl(datain, cloc, load, afisaje);


// DHT sensor
#define DHTPIN senzor     // what pin we're connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
// NOTE: For working with a faster chip, like an Arduino Due or Teensy, you
// might need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// Example to initialize DHT sensor for Arduino Due:
//DHT dht(DHTPIN, DHTTYPE, 30);

// Setup LED Matrix
// pin 12 is connected to the DataIn (DIN) on the display
// pin 11 is connected to the CLK on the display
// pin 10 is connected to LOAD (CS) on the display



//global variables
bool debug = true;                      // For debugging only, starts serial output (true/false)
bool show_intro = true;                  // Show intro at startup ?  (true/false)
byte intensity = 15;                      // Startup intensity/brightness (0-15)
bool ampm = false;                       // Define 12 or 24 hour time. false = 24 hour. true = 12 hour
bool show_date = true;                   // Show date? - Display date approx. every 2 minutes (default = true)
bool circle = true;                      // Define circle mode - changes the clock-mode approx. every 2 minutes. Default = true (on)
byte clock_mode = 0;                     // Default clock mode.
                                         // clock_mode 0 = basic mode 
                                         // clock_mode 1 = small mode
                                         // clock_mode 2 = slide mode
                                         // clock_mode 3 = smallslide mode                                          
                                         // clock_mode 4 = word clock
                                         // clock_mode 5 = shift mode
                                         // clock_mode 6 = setup menu
////________________________________________________________________________________________
//Please don't change the following variables:                                                                                                                      
byte old_mode = clock_mode;              // Stores the previous clock mode, so if we go to date or whatever, we know what mode to go back.
short DN;                                // Returns the number of day in the year
short WN;                                // Returns the number of the week in the year
bool date_state = true;                  // Holds state of displaying date 
int devices, dev;                        // Number of LED Matrix-Displays (dev = devices-1)
int rtc[7];                              // Array that holds complete real time clock output
char tempi[4];                           // Holds temperature-chars for displaying temp
char humi[4]; 
char dig[7];                             // Holds time-chars for shift-mode
char shiftChar[8];                       // Holds chars to display in shift-mode
////________________________________________________________________________________________



//day array  (The DS1307/DS3231 outputs 1-7 values for day of week)
char days[7][4] = {
  "Dum", "Lun", "Mar", "Mie", "Joi", "Vin", "Sam"
};
char daysfull[7][9] = {
  "Duminica", "Luni", "Marti", "Miercuri", "Joi", "Vineri", "Sambata"
};
char suffix[1] = {'.'};                  //date suffix "." , used in slide, basic and jumble modes - e.g. date = 25.
                                         //suffix in German is always "."


RTC_DS1307 ds1307;                       // Create RTC object - works also with DS3231

Button buttonA = Button(buton1, BUTTON_PULLUP);      // Setup button A (using button library)
Button buttonB = Button(buton1, BUTTON_PULLUP);      // Setup button B (using button library)

////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  dht.begin();
  
  digitalWrite(buton1, HIGH);                 // turn on pullup resistor for button on pin 2
  digitalWrite(buton2, HIGH);                 // turn on pullup resistor for button on pin 3
  pinMode(LIGHT, INPUT);                 // LDR for brightness
 
  if(debug){
  Serial.begin(9600); //start serial
  Serial.println("Debugging activated ... ");
  }
 
  //initialize the 4 matrix panels
  //we have already set the number of devices when we created the LedControl
  devices = lc.getDeviceCount();
  dev = devices-1;
 
  //we have to init all devices in a loop
  for (int address = 0; address < devices; address++) {
    /*The MAX72XX is in power-saving mode on startup*/
    lc.shutdown(address, false);
    /* Set the brightness to a medium values */
    lc.setIntensity(address, intensity);
    /* and clear the display */
    lc.clearDisplay(address);
  }
/*
  //Setup DS18B20 Temperature-Sensor 
  sensors.begin();                                          // start up Dallas Temperature library
  sensors.getAddress(tempDeviceAddress, 0);                 // get the adress of the first DS18B20 Temp-Sensor
  sensors.requestTemperaturesByAddress(tempDeviceAddress);  // sends command for one device to perform a temperature by address
*/

  //Setup DS1307/DS3231 RTC
  #ifdef AVR
  Wire.begin();    // start I2C communication
  #else
  Wire1.begin();   // Shield I2C pins connect to alt I2C bus on Arduino
  #endif
  ds1307.begin();  //start RTC Clock - works also with DS3231
 
/*
  if (! ds1307.isrunning()) {
    Serial.println("RTC is NOT running!");
    ds1307.adjust(DateTime(__DATE__, __TIME__));  // sets the RTC to the date & time this sketch was compiled
  }
*/

 //Show intro ?
 if(show_intro){ intro(); }

 //Show temperature
 display_temp();
 wipeBottom();

  //Show humidity
 display_hum();
 wipeBottom();


 // Show state of displaying date. toggleDateState() must! run once at startup, otherwise it shows opposite information.
 toggleDateState();

} // end of setup

////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  
  //run the clock with whatever mode is set by clock_mode - the default is set at top of code.
  switch (clock_mode){       
  case 0: 
    basic();
    break; 
  case 1: 
    small(); 
    break;
  case 2: 
    slide(); 
    break;
  case 3: 
    smallslide(); 
    break;  
  case 4: 
    word_clock(); 
    break;
  case 5: 
    shift(); 
    break;
  case 6: 
    setup_menu(); 
    break;
  }

  
} // end of loop


////////////////////////////////////////////////////////////////////////////////////////

// plot: plot a dot at positon xy with val 0/1 
void plot (byte x, byte y, byte val) {
  //select which matrix depending on the x coord
  byte address;
  y=7-y;
  if (x >= 0 && x <= 7)   {
    address = 3;
  }
  if (x >= 8 && x <= 15)  {
    address = 2;
    x = x - 8;
  }
  if (x >= 16 && x <= 23) {
    address = 1;
    x = x - 16;
  }
  if (x >= 24 && x <= 31) {
    address = 0;
    x = x - 24;
  }

  if (val == 1) {
    lc.setLed(address, 7 - y, x, true);
  } else {
    lc.setLed(address, 7 - y, x, false);
  }
}


////////////////////////////////////////////////////////////////////////////////////////

//clear screen
void clear_display() {
  for (byte address = 0; address < 4; address++) {
    lc.clearDisplay(address);
  }
}

////////////////////////////////////////////////////////////////////////////////////////

// setBright: set the brightness to a value between 0 and 15 (= 16 steps, in dependence of LDR)
int setBright(){
  // map LDR-values from 0 to 15 and set the brightness of devices
  
//  int brightness = map(analogRead(LIGHT), 0, 1023, 0, 15);
int brightness = 1;

  //we have to init all devices in a loop
  for (int address = 0; address < devices; address++) {
    lc.setIntensity(address, brightness);
  }
  return brightness;
}

////////////////////////////////////////////////////////////////////////////////////////

// fade_high: fade intensity from 0 to brightness (in dependence of LDR)
void fade_high() {

  // map LDR-values from 0 to 15
  int brightness = map(analogRead(LIGHT), 0, 1023, 0, 15);
  
  //fade from intensity 0 to brightness and set the brightness of devices
  for (byte f=0; f<=brightness; f++) {
    for (byte address = 0; address < 4; address++) {
      lc.setIntensity(address, f);
    }
    delay(120); //change this to alter fade-up speed
  }
  return;
}

////////////////////////////////////////////////////////////////////////////////////////
// fade_low: fade intensity from brightness (in dependence of LDR) to 0
void fade_low() {

  // map LDR-values from 0 to 15
  int brightness = map(analogRead(LIGHT), 0, 1023, 0, 15);
 
  //fade from brightness to 1 and set the brightness of devices
  for (byte f=brightness; f>0; f--) {
    for (byte address = 0; address < 4; address++) {
      lc.setIntensity(address, f);
    }
    delay(120); //change this to alter fade-low speed
  }  
  for (byte address = 0; address < 4; address++) {
    lc.setIntensity(address, 0);  // set intensity to lowest level
  }
  return;
}

////////////////////////////////////////////////////////////////////////////////////////

//intro: show intro at startup
void intro() {

  for (byte address = 0; address < 4; address++) {
       lc.setIntensity(address, 3);
  }

  for(int i=0; i<2; i++){
      wipeBottom();
      wipeTop();
  }
  wipeOutside();


  char ver_a[9] = "clock"; 
  char ver_b[9] = "v.1.4. ";

  for (byte address = 0; address < 4; address++) {
       lc.setIntensity(address, 0);
  }

 byte i = 0;
  while (ver_a[i]) {
    delay(80);
    puttinychar((i * 4), 1, ver_a[i]);
    i++;
  }
  fade_high();
  delay(200);
  fade_low();
  delay(500);
  wipeOutside();

  i = 0;
  while (ver_b[i]) {
    delay(80);
    puttinychar((i * 4), 1, ver_b[i]);
    i++;
  } 
  fade_high();
  delay(200);
  fade_low(); 
  delay(500);
  wipeMiddle();


} // end of intro

////////////////////////////////////////////////////////////////////////////////////////

// puttinychar:
// Copy a 3x5 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
// This is unoptimized and simply uses plot() to draw each dot.
void puttinychar(byte x, byte y, char c){
  byte dots;
  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
    c &= 0x1F;   // A-Z maps to 1-26
  }
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 32;
  }
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 27; // full stop
  }
  else if (c == ':') {
    c = 28; // colon
  }
  else if (c == '\'') {
    c = 29; // single quote mark
  }
  else if (c == '!') {
    c = 30; // exclamation mark
  }
 else if (c == '`') {
    c = 67; // degree
  }
  else if (c == '+') {
    c = 68; // plus
  }
   else if (c == '-') {
    c = 69; // plus
  }
  else if (c == '%') {
    c = 70; // procent
  }
   else if (c == '~') {
    c = 71; // degree Celsius
  }
  else if (c >= -80 && c <= -67) {
    c *= -1;
  }
  
  
  for (byte col = 0; col < 3; col++) {
    dots = pgm_read_byte_near(&mytinyfont[c][col]);
    for (char row = 0; row < 5; row++) {
      if (dots & (16 >> row))
        plot(x + col, y + row, 1);
      else
        plot(x + col, y + row, 0);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////

//putnormalchar:
//Copy a 5x7 character glyph from the myfont data structure to display memory
void putnormalchar(byte x, byte y, char c){
  byte dots;
  if (c >= 'A' && c <= 'Z' ) {
    c &= 0x1F;   // A-Z maps to 1-26
  }
  else if (c >= 'a' && c <= 'z') {
    c = (c - 'a') + 41;   // A-Z maps to 41-67
  }
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 31;
  }
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 27; // full stop
  }
  else if (c == '\'') {
    c = 28; // single quote mark
  }
  else if (c == ':') {
    c = 29; // colon
  }
  else if (c == '>') {
    c = 30; // clock_mode selector arrow
  }
   else if (c == '`') {
    c = 67; // degree
  }
  else if (c == '+') {
    c = 68; // plus
  }
   else if (c == '-') {
    c = 69; // plus
  }
  else if (c == '%') {
    c = 70; // procent
  }
   else if (c == '~') {
    c = 71; // degree Celsius
  }
  else if (c >= -80 && c <= -67) {
    c *= -1;
  }

  for (char col = 0; col < 5; col++) {
    dots = pgm_read_byte_near(&myfont[c][col]);
    for (char row = 0; row < 7; row++) {
      //check coords are on screen before trying to plot
      //if ((x >= 0) && (x <= 31) && (y >= 0) && (y <= 7)){

      if (dots & (64 >> row)) {   // only 7 rows.
        plot(x + col, y + row, 1);
      } else {
        plot(x + col, y + row, 0);
      }
      //}
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////

// small(=mode 1): show the time in small 3x5 characters with seconds-dots at bottom-line
void small() {
  char textchar[8]; // the 16 characters on the display
  byte mins = 100; //mins
  byte secs = rtc[0]; //seconds
  byte old_secs = secs; //holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed
  
  cls();

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {
  get_time();
  secs = rtc[0];

  //check for button presses
  if (buttonA.uniquePress()) { switch_mode();  return; }
  if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }


  // when in circle mode and minute=even and second=14, switch to word_clock (mode 4)
  if(circle){
    if(rtc[1] % 2 == 0 && rtc[0]==14){
       wipeInside();
       clock_mode =1;  // switch to wordclock mode
       return;
    }
  }


  //if secs changed then update them on the display
  if (secs != old_secs) {

  bottomleds(secs); // plot seconds-dots at bottomline

  // display date, when second=40  and date_state = true
  if(rtc[0]==40  && date_state){
     display_date();
     return;    
    }
      
      char buffer[3];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      puttinychar( 20, 1, ':'); //seconds colon
      puttinychar( 24, 1, buffer[0]); //seconds
      puttinychar( 28, 1, buffer[1]); //seconds
      old_secs = secs;
    }

    //if minute changes change time
    if (mins != rtc[1]) {

      //reset these for comparison next time
      mins = rtc[1];
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }


      //byte dow  = rtc[3]; // the DS1307/DS3231 outputs 0 - 6 where 0 = Sunday0 - 6 where 0 = Sunday.
      //byte date = rtc[4];

      //set characters
      char buffer[3];
      itoa(hours, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ".
      if (hours < 10) {
        buffer[1] = buffer[0];
        //if we are in 12 hour mode blank the leading zero.
        if (ampm) {
          buffer[0] = ' ';
        }
        else {
          buffer[0] = '0';
        }
      }
      //set hours chars
      textchar[0] = buffer[0];
      textchar[1] = buffer[1];
      textchar[2] = ':';

      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set mins characters
      textchar[3] = buffer[0];
      textchar[4] = buffer[1];

      //do seconds
      textchar[5] = ':';
      buffer[3];
      secs = rtc[0];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set seconds
      textchar[6] = buffer[0];
      textchar[7] = buffer[1];

      byte x = 0;
      byte y = 0;

      //print each char
      for (byte x = 0; x < 6 ; x++) {
        puttinychar( x * 4, 1, textchar[x]);
      }
    }
    delay(50);
  } // end of while run_mode
}


////////////////////////////////////////////////////////////////////////////////////////

// basic(= mode 0): simple mode shows the time in 5x7 characters
void basic(){
  cls();

  char buffer[3];   //for int to char conversion to turn rtc values into chars we can print on screen
  byte offset = 0;  //used to offset the x postition of the digits and centre the display when we are in 12 hour mode and the clock shows only 3 digits. e.g. 3:21
  byte x, y;        //used to draw a clear box over the left hand "1" of the display when we roll from 12:59 -> 1:00am in 12 hour mode.

  //do 12/24 hour conversion if ampm set to 1
  byte hours = rtc[2];

  if (hours > 12) {
    hours = hours - ampm * 12;
  }
  if (hours < 1) {
    hours = hours + ampm * 12;
  }

  //do offset conversion
  if (ampm && hours < 10) {
    offset = 2;
  }
  else{
    offset = 0;
  }
  
  //set the next minute we show the date at
  //set_next_date();
  
  // initially set mins to value 100 - so it wll never equal rtc[1] on the first loop of the clock, meaning we draw the clock display when we enter the function
  byte secs = 100;
  byte mins = 100;
  int count = 0;
  
  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    //get the time from the clock chip
    get_time();
   
    //check for button press
    if (buttonA.uniquePress()) { switch_mode(); return;  }
    if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }


    // display temp, when second=40 and minute=even and date_state=true
    if(rtc[0]==40 && rtc[1] % 2 == 0 && date_state){
       wipeBottom();
       display_temp();
       wipeTop();
       return; 
      }

    if(rtc[0]==50 && rtc[1] % 2 == 0 && date_state){
       wipeBottom();
       display_hum();
       wipeTop();
       return; 
      }
    
    // display date, when second=40 and minute=odd and date_state = true
    if(rtc[0]==40 && rtc[1] % 2 == 1 && date_state){
       display_date();
       return;    
      }

    //draw the flashing colon on/off if the secs have changed.
    if (secs != rtc[0]) {     
       secs = rtc[0];  //update secs with new value
   
      //Blink "::"
      if(secs % 2 == 0){
      plot (14 - offset, 2, 1); //top point
      plot (14 - offset, 1, 1); //top point
      plot (15 - offset, 3, 1); //top point
      plot (16 - offset, 2, 1); //top point
      plot (16 - offset, 1, 1); //top point
      plot (14 - offset, 5, 1); //bottom point
      plot (14 - offset, 6, 1); //bottom point
      plot (15 - offset, 4, 1); //bottom point
      plot (16 - offset, 5, 1); //bottom point
      plot (16 - offset, 6, 1); //bottom point
      }
      else {
      plot (14 - offset, 2, 0); //top point
      plot (14 - offset, 1, 0); //top point
      plot (16 - offset, 2, 0); //top point
      plot (16 - offset, 1, 0); //top point
      plot (14 - offset, 5, 0); //bottom point
      plot (14 - offset, 6, 0); //bottom point
      plot (16 - offset, 5, 0); //bottom point
      plot (16 - offset, 6, 0); //bottom point
      }
    }
   
    //redraw the display if button pressed or if mins != rtc[1]
    if (mins != rtc[1]) {

      //update mins and hours with the new values
      mins = rtc[1];
      hours = rtc[2];

      //adjust hours of ampm set to 12 hour mode
      if (hours > 12) { hours = hours - ampm * 12; }
      if (hours < 1) { hours = hours + ampm * 12;  }

      itoa(hours, buffer, 10);

      //if hours < 10 the num e.g. "3" hours, itoa coverts this to chars with space "3 " which we dont want
      if (hours < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      //print hours
      //if we in 12 hour mode and hours < 10, then don't print the leading zero, and set the offset so we centre the display with 3 digits.
      if (ampm && hours < 10) {
        offset = 2;

        //if the time is 1:00am clear the entire display as the offset changes at this time and we need to blank out the old 12:59
        if ((hours == 1 && mins == 0) ) {
          cls();
        }
      }
      else {
        //else no offset and print hours tens digit
        offset = 0;
        
              //if the time is 10:00am clear the entire display as the offset changes at this time and we need to blank out the old 9:59
              if (hours == 10 && mins == 0) {
              cls();
              }
              
         putnormalchar(1,  0, buffer[0]);
      }
      
      //print hours ones digit
      putnormalchar(7 - offset, 0, buffer[1]);

      //print mins
      //add leading zero if mins < 10 
      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //print mins tens and mins ones digits
      putnormalchar(19 - offset, 0, buffer[0]);
      putnormalchar(25 - offset, 0, buffer[1]);
    } // end of if (mins != rtc[1]
  } // end of while run_mode
}

////////////////////////////////////////////////////////////////////////////////////////

//Big-Slide mode (=mode 2): like basic-mode, but with sliding digits top-down
void slide() {
  byte digits_old[4] = {99, 99, 99, 99}; //old values  we store time in. Set to somthing that will never match the time initially so all digits get drawn wnen the mode starts
  byte digits_new[4]; //new digits time will slide to reveal
  byte digits_x_pos[4] = {25, 19, 7, 1}; //x pos for which to draw each digit at

  char old_char[2]; //used when we use itoa to transpose the current digit (type byte) into a char to pass to the animation function
  char new_char[2]; //used when we use itoa to transpose the new digit (type byte) into a char to pass to the animation function

  //old_chars - stores the 5 day and date suffix chars on the display. e.g. "mon" and "st". We feed these into the slide animation as the current char when these chars are updated.
  //We sent them as A initially, which are used when the clocl enters the mode and no last chars are stored.
  //char old_chars[6] = "AAAAA";

  cls();
  
  // plot the clock colon on the display
  //  putnormalchar( 13, 0, ':');

  byte old_secs = rtc[0]; //store seconds in old_secs. We compare secs and old secs. WHen they are different we redraw the display

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    get_time();
    byte secs =rtc[0];


  // display date, when second=40 and date_state = true
  if(rtc[0]==40 && date_state){
     display_date();
     return;    
    }
      

  // when in circle mode and minute=even and second=15, switch to shift mode (mode 5)
  if(circle){
    if(rtc[1] % 2 == 0 && rtc[0]==15){
       wipeMiddle();
       display_temp();
       wipeTop();
       clock_mode =2;  // switch to shift mode
       return;
    }
  }
    
    //check for button press
    if (buttonA.uniquePress()) { switch_mode();  return; }
    if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }

    //if secs have changed then update the display
    if (rtc[0] != old_secs) {

     //Blink "::"
      if(old_secs % 2 == 0){       
        plot(14, 4, 1);
        plot(14, 5, 1);
        plot(14, 6, 1);
        plot(14, 2, 0);
        plot(14, 1, 0);
        plot(14, 0, 0);
        plot(15, 3, 1);       
        plot(16, 4, 0);
        plot(16, 5, 0);
        plot(16, 6, 0);
        plot(16, 2, 1);
        plot(16, 1, 1);
        plot(16, 0, 1);         
      }
      else {
        plot(14, 4, 0);
        plot(14, 5, 0);
        plot(14, 6, 0);
        plot(14, 2, 1);
        plot(14, 1, 1);
        plot(14, 0, 1);
        plot(16, 4, 1);
        plot(16, 5, 1);
        plot(16, 6, 1);
        plot(16, 2, 0);
        plot(16, 1, 0);
        plot(16, 0, 0);       
      }
     
      old_secs = rtc[0];

      //do 12/24 hour conversion if ampm set to 1
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      //split all date and time into individual digits - stick in digits_new array

      //rtc[0] = secs                        //array pos and digit stored
      //digits_new[0] = (rtc[0]%10);           //0 - secs ones
      //digits_new[1] = ((rtc[0]/10)%10);      //1 - secs tens
      //rtc[1] = mins
      digits_new[0] = (rtc[1] % 10);         //2 - mins ones
      digits_new[1] = ((rtc[1] / 10) % 10);  //3 - mins tens
      //rtc[2] = hours
      digits_new[2] = (hours % 10);         //4 - hour ones
      digits_new[3] = ((hours / 10) % 10);  //5 - hour tens
      //rtc[4] = date
      //digits_new[6] = (rtc[4]%10);           //6 - date ones
      //digits_new[7] = ((rtc[4]/10)%10);      //7 - date tens

      //draw initial screen of all chars. After this we just draw the changes.

      //compare digits 0 to 3 (mins and hours)
      for (byte i = 0; i <= 3; i++) {
        //see if digit has changed...
        if (digits_old[i] != digits_new[i]) {

          //run 9 step animation sequence for each in turn
          for (byte seq = 0; seq <= 8 ; seq++) {

            //convert digit to string
            itoa(digits_old[i], old_char, 10);
            itoa(digits_new[i], new_char, 10);

            //if set to 12 hour mode and we're on digit 2 (hours tens mode) then check to see if this is a zero. If it is, blank it instead so we get 2.00pm not 02.00pm
            if (ampm && i == 3) {
              if (digits_new[3] == 0) {
                new_char[0] = ' ';
              }
              if (digits_old[3] == 0) {
                old_char[0] = ' ';
              }
            }
            //draw the animation frame for each digit
            slideanim(digits_x_pos[i], 0, seq, old_char[0], new_char[0]);
            delay(SLIDE_DELAY);
          }
        }
      }

 
      //save digita array tol old for comparison next loop
      for (byte i = 0; i <= 3; i++) {
        digits_old[i] =  digits_new[i];
      }
    }// end of secs/oldsecs
  }// end of while run_mode
}

////////////////////////////////////////////////////////////////////////////////////////

//called by slide
//this draws the animation of one char sliding on and the other sliding off. There are 8 steps in the animation, we call the function to draw one of the steps from 0-7
//inputs are are char x and y, animation frame sequence (0-7) and the current and new chars being drawn.
void slideanim(byte x, byte y, byte sequence, char current_c, char new_c) {

  //  To slide one char off and another on we need 9 steps or frames in sequence...

  //  seq# 0123456 <-rows of the display
  //   |   |||||||
  //  seq0 0123456  START - all rows of the display 0-6 show the current characters rows 0-6
  //  seq1  012345  current char moves down one row on the display. We only see it's rows 0-5. There are at display positions 1-6 There is a blank row inserted at the top
  //  seq2 6 01234  current char moves down 2 rows. we now only see rows 0-4 at display rows 2-6 on the display. Row 1 of the display is blank. Row 0 shows row 6 of the new char
  //  seq3 56 0123
  //  seq4 456 012  half old / half new char
  //  seq5 3456 01
  //  seq6 23456 0
  //  seq7 123456
  //  seq8 0123456  END - all rows show the new char

  //from above we can see...
  //currentchar runs 0-6 then 0-5 then 0-4 all the way to 0. starting Y position increases by 1 row each time.
  //new char runs 6 then 5-6 then 4-6 then 3-6. starting Y position increases by 1 row each time.

  //if sequence number is below 7, we need to draw the current char
  if (sequence < 7) {
    byte dots;
    if (current_c >= 'A' && current_c <= 'Z' ) {
      current_c &= 0x1F;   // A-Z maps to 1-26
    }
    else if (current_c >= 'a' && current_c <= 'z') {
      current_c = (current_c - 'a') + 41;   // a-z maps to 41-66
    }
    else if (current_c >= '0' && current_c <= '9') {
      current_c = (current_c - '0') + 31;
    }
    else if (current_c == ' ') {
      current_c = 0; // space
    }
    else if (current_c == '.') {
      current_c = 27; // full stop
    }
    else if (current_c == '\'') {
      current_c = 28; // single quote mark
    }
    else if (current_c == ':') {
      current_c = 29; //colon
    }
    else if (current_c == '>') {
      current_c = 30; // clock_mode selector arrow
    }

    byte curr_char_row_max = 7 - sequence; //the maximum number of rows to draw is 6 - sequence number
    byte start_y = sequence; //y position to start at - is same as sequence number. We inc this each loop

    //plot each row up to row maximum (calculated from sequence number)
    for (byte curr_char_row = 0; curr_char_row <= curr_char_row_max; curr_char_row++) {
      for (byte col = 0; col < 5; col++) {
        dots = pgm_read_byte_near(&myfont[current_c][col]);
        if (dots & (64 >> curr_char_row))
          plot(x + col, y + start_y, 1); //plot led on
        else
          plot(x + col, y + start_y, 0); //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
  }

  //draw a blank line between the characters if sequence is between 1 and 7. If we don't do this we get the remnants of the current chars last position left on the display
  if (sequence >= 1 && sequence <= 8) {
    for (byte col = 0; col < 5; col++) {
      plot(x + col, y + (sequence - 1), 0); //the y position to draw the line is equivalent to the sequence number - 1
    }
  }

  //if sequence is above 2, we also need to start drawing the new char
  if (sequence >= 2) {

    //work out char
    byte dots;
    //if (new_c >= 'A' && new_c <= 'Z' || (new_c >= 'a' && new_c <= 'z') ) {
    //  new_c &= 0x1F;   // A-Z maps to 1-26
    //}
    if (new_c >= 'A' && new_c <= 'Z' ) {
      new_c &= 0x1F;   // A-Z maps to 1-26
    }
    else if (new_c >= 'a' && new_c <= 'z') {
      new_c = (new_c - 'a') + 41;   // A-Z maps to 41-67
    }
    else if (new_c >= '0' && new_c <= '9') {
      new_c = (new_c - '0') + 31;
    }
    else if (new_c == ' ') {
      new_c = 0; // space
    }
    else if (new_c == '.') {
      new_c = 27; // full stop
    }
    else if (new_c == '\'') {
      new_c = 28; // single quote mark
    }
    else if (new_c == ':') {
      new_c = 29; // clock_mode selector arrow
    }
    else if (new_c == '>') {
      new_c = 30; // clock_mode selector arrow
    }

    byte newcharrowmin = 6 - (sequence - 2); //minimumm row num to draw for new char - this generates an output of 6 to 0 when fed sequence numbers 2-8. This is the minimum row to draw for the new char
    byte start_y = 0; //y position to start at - is same as sequence number. we inc it each row

    //plot each row up from row minimum (calculated by sequence number) up to 6
    for (byte newcharrow = newcharrowmin; newcharrow <= 6; newcharrow++) {
      for (byte col = 0; col < 5; col++) {
        dots = pgm_read_byte_near(&myfont[new_c][col]);
        if (dots & (64 >> newcharrow))
          plot(x + col, y + start_y, 1); //plot led on
        else
          plot(x + col, y + start_y, 0); //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////

//Small-Slide-mode (mode 3): like small-mode, but with sliding digits top-down
void smallslide() {
  byte digits_old[6] = {99, 99, 99, 99, 99, 99}; //old values  we store time in. Set to somthing that will never match the time initially so all digits get drawn wnen the mode starts
  byte digits_new[6]; //new digits time will slide to reveal
  byte digits_x_pos[6] = {29, 25, 17, 13, 5, 1}; //x pos for which to draw each digit at

  char old_char[2]; //used when we use itoa to transpose the current digit (type byte) into a char to pass to the animation function
  char new_char[2]; //used when we use itoa to transpose the new digit (type byte) into a char to pass to the animation function
  
  byte old_secs = rtc[0]; //store seconds in old_secs. We compare secs and old secs. WHen they are different we redraw the display
  
  cls();
  
  //run clock main loop as long as run_mode returns true
  while (run_mode()) {
   get_time();

      
  // when in circle mode and minute=odd and second=12, switch to slide mode (mode 2)
  if(circle){
    if(rtc[1] % 2 == 1 && rtc[0]==12){
       wipeInside();
       display_temp();
       wipeTop();
       clock_mode =3;  // switch to slide mode
       return;
    }
  }

    //if secs have changed then update the display
    if (rtc[0] != old_secs) {
      old_secs = rtc[0];

      //do 12/24 hour conversion if ampm set to 1
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      //split all date and time into individual digits - stick in digits_new array
      //rtc[0] = secs                        //array pos and digit stored
      digits_new[0] = (rtc[0]%10);           //0 - secs ones
      digits_new[1] = ((rtc[0]/10)%10);      //1 - secs tens
      //rtc[1] = mins
      digits_new[2] = (rtc[1] % 10);         //2 - mins ones
      digits_new[3] = ((rtc[1] / 10) % 10);  //3 - mins tens
      //rtc[2] = hours
      digits_new[4] = (hours % 10);          //4 - hour ones
      digits_new[5] = ((hours / 10) % 10);   //5 - hour tens
      //rtc[4] = date
      //digits_new[6] = (rtc[4]%10);         //6 - date ones
      //digits_new[7] = ((rtc[4]/10)%10);    //7 - date tens

      //draw initial screen of all chars. After this we just draw the changes.

      //compare digits 0 to 5 (secs, mins and hours)
      for (byte i = 0; i <= 5; i++) {
        //see if digit has changed...
        if (digits_old[i] != digits_new[i]) {

          //run 9 step animation sequence for each in turn
          for (byte seq = 0; seq <= 8 ; seq++) {

            //convert digit to string
            itoa(digits_old[i], old_char, 10);
            itoa(digits_new[i], new_char, 10);

            //if set to 12 hour mode and we're on digit 5 (hours tens mode) then check to see if this is a zero. If it is, blank it instead so we get 2.00pm not 02.00pm
            if (ampm && i == 5) {
              if (digits_new[5] == 0) {
                new_char[0] = ' ';
              }
              if (digits_old[5] == 0) {
                old_char[0] = ' ';
              }
            }
            //draw the animation frame for each digit
            slideTyniAnim(digits_x_pos[i], 0, seq, old_char[0], new_char[0]);
                      //hold display but check for button presses
                      int counter = 35; // = slide animation-time
                      while (counter > 0){
                      //check for button press
                        if (buttonA.uniquePress()) { switch_mode();  return; }
                        if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
                      delay(1);
                      counter--;
                      }
          }
        }
      }

       // plot the clock colon on the display
       putnormalchar( 8, 0, ':');
       putnormalchar( 20, 0, ':');


 
      //save digita array tol old for comparison next loop
      for (byte i = 0; i <= 5; i++) {
        digits_old[i] =  digits_new[i];
      }
    }//secs %2
  }//end of while run_mode
}

////////////////////////////////////////////////////////////////////////////////////////

//called by smallslide_mode
//this draws the animation of one char sliding on and the other sliding off. There are 8 steps in the animation, we call the function to draw one of the steps from 0-7
//inputs are are char x and y, animation frame sequence (0-7) and the current and new chars being drawn.
void slideTyniAnim(byte x, byte y, byte sequence, char current_c, char new_c) {

  //  To slide one char off and another on we need 9 steps or frames in sequence...

  //  seq# 0123456 <-rows of the display
  //   |   |||||||
  //  seq0 0123456  START - all rows of the display 0-6 show the current characters rows 0-6
  //  seq1  012345  current char moves down one row on the display. We only see it's rows 0-5. There are at display positions 1-6 There is a blank row inserted at the top
  //  seq2 6 01234  current char moves down 2 rows. we now only see rows 0-4 at display rows 2-6 on the display. Row 1 of the display is blank. Row 0 shows row 6 of the new char
  //  seq3 56 0123
  //  seq4 456 012  half old / half new char
  //  seq5 3456 01
  //  seq6 23456 0
  //  seq7 123456
  //  seq8 0123456  END - all rows show the new char

  //from above we can see...
  //currentchar runs 0-6 then 0-5 then 0-4 all the way to 0. starting Y position increases by 1 row each time.
  //new char runs 6 then 5-6 then 4-6 then 3-6. starting Y position increases by 1 row each time.

  //if sequence number is below 7, we need to draw the current char
  if (sequence < 7) {
    byte dots;
    if (current_c >= 'A' && current_c <= 'Z' ) {
      current_c &= 0x1F;   // A-Z maps to 1-26
    }
    else if (current_c >= 'a' && current_c <= 'z') {
      current_c = (current_c - 'a') + 41;   // A-Z maps to 41-67
    }
    else if (current_c >= '0' && current_c <= '9') {
      current_c = (current_c - '0') + 31;
    }
    else if (current_c == ' ') {
      current_c = 0; // space
    }
    else if (current_c == '.') {
      current_c = 27; // full stop
    }
    else if (current_c == '\'') {
      current_c = 28; // single quote mark
    }
    else if (current_c == ':') {
      current_c = 29; //colon
    }
    else if (current_c == '>') {
      current_c = 30; // clock_mode selector arrow
    }
    
   // byte curr_char_row_max = 6 - sequence; //(6) the maximum number of rows to draw is 6 - sequence number
    byte curr_char_row_max = 7 - sequence; //(6) the maximum number of rows to draw is 6 - sequence number
    byte start_y = sequence; //y position to start at - is same as sequence number. We inc this each loop

    //plot each row up to row maximum (calculated from sequence number)
    for (byte curr_char_row = 0; curr_char_row <= curr_char_row_max; curr_char_row++) {
      for (byte col = 0; col < 3; col++) { 
        dots = pgm_read_byte_near(&mytinyfont[current_c+1][col]);            
        if (dots & (64 >> curr_char_row))
          plot(x + col, y + start_y, 1); //plot led on
        else
          plot(x + col, y + start_y, 0); //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
 }

  //draw a blank line between the characters if sequence is between 1 and 7. If we don't do this we get the remnants of the current chars last position left on the display
  if (sequence >= 1 && sequence <= 8) {
    for (byte col = 0; col < 2; col++) {
      plot(x + col, y + (sequence - 1), 0); //the y position to draw the line is equivalent to the sequence number - 1
    }
  }

  //if sequence is above 2, we also need to start drawing the new char
  if (sequence >= 2) {

    //work out char
    byte dots;
    if (new_c >= 'A' && new_c <= 'Z' ) {
      new_c &= 0x1F;   // A-Z maps to 1-26
    }
    else if (new_c >= 'a' && new_c <= 'z') {
      new_c &= 0x1F;   // A-Z maps to 1-26
    }
    else if (new_c >= '0' && new_c <= '9') {
      new_c = (new_c - '0') + 32;
    }
    else if (new_c == ' ') {
      new_c = 0; // space
    }
    else if (new_c == '.') {
      new_c = 27; // full stop
    }
    else if (new_c == ':') {
      new_c = 28; // doppelpunkt
    }
    else if (new_c == '\'') {
      new_c = 29; // clock_mode selector arrow
    }
    else if (new_c == '!') {
      new_c = 30; // clock_mode selector arrow
    }

    byte newcharrowmin = 7 - (sequence - 2); //minimumm row num to draw for new char - this generates an output of 6 to 0 when fed sequence numbers 2-8. This is the minimum row to draw for the new char
    byte start_y = 0; //y position to start at - is same as sequence number. we inc it each row

    //plot each row up from row minimum (calculated by sequence number) up to 6
    for (byte newcharrow = newcharrowmin; newcharrow <= 6; newcharrow++) {
      for (byte col = 0; col < 3; col++) {
        dots = pgm_read_byte_near(&mytinyfont[new_c][col]);
        if (dots & (64 >> newcharrow))
          plot(x + col, y + start_y, 1); //plot led on
        else
          plot(x + col, y + start_y, 0); //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////


// word_clock (= mode4): show the time using words rather than numbers
void word_clock() {

  //potentially 5 lines to display
  char str_0[8];
  char str_a[8];
  char str_b[8];
  char str_c[8];
  char str_d[8];
  char str_e[8];

  
  //byte hours_y, mins_y; //hours and mins and positions for hours and mins lines
  byte hours = rtc[2];
  if (hours > 12) { hours = hours - ampm * 12; }
  if (hours < 1)  { hours = hours + ampm * 12; }

//  get_time(); //get the time from the clock chip

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {
    get_time(); //get the time from the clock chip

        // when in circle mode and minute=odd and second is between 14 and 30, switch to smallslide-mode (mode 3)
        if(circle){
                 if(rtc[1] % 2 == 1 && (rtc[0] >= 14 && rtc[0] <=30 )){
                  clock_mode =4;  // switch to smallslide-mode (mode 3)
                  return;
                 }
                 else{
                   cls();
                   display_temp();
              
                   //hold display but check for button presses
                   int counter = 20;
                   while (counter > 0){
                   if (buttonA.uniquePress()) { switch_mode();  return; }
                   if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
                   delay(1);
                   counter--;
                   }
                 }
          }

    get_time();
    // display date when second is between 35 and 45 and date_state = true
    if((rtc[0] >= 35 && rtc[0] <= 45) && date_state){
        display_date();
        return;    
       }
     else{
          wipeOutside();
         }



    get_time();
    byte mins =  rtc[1];  //get mins
    hours = rtc[2];
 
    //make hours into 12 hour format
    if (hours > 12) { hours = hours - 12; }
    if (hours == 0) { hours = 12; }
    
       byte len = 0;
       int halb=0;
      
       if      (mins >= 5 && mins <= 9)  { strcpy (str_b, "SI");           strcpy (str_c, "");          strcpy (str_d, "CINCI"); }
       else if (mins >= 10 && mins <= 14){ strcpy (str_b, "SI");           strcpy (str_c, "");          strcpy (str_d, "ZECE"); }
       else if (mins >= 15 && mins <= 19){ strcpy (str_b, "SI");           strcpy (str_c, "");          strcpy (str_d, "UN SFERT"); }
       else if (mins >= 20 && mins <= 24){ strcpy (str_b, "SI");           strcpy (str_c, "SI");        strcpy (str_d, "DOUAZECI"); halb = 0; }      
       else if (mins >= 25 && mins <= 29){ strcpy (str_b, "DOIZECI");      strcpy (str_c, "");          strcpy (str_d, "SI CINCI"); halb = 0; }      
       else if (mins >= 30 && mins <= 34){ strcpy (str_b, "SI");           strcpy (str_c, "SI");        strcpy (str_d, "TREIZECI"); halb = 0; }      
       else if (mins >= 35 && mins <= 39){ strcpy (str_b, "TRIZECI");      strcpy (str_c, "SI");        strcpy (str_d, "CINCI"); halb = 0; }      
       else if (mins >= 40 && mins <= 44){ strcpy (str_b, "FARA");         strcpy (str_c, "");          strcpy (str_d, "DOUAZECI"); halb = 1; }       
       else if (mins >= 45 && mins <= 49){ strcpy (str_b, "FARA");         strcpy (str_c, "");          strcpy (str_d, "UN SFERT"); halb = 1; }     
       else if (mins >= 50 && mins <= 54){ strcpy (str_b, "FARA");         strcpy (str_c, "");          strcpy (str_d, "ZECE"); halb = 1; }       
       else if (mins >= 55 && mins <= 59){ strcpy (str_b, "FARA");         strcpy (str_c, "");          strcpy (str_d, "CINCI"); halb = 1; } 

      int wordHour = hours + halb;
      if(wordHour > 12){ wordHour = 1;}
      
      if ( wordHour == 1 ) {
         strcpy (str_a, "UNU");    
         if (mins >= 5 && mins <= 59){  // if minute between 5-59 add "S" to "EIN" -> "EINS"
          strcpy (str_a, "UNUS");
          }
      }
      else if ( wordHour == 2 ) { strcpy (str_a, "DOI"); } 
      else if ( wordHour == 3 ) { strcpy (str_a, "TREI"); } 
      else if ( wordHour == 4 ) { strcpy (str_a, "PATRU"); }
      else if ( wordHour == 5 ) { strcpy (str_a, "CINCI"); } 
      else if ( wordHour == 6 ) { strcpy (str_a, "SASE"); } 
      else if ( wordHour == 7 ) { strcpy (str_a, "SAPTE"); }
      else if ( wordHour == 8 ) { strcpy (str_a, "OPT"); } 
      else if ( wordHour == 9 ) { strcpy (str_a, "NOUA"); } 
      else if ( wordHour == 10) { strcpy (str_a, "ZECE"); }
      else if ( wordHour == 11) { strcpy (str_a, "UNSPE"); } 
      else if ( wordHour == 12) { strcpy (str_a, "DOISPE"); } 

      if (mins <= 4){
         strcpy (str_a, "");
         strcpy (str_b, "");
         strcpy (str_c, "");
         strcpy (str_e, ""); 
         }
      else{
         strcpy (str_e, "");        
      }
      
    //end working out time

    //run in a loop
    setBright();  // set brightness of devices   
    int delayChar = 60; // delay between displaying next char

    String dstring(str_d);
    String estring(str_e);  
    
    //print line_0 / this line is always shown
    strcpy (str_0, "ESTE");
    len = 0;
    while (str_0[len]) {
      len++;
    } //get length of message
    byte offset_top = (31 - ((len - 1) * 4)) / 2; //
    byte i = 0;
    while (str_0[i]) {
      puttinychar((i * 4) + offset_top, 1, str_0[i]);
      delay(delayChar);
      i++;
    }
 
    //hold display but check for button presses
    int counter = 900;
    while (counter > 0){
    //check for button press
    if (buttonA.uniquePress()) { switch_mode();  return; }
    if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
    delay(1);
    counter--;
    }
    cls();

    // Check minutes-LEDS at bottom-line          
    if ((mins-(mins/5)*5)==1)      
        {plot(13,7,1);
          plot(15,7,0);
          plot(17,7,0);
          plot(19,7,0);         
        }        
    else if((mins-(mins/5)*5)==2)      
        {plot(13,7,1);
         plot(15,7,1);
         plot(17,7,0);
         plot(19,7,0);
        }        
    else if((mins-(mins/5)*5)==3)      
        {plot(13,7,1);
         plot(15,7,1);
         plot(17,7,1);
         plot(19,7,0);
        }        
    else if((mins-(mins/5)*5)==4)      
        {plot(13,7,1);
         plot(15,7,1);
         plot(17,7,1);
         plot(19,7,1);
        }
    else {plot(13,7,0);
         plot(15,7,0);
         plot(17,7,0);
         plot(19,7,0);
        }          


    //print line a / 5minute-intervall
    char al = str_a[0];
    if(al>1){
    len = 0;
    while (str_a[len]) {
      len++;
    } //get length of message
    byte offset_top = (31 - ((len - 1) * 4)) / 2; //
    byte i = 0;
    while (str_a[i]) {
      puttinychar((i * 4) + offset_top, 1, str_a[i]);
      delay(delayChar);
      i++;
    }
    //hold display but check for button presses
    counter = 900;
    while (counter > 0){
    //check for button press
    if (buttonA.uniquePress()) { switch_mode();  return; }
    if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
      delay(1);
      counter--;
    }
    cls();
   }



    //print line b, if not empty / ""/""/""
    char bl = str_b[0];
    if(bl>1){
    len = 0;
    while (str_b[len]) {
      len++;
    } //get length of message
    byte offset_top = (31 - ((len - 1) * 4)) / 2; 
    byte i = 0;
    while (str_b[i]) {
      puttinychar((i * 4) + offset_top, 1, str_b[i]);
      delay(delayChar);
      i++;
    }
    //hold display but check for button presses
    counter = 900;
    while (counter > 0){
    //check for button press
    if (buttonA.uniquePress()) { switch_mode();  return; }
    if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
      delay(1);
      counter--;
    }
    cls();
   }



    //print line c, if not empty / ""
    char cl = str_c[0];
    if(cl>1){
    len = 0;
    while (str_c[len]) {
      len++;
    } //get length of message
    byte offset_top = (31 - ((len - 1) * 4)) / 2; 
    byte i = 0;
    while (str_c[i]) {
      puttinychar((i * 4) + offset_top, 1, str_c[i]);
      delay(delayChar);
      i++;
    }
    //hold display but check for button presses
    counter = 900;
    while (counter > 0){
    //check for button press
    if (buttonA.uniquePress()) { switch_mode();  return; }
    if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
      delay(1);
      counter--;
    }
    cls();
   }


    //print line d, if not empty / Hours
    char dl = str_d[0]; 
    if(dl>1){
    len = 0;
    while (str_d[len]) {
      len++;
    } //get length of message   
    byte offset_top = (31 - ((len - 1) * 4)) / 2; 
    byte i = 0;
    while (str_d[i]) {
      puttinychar((i * 4) + offset_top, 1, str_d[i]);
      delay(delayChar);
      i++;
    }
    
    //hold display but check for button presses
    counter = 870;
    while (counter > 0){
    if (buttonA.uniquePress()) { switch_mode();  return; }
    if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
      delay(1);
      counter--;
    }
    if(estring.length() != 0 ){ cls(); } 
   }


    //print line e, if not empty  / "Uhr/""
    char el = str_e[0];
    if(el>1){
    len = 0;
    while (str_e[len]) {
      len++;
    } //get length of message
    byte offset_top = (31 - ((len - 1) * 4)) / 2; 
    byte i = 0;
    while (str_e[i]) {
      puttinychar((i * 4) + offset_top, 1, str_e[i]);
      delay(delayChar);
      i++;
    }
    //hold display but check for button presses
    counter = 900;
    while (counter > 0){
    if (buttonA.uniquePress()) { switch_mode();  return; }
    if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
      delay(1);
      counter--;
    }
    if(estring.length() == 0 ){ cls(); }
  }

  //wipe out devices
  wipeMiddle();

 
 } // end of while run-mode
} //end of wordclock

////////////////////////////////////////////////////////////////////////////////////////


/// shift-mode (=mode5):  shift time-chars from right to left and back
void shift() {
  
 while (run_mode()) {
  setBright(); 
  cls();
  get_time();

      // when in circle mode and minute=odd and second is between 4 and 15, switch to small mode (mode 1)
      if(circle){
        if(rtc[1] % 2 == 1 && (rtc[0] >= 4 && rtc[0] <=15 )){
          wipeMiddle();
          display_temp();
          wipeTop();
          clock_mode =5;  // switch to small mode
          return;
        }
      }
      
  bool secflag= false;
  get_shiftTime(secflag);
      shiftChar[0] = dig[5];
      shiftChar[1] = dig[4];
      shiftChar[2] = ':';
      shiftChar[3] = dig[3];
      shiftChar[4] = dig[2];
      shiftChar[5] = ':';
      shiftChar[6] = dig[1];
      shiftChar[7] = dig[0];   
//________________________________________________
// shift chars from right to left inside
int x;
int y=1;
int nr=0;
int w=0;

  do{
      for(x=34; x>=w; x--){      
        puttinychar(x+w, y, shiftChar[nr]);
        for (byte yy = 0 ; yy < 7; yy ++) {
              plot(x+w + 3, yy, 0);     
        }
      }
      w=w+2;
      nr++;
    } while(w<=28);
    
//________________________________________________
    setBright();   
    get_time();
     
      // when in circle mode and minute=odd and second is between 4 and 15, switch to small mode (mode 1)
      if(circle){
        if(rtc[1] % 2 == 1 && (rtc[0] >= 4 && rtc[0] <=15 )){
          wipeMiddle();
          display_temp();
          wipeTop();
          clock_mode =1;  // switch to small mode
          return;
        }
      }
      
  // get 5x new chars to display, when there is no shifting
   for (int i=0; i<5; i++) {
      setBright();
      bool secflag= true;  // -4 seconds, because the shifting of previous chars was approx. 4 seconds in the past
      get_shiftTime(secflag);   
      shiftChar[0] = dig[5];
      shiftChar[1] = dig[4];
      shiftChar[2] = ':';
      shiftChar[3] = dig[3];
      shiftChar[4] = dig[2];
      shiftChar[5] = ':';
      shiftChar[6] = dig[1];
      shiftChar[7] = dig[0];

      puttinychar( 0, 1, shiftChar[0]);
      puttinychar( 4, 1, shiftChar[1]);
      puttinychar(12, 1, shiftChar[3]);
      puttinychar(16, 1, shiftChar[4]);
      puttinychar(24, 1, shiftChar[6]);
      puttinychar(28, 1, shiftChar[7]);

         //hold display but check for button presses
         int counter = 860; // while in for-loop, stop 860ms before displaying next char
         while (counter > 0){
            if (buttonA.uniquePress()) { switch_mode();  return; }
            if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
          delay(1);
          counter--;
          }
   }

//________________________________________________
// shift chars from left to right outside
x=0;
nr=7;
    for(int w=28; w>=0; w-=4){  
        do{      
            puttinychar(x+w, y, shiftChar[nr]);
            for(byte yy = 0 ; yy < 7; yy ++) {
                plot(x+w - 1, yy, 0);        
            } 
          x=x+1;
        } while(x<=34);
      x=0;
      nr--;
    }
//________________________________________________


         //hold display but check for button presses
         int counter = 300;
         while (counter > 0){
            if (buttonA.uniquePress()) { switch_mode();  return; }
            if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
          delay(1);
          counter--;
          }
      setBright();          
      get_time();
      secflag=false;
      get_shiftTime(secflag);

      shiftChar[0] = dig[5];
      shiftChar[1] = dig[4];
      shiftChar[2] = ':';
      shiftChar[3] = dig[3];
      shiftChar[4] = dig[2];
      shiftChar[5] = ':';
      shiftChar[6] = dig[1];
      shiftChar[7] = dig[0];

       // when in circle mode and minute=odd and second is between 4 and 15, switch to small mode (mode 1)
      if(circle){
        if(rtc[1] % 2 == 1 && (rtc[0] >= 4 && rtc[0] <=15 )){
          wipeMiddle();
          display_temp();
          wipeTop();
          clock_mode =1;  // switch to small mode
          return;
        }
      }
//________________________________________________
// shift chars from right to left inside
//int x;
//int y=1;
 nr=0;
 w=0;

  do{
      for(x=34; x>=w; x--){      
        puttinychar(x+w, y, shiftChar[nr]);
        for (byte yy = 0 ; yy < 7; yy ++) {
              plot(x+w + 3, yy, 0);
        }
      }
      w=w+2;
      nr++;
    } while(w<=28);   
//________________________________________________

   setBright();
   get_time(); 
      
       // when in circle mode and minute=odd and second is between 4 and 15, switch to small mode (mode 1)
      if(circle){
        if(rtc[1] % 2 == 1 && (rtc[0] >= 4 && rtc[0] <=15 )){
          wipeMiddle();
          display_temp();
          wipeTop();
          clock_mode =1;  // switch to small mode
          return;
        }
      }

  // get 5x new chars to display, when there is no shifting
  for (int i=0; i<5; i++) {
      setBright();
      secflag=true;  // -4 seconds, because the shifting of previous chars was approx. 4 seconds in the past
      get_shiftTime(secflag);     
      shiftChar[0] = dig[5];
      shiftChar[1] = dig[4];
      shiftChar[2] = ':';
      shiftChar[3] = dig[3];
      shiftChar[4] = dig[2];
      shiftChar[5] = ':';
      shiftChar[6] = dig[1];
      shiftChar[7] = dig[0];

      puttinychar( 0, 1, shiftChar[0]);
      puttinychar( 4, 1, shiftChar[1]);
      puttinychar(12, 1, shiftChar[3]);
      puttinychar(16, 1, shiftChar[4]);
      puttinychar(24, 1, shiftChar[6]);
      puttinychar(28, 1, shiftChar[7]);

         //hold display but check for button presses
         counter = 860;  // while in for-loop, stop 860ms before displaying next char
         while (counter > 0){
            if (buttonA.uniquePress()) { switch_mode();  return; }
            if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
          delay(1);
          counter--;
          }
   }      
//________________________________________________
// shift chars from left to left outside:
nr=0;
w=-28;
  do{
      for(x=28; x>=w-3; x--){      
        puttinychar(x+w, y, shiftChar[nr]);
        for (byte yy = 0 ; yy < 7; yy ++) {
              plot(x+w + 3, yy, 0);
        }
      }
      w=w+4;
      nr++;
    } while(w<=0);   
//________________________________________________

         //hold display but check for button presses
         counter = 150;
         while (counter > 0){
            if (buttonA.uniquePress()) { switch_mode();  return; }
            if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
          delay(1);
          counter--;
          }
     }  // end of while run-mode
}
// end of shift-mode
////////////////////////////////////////////////////////////////////////////////////////

// Create time-chars for shift-mode
void get_shiftTime(bool secflag){
  
      get_time();
      if (secflag){
        if(rtc[0]>=4){       
        rtc[0] = rtc[0] -4; // -4 seconds, because the shifting of previous chars was approx. 4 seconds in the past
        }
      }
      
  
      //split all time into individual digits - stick in dig array
      char buffer_hours[3];
      itoa( rtc[2], buffer_hours, 10);
      char buffer_mins[3];
      itoa( rtc[1], buffer_mins, 10);
      char buffer_secs[3];
      itoa( rtc[0], buffer_secs, 10);


      if (rtc[0] < 10) {
        buffer_secs[1] = buffer_secs[0];
        buffer_secs[0] = '0';
      }
      dig[0] = buffer_secs[1];      //0 - secs ones
      dig[1] = buffer_secs[0];      //1 - secs tens

      if (rtc[1] < 10) {
        buffer_mins[1] = buffer_mins[0];
        buffer_mins[0] = '0';
      }
      dig[2] = buffer_mins[1];      //2 - mins ones
      dig[3] = buffer_mins[0];      //3 - mins tens

      if (rtc[2] < 10) {
        buffer_hours[1] = buffer_hours[0];
        buffer_hours[0] = '0';
      }
      dig[4] = buffer_hours[1];     //4 - hour ones
      dig[5] = buffer_hours[0];     //5 - hour tens

      // the string we want to shift:
      //char shiftChar[8] = { dig[5], dig[4], ':', dig[3], dig[2], ':', dig[1], dig[0] };

    
} // end of get_shiftTime

////////////////////////////////////////////////////////////////////////////////////////


//display tempwerature
void display_temp(){

  measure_Temp(); //get the temp-values from the DS18B20-sensor
    
  char tempC[6];

  tempC[0]=tempi[0];
  tempC[1]=tempi[1];
  tempC[2]=tempi[2];
  tempC[3]=tempi[3];
//  tempC[4]= '~'; // degree-symbol + C
  tempC[4]= '`'; //degree 
  tempC[5]= 'C';

  //set intensity for all 4 devices to 0 (for fading up later)
  for (byte address = 0; address < 4; address++) {
    lc.setIntensity(address, 0);
  }

  int date_delay = 70;  // delay between displaying next character
  
  // print temp
/*
  byte offset = 6;      
  puttinychar(0+offset, 1, tempC[0]);   //print the 1st temp number (10 degrees)
  delay(date_delay);
  puttinychar(4+offset, 1, tempC[1]);   //print the 2nd temp number (1 degrees)
  delay(date_delay);
  puttinychar(8+offset, 1, tempC[2]);   //print the 3rd temp number (.)
  delay(date_delay);
  puttinychar(10+offset, 1, tempC[3]);  //print the 4th temp number (first decimal place)
  delay(date_delay);
  puttinychar(14+offset, 1, tempC[4]);  //print degree-symbol
  delay(date_delay);
  puttinychar(18+offset, 1, tempC[5]);  //print the 6th temp number C(elsius)
*/
 byte offset = 1;      
  putnormalchar(0+offset, 0, tempC[0]);   //print the 1st temp number (10 degrees)
  delay(date_delay);
  putnormalchar(6+offset, 0, tempC[1]);   //print the 2nd temp number (1 degrees)
  delay(date_delay);
  putnormalchar(12+offset, 0, tempC[2]);   //print the 3rd temp number (.)
  delay(date_delay);
  putnormalchar(14+offset, 0, tempC[3]);  //print the 4th temp number (first decimal place)
  delay(date_delay);
  putnormalchar(20+offset, 0, tempC[4]);  //print degree-symbol
  delay(date_delay);
  putnormalchar(26+offset, 0, tempC[5]);  //print the 6th temp number C(elsius)
  fade_high();
  delay(2500);  
  setBright();  // set brightness depending of value read from LDR

}

////////////////////////////////////////////////////////////////////////////////////////

//display humidity
void display_hum(){

  measure_Hum(); //get the temp-values from the DS18B20-sensor
    
  char hum[5];

  hum[0]=humi[0];
  hum[1]=humi[1];
  hum[2]= '%';
  hum[3]= 'R';
  hum[4]= 'H';

  //set intensity for all 4 devices to 0 (for fading up later)
  for (byte address = 0; address < 4; address++) {
    lc.setIntensity(address, 0);
  }

  int date_delay = 70;  // delay between displaying next character
  
  // print humidity
  /*
  byte offset = 6;      
  puttinychar(0+offset, 1, hum[0]);   //print the 1st temp number (10 degrees)
  delay(date_delay);
  puttinychar(4+offset, 1, hum[1]);   //print the 2nd temp number (1 degrees)
  delay(date_delay);
  puttinychar(8+offset, 1, hum[2]);   //print the %
  delay(date_delay);
  puttinychar(12+offset, 1, hum[3]);  //print the 6th temp number R
  delay(date_delay);
  puttinychar(16+offset, 1, hum[4]);  //print the 6th temp number H
  */
  byte offset = 1;      
  putnormalchar(0+offset, 0, hum[0]);   //print the 1st temp number (10 degrees)
  delay(date_delay);
  putnormalchar(6+offset, 0, hum[1]);   //print the 2nd temp number (1 degrees)
  delay(date_delay);
  putnormalchar(12+offset, 0, hum[2]);   //print the 3rd temp number (.)
  delay(date_delay);
  putnormalchar(19+offset, 0, hum[3]);  //print the 4th temp number (first decimal place)
  delay(date_delay);
  putnormalchar(25+offset, 0, hum[4]);  //print the 4th temp number (first decimal place)
  delay(date_delay);
  fade_high();
  delay(2500);  
  setBright();  // set brightness depending of value read from LDR
}

////////////////////////////////////////////////////////////////////////////////////////

//display date - show dayname, date, month, year, week of year in 4 steps
void display_date(){
  int date_delay = 70;  // delay between displaying next character
    
  wipeBottom();     //wipe out devices  

  //read the date from the DS1307/DS3231
  byte dow = rtc[3]; // day of week 0 = Sunday
  byte date = rtc[4];
  byte month = rtc[5] - 1;
  byte year = rtc[6]-2000;

  //array of month names to print on the display. Some are shortened as we only have 8 characters across to play with
  //  char monthnames[12][9] = {
  //    "Ianuarie", "Februarie", "Martie", "Aprilie", "Mai", "Iunie", "Iulie", "August", "Septembrie", "Octombrie", "Noiembrie", "Decembrie"
  //  };

  char monthnames[12][4] = {
    "Ian", "Feb", "Mar", "Apr", "Mai", "Iun", "Iul", "Aug", "Sep", "Oct", "Noi", "Dec"
  };

  //----------- print the day name ----------- //
  //get length of text in pixels, that way we can centre it on the display by divindin the remaining pixels b2 and using that as an offset
  byte len = 0;
  while(daysfull[dow][len]) { 
    len++; 
  }; 
  byte offset = (31 - ((len-1)*4)) / 2; //our offset to centre up the text
   
  int i = 0;
  while(daysfull[dow][i]){
    puttinychar((i*4) + offset , 1, daysfull[dow][i]);
    delay(date_delay); 
    i++;
  }
        //hold display but check for button presses
        int counter = 1000;
        while (counter > 0){
        if (buttonA.uniquePress()) { switch_mode();  return; }
        if (buttonB.uniquePress()) { toggleDateState();  return; }
        delay(1);
        counter--;
        }
  cls();


  
  //----------- print date numerals ----------- //
  char buffer[3];
  //if date < 10 add a 0
  itoa(date,buffer,10);
     if (date < 10) {
       buffer[1] = buffer[0];
       buffer[0] = '0';
      }
  offset = 5;      
  puttinychar(0+offset, 1, buffer[0]);  //print the 1st date number
  delay(date_delay);
  puttinychar(4+offset, 1, buffer[1]);  //print the 2nd date number
  delay(date_delay);
  puttinychar(8+offset, 1, suffix[0]);  //print suffix -  char suffix[1]={'.'}; is defined at top of code
  delay(90);

  
  //----------- print month name ----------- // 
  //get length of text in pixels, that way we can centre it on the display by divindin the remaining pixels b2 and using that as an offset
  len = 0;
  while(monthnames[month][len]) { 
    len++; 
  }; 
  //offset = (31 - ((len-1)*4)) / 2; //our offset to centre up the text
  offset = 17;
  i = 0;
  while(monthnames[month][i]){
    puttinychar((i*4) +offset, 1, monthnames[month][i]);
    delay(date_delay); 
    i++; 
  }
        //hold display but check for button presses
        counter = 1000;
        while (counter > 0){
        if (buttonA.uniquePress()) { switch_mode();  return; }
        if (buttonB.uniquePress()) { toggleDateState();  return; }
        delay(1);
        counter--;
        }
  cls();


  //----------- print year ----------- //
  offset = 9; //offset to centre text - e.g. 2016
  char buffer_y[3] = "20";
  puttinychar(0+offset , 1, buffer_y[0]);   //print the 1st year number: 2
  delay(date_delay);
  puttinychar(4+offset , 1, buffer_y[1]);   //print the 2nd year number: 0 
  delay(date_delay);
  itoa(year,buffer,10);                     //if year < 10 add a 0
   if (year < 10) {
       buffer[1] = buffer[0];
       buffer[0] = '0';
      }
  puttinychar(8+offset, 1, buffer[0]);      //print the 1st year number
  delay(date_delay);
  puttinychar(12+offset, 1, buffer[1]);     //print the 2nd year number
  delay(1000);
  cls();

  //----------- print week of year ----------- //
  offset = 1;
  char buffer_w[6] = "Sapt.";
  puttinychar(0+offset , 1, buffer_w[0]);   //print "W"
  delay(date_delay);
  puttinychar(4+offset , 1, buffer_w[1]);   //print "o"
  delay(date_delay);
  puttinychar(8+offset , 1, buffer_w[2]);   //print "c"
  delay(date_delay);
  puttinychar(12+offset , 1, buffer_w[3]);   //print "h"
  delay(date_delay);
  puttinychar(16+offset , 1, buffer_w[4]);   //print "e"
  delay(date_delay);
  itoa(WN,buffer,10);                        //if week < 10 add a 0
   if (WN < 10) {
       buffer[1] = buffer[0];
       buffer[0] = '0';
      }
  puttinychar(23+offset, 1, buffer[0]);      //print the 1st week number
  delay(date_delay);
  puttinychar(27+offset, 1, buffer[1]);      //print the 2nd week number
        //hold display but check for button presses
        counter = 1000;
        while (counter > 0){
        if (buttonA.uniquePress()) { switch_mode();  return; }
        if (buttonB.uniquePress()) { toggleDateState();  return; }
        delay(1);
        counter--;
        }
  wipeTop();  //wipe out devices

} // end of display_date

////////////////////////////////////////////////////////////////////////////////////////

// toggleDateState: toggle Show date : On/Off
void toggleDateState(){ 
            if (show_date == true ) {
                show_date = false;
                date_state = true;
                    if(debug){
                       Serial.println("Show date = On");
                        }
                //cls();
                wipeTop();
                //display state of date
                char dateOn[8] = "DATE:ON";
                int len=7;  // length of dateOn               
                byte offset_top = (31 - ((len - 1) * 4)) / 2; 
                byte i = 0;
                while (dateOn[i]) {
                 puttinychar((i * 4) + offset_top, 1, dateOn[i]);
                 i++;
                }
                    //hold display but check for button presses
                    int counter = 1000;
                    while (counter > 0){
                          if (buttonA.uniquePress()) { switch_mode();  return; }
                          if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
                          delay(1);
                          counter--;
                    }
               wipeBottom();     
              }
              
            else{
              show_date = true;
              date_state = false;
                  if(debug){
                    Serial.println("Show date = Off");
                    }
                //cls();
                wipeTop();
                //display state of date
                char dateOff[9] = "DATE:OFF";
                int len=8;  // length of dateOn               
                byte offset_top = (31 - ((len - 1) * 4)) / 2; 
                byte i = 0;
                while (dateOff[i]) {
                 puttinychar((i * 4) + offset_top, 1, dateOff[i]);
                 i++;
                }
                    //hold display but check for button presses
                    int counter = 1000;
                    while (counter > 0){
                          if (buttonA.uniquePress()) { switch_mode();  return; }
                          if (buttonB.uniquePress()) { toggleDateState();  delay(1000); return; }
                          delay(1);
                          counter--;
                    }
               wipeBottom();               
            }
}

////////////////////////////////////////////////////////////////////////////////////////

//display menu to change the clock-mode
void switch_mode() {

  //remember mode we are in. We use this value if we go into settings mode, so we can change back from settings mode (6) to whatever mode we were in.
  old_mode = clock_mode;

  const char *modes[] = {    
    "Basic", "Small", "Big S", "Sml S", "Words", "Shift", "Setup",
  };

  byte next_clock_mode;
  byte firstrun = 1;

  //loop waiting for button (timeout after 35 loops to return to mode X)
  for (int count = 0; count < 35 ; count++) {

    //if user hits button, change the clock_mode
    if (buttonA.uniquePress() || firstrun == 1) {

      count = 0;
      cls();

      if (firstrun == 0) {
        clock_mode++;
      }
      if (clock_mode > NUM_DISPLAY_MODES + 1 ) {
        clock_mode = 0;
      }

      //print arrown and current clock_mode name on line one and print next clock_mode name on line two
      char str_top[9];

      //strcpy (str_top, "-");
      strcpy (str_top, modes[clock_mode]);

      next_clock_mode = clock_mode + 1;
      if (next_clock_mode >  NUM_DISPLAY_MODES + 1 ) {
        next_clock_mode = 0;
      }

      byte i = 0;
      while (str_top[i]) {
        putnormalchar(i * 6, 0, str_top[i]);
        i++;
      }
      firstrun = 0;
    }
    delay(50);
  }
}

////////////////////////////////////////////////////////////////////////////////////////

//run clock main loop as long as run_mode returns true
byte run_mode() {
  setBright();  // 
  return 1;
}

////////////////////////////////////////////////////////////////////////////////////////


// setup menu(=mode6): display menu to change the clock settings
void setup_menu() {

  //char* set_modes[] = { //depecated
  const char *set_modes[] = {
     "Circl", "=24Hr","Set >", "Exit"};   
  if (ampm == 0) { 
    set_modes[1] = ("=12Hr"); 
  }

  byte setting_mode = 0;
  byte next_setting_mode;
  byte firstrun = 1;

  //loop waiting for button (timeout after 35 loops to return to mode X)
  for(int count=0; count < 35 ; count++) {
    //if user hits button, change the clock_mode
    if(buttonA.uniquePress() || firstrun == 1){
      count = 0;
      cls();

      if (firstrun == 0) { 
        setting_mode++; 
      } 
      if (setting_mode > NUM_SETTINGS_MODES) { 
        setting_mode = 0; 
      }

      //print arrown and current clock_mode name on line one and print next clock_mode name on line two
      char str_top[9];
    
      strcpy (str_top, set_modes[setting_mode]);

      next_setting_mode = setting_mode + 1;
      if (next_setting_mode > NUM_SETTINGS_MODES) { 
        next_setting_mode = 0; 
      }
      
      byte i = 0;
      while(str_top[i]) {
        putnormalchar(i*6, 0, str_top[i]); 
        i++;
      }

      firstrun = 0;
    }
    delay(50); 
  }
  
  //pick the mode 
  switch(setting_mode){
    case 0: 
      set_circle(); 
      break;
    case 1: 
       set_ampm(); 
      break;
    case 2: 
      set_time(); 
      break;
      case 3: 
      //exit form menu
      break;
  }
    
  //change the mode from mode 6 (=settings) back to the one it was in before 
  clock_mode=old_mode;
}

////////////////////////////////////////////////////////////////////////////////////////

//toggle circle mode: change clock-mode every 2 minutes? On/Off
void set_circle(){
  cls();

  char text_a[9] = "=Off";
  char text_b[9] = "=On";
  byte i = 0;

  //if circle mode is on, turn it off
  if (circle){

    //turn circle mode off
    circle = 0;

    //print a message on the display
    while(text_a[i]) {
      putnormalchar((i*6), 0, text_a[i]);
      i++;
    }
  } else {
    //turn circlee mode on. 
    circle = 1;
      
    //print a message on the display
    while(text_b[i]) {
      putnormalchar((i*6), 0, text_b[i]);
      i++;
    }  
  } 
  delay(1200); //leave the message up for a second or so
}

////////////////////////////////////////////////////////////////////////////////////////

//ampm: set 12 or 24 hour clock
void set_ampm() {
  // AM/PM or 24 hour clock mode - flip the bit (makes 0 into 1, or 1 into 0 for ampm mode)
  ampm = (ampm ^ 1);
  cls();
}

////////////////////////////////////////////////////////////////////////////////////////

//set_time: set time and date
void set_time() {
  cls();

  //fill settings with current clock values read from clock
  get_time();
  byte set_min   = rtc[1];
  byte set_hr    = rtc[2];
  byte set_date  = rtc[4];
  byte set_mnth  = rtc[5];
  int  set_yr    = rtc[6];


  //Set function - we pass in: which 'set' message to show at top, current value, reset value, and rollover limit.
  set_date = set_value(2, set_date, 1, 31);
  set_mnth = set_value(3, set_mnth, 1, 12);
  set_yr   = set_value(4, set_yr, 2013, 2099);
  set_hr   = set_value(1, set_hr, 0, 23);
  set_min  = set_value(0, set_min, 0, 59);

  ds1307.adjust(DateTime(set_yr, set_mnth, set_date, set_hr, set_min));
  
  cls();
}


//used to set min, hr, date, month, year values. pass 
//message = which 'set' message to print, 
//current value = current value of property we are setting
//reset_value = what to reset value to if to rolls over. E.g. mins roll from 60 to 0, months from 12 to 1
//rollover limit = when value rolls over
int set_value(byte message, int current_value, int reset_value, int rollover_limit){

  cls();
  //char messages[6][17]   = {
  char messages[6][9]   = {
  //"Set Mins", "Set Hour", "Set Day", "Set Mnth", "Set Year"};
    "Minute >", "Ora >", "Ziua   >", "Luna  >", "An   >"};

  //Print "set xyz" top line
  byte i = 0;
  while(messages[message][i])
  {
    puttinychar(i*4 , 1, messages[message][i]); 
    i++;
  }

  delay(999);
  cls();

  //print digits bottom line
  char buffer[5] = "    ";
  itoa(current_value,buffer,10);
  puttinychar(0 , 1, buffer[0]); 
  puttinychar(4 , 1, buffer[1]); 
  puttinychar(8 , 1, buffer[2]); 
  puttinychar(12, 1, buffer[3]); 

  delay(300);
  //wait for button input
  while (!buttonA.uniquePress()) {

    while (buttonB.isPressed()){

      if(current_value < rollover_limit) { 
        current_value++;
      } 
      else {
        current_value = reset_value;
      }
      //print the new value
      itoa(current_value, buffer ,10);
      puttinychar(0 , 1, buffer[0]); 
      puttinychar(4 , 1, buffer[1]); 
      puttinychar(8 , 1, buffer[2]); 
      puttinychar(12, 1, buffer[3]);    
      delay(150);
    }
  }
  return current_value;
}

////////////////////////////////////////////////////////////////////////////////////////

// get_time: get the current time from the RTC
void get_time()
{
  //get time
  DateTime now = ds1307.now();
  //save time to array
  rtc[6] = now.year();
  rtc[5] = now.month();
  rtc[4] = now.day();
//  rtc[3] = now.dayOfWeek(); //returns 0-6 where 0 = Sunday (new RTC)
  rtc[3] = now.dayOfTheWeek(); //returns 0-6 where 0 = Sunday (old RTC)   
  //rtc[2] = now.hour();  depends on boolean summertime_EU
  rtc[1] = now.minute();
  rtc[0] = now.second();

 // Calculate if summer- or wintertime
  if(summertime_EU(now.year(), now.month(), now.day(), now.hour(), 1)){
    if(debug){Serial.print("Sommerzeit =  true"); }
    rtc[2] = now.hour() +1;   // wintertime: add 1 hour
      if(rtc[2] > 23){        // if hour > 23, we have next day between 00:00:00 and 01:00:00
        rtc[2] = 0;           // hour = 0
        rtc[3] = rtc[3] +1;   // dayOfWeek +1 
        rtc[4] = rtc[4] +1;   // day +1 
      }
  }
  else{
      if(debug){Serial.print("Sommerzeit = false"); }
      rtc[2] = now.hour();    // summertime = "normal" hour
  }


 // Calculate day of year and week of year 
 DayWeekNumber(rtc[6],rtc[5],rtc[4],rtc[3]);


  if(debug){
  //print the time to the serial port - for debuging
  Serial.print("     ");
  Serial.print(rtc[2]);
  Serial.print(":");
  Serial.print(rtc[1]);
  Serial.print(":");
  Serial.print(rtc[0]);

  Serial.print("   ");
  Serial.print(rtc[4]);
  Serial.print(".");
  Serial.print(rtc[5]);
  Serial.print(".");
  Serial.print(rtc[6]);

  Serial.print("    Wochentag: ");
  Serial.print(rtc[3]);    

  Serial.print("      Tag ");
  Serial.print(DN);
  Serial.print("  in Woche ");
  Serial.print(WN);
  Serial.print(" in ");
  Serial.print(rtc[6]);

  Serial.print("  clock_mode: ");
  Serial.println(clock_mode);
  }

}

////////////////////////////////////////////////////////////////////////////////////////

boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
 if (month<3 || month>10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
 if (month>3 && month<10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
 if (month==3 && (hour + 24 * day)>=(1 + tzHours + 24*(31 - (5 * year /4 + 4) % 7)) || month==10 && (hour + 24 * day)<(1 + tzHours + 24*(31 - (5 * year /4 + 1) % 7)))
   return true;
 else
   return false;
}

////////////////////////////////////////////////////////////////////////////////////////

//DayWeekNumber: Calculate day of year and week of year 
void DayWeekNumber(unsigned int y, unsigned int m, unsigned int d, unsigned int w){
 int days[]={0,31,59,90,120,151,181,212,243,273,304,334};    // Number of days at the beginning of the month in a not leap year.
//Start to calculate the number of day
 if (m==1 || m==2){
   DN = days[(m-1)]+d;                                      //for any type of year, it calculate the number of days for January or february
 }                                                          // Now, try to calculate for the other months
 else if ((y % 4 == 0 && y % 100 != 0) ||  y % 400 == 0){   //those are the conditions to have a leap year
   DN = days[(m-1)]+d+1;                                    // if leap year, calculate in the same way but increasing one day
 }
 else {                                                     //if not a leap year, calculate in the normal way, such as January or February
   DN = days[(m-1)]+d;
 }
// Now start to calculate Week number
 if (w==0){
   WN = (DN-7+10)/7;                                        //if it is sunday (time library returns 0)
 }
 else{
   WN = (DN-w+10)/7;                                        // for the other days of week
 }
}

////////////////////////////////////////////////////////////////////////////////////////

//Measure Temperature 
char measure_Temp(){

//  sensors.requestTemperaturesByAddress(tempDeviceAddress);  // sends command for one device to perform a temperature by address
 
 float TempC = dht.readTemperature();
//  float TempC = sensors.getTempC(tempDeviceAddress);   
  String stringTempC = "";           //data in buffer is copied to this string  
  dtostrf(TempC, 4, 1, tempi);       //4 is mininum width, 1 is precision; float value is copied to buffer
/*
float hum = dht.readHumidity();
 String stringHum = "";           //data in buffer is copied to this string  
  dtostrf(hum, 4, 1, tempi);       //4 is mininum width, 1 is precision; float value is copied to buffer
 */
/*
      if (debug){
        Serial.println("");
        Serial.print("Temperatur: ");
        Serial.println(sensors.getTempC(tempDeviceAddress)); // the first temp-sensor on I2C
        Serial.print("          tempi[0]: ");
        Serial.println(tempi[0]);
        Serial.print("          tempi[1]: ");
        Serial.println(tempi[1]);
        Serial.print("          tempi[2]: ");
        Serial.println(tempi[2]);
        Serial.print("          tempi[3]: ");
        Serial.println(tempi[3]);
        Serial.println("");        
      }
*/
   return  tempi[0], tempi[1], tempi[2], tempi[3] ;   
}
//  End of measure temperature

////////////////////////////////////////////////////////////////////////////////////////

//Measure humidity
char measure_Hum(){

//  sensors.requestTemperaturesByAddress(tempDeviceAddress);  // sends command for one device to perform a temperature by address
 
float hum = dht.readHumidity();
 String stringHum = "";           //data in buffer is copied to this string  
  dtostrf(hum, 4, 1, humi);       //4 is mininum width, 1 is precision; float value is copied to buffer
  
   return humi[0], humi[1], humi[2], humi[3] ;   
}
//  End of measure humidity

////////////////////////////////////////////////////////////////////////////////////////

// bottomleds: plot seconds-dots at bottomline
void bottomleds(byte secs){

      //switch on bottomleds from 1 to 30
      if(secs >=1 && secs <=30){                
        for(int i=0; i<=secs-1; i++){
             plot(i, 7, 1);
        }
      }
      
      //switch off bottomleds from 30 to 1   
      if(secs>=31){
        for(int i=0; i<=(30-(secs-30)); i++){
             plot(i, 7, 1);
        }
         plot(30-(secs-30), 7, 0);
       }


      //switch off bottomled 1      
      if(secs == 0){               
         plot(0, 7, 0);           
      }   
}

////////////////////////////////////////////////////////////////////////////////////////

//wipeRight: wipe-effect from right to left
void wipeRight(){

  //left to right
  for(int c=0; c<32; c++){
    for(int r=7; r>=0; r--){
        plot (c, r, 1);    
        }
     delay(15);
     for(int r=7; r>=0; r--){       
        plot (c, r, 0);    
     }
  }
} // end of wipeRight

////////////////////////////////////////////////////////////////////////////////////////

//wipeLeft: wipe-effect from left to right
void wipeLeft(){
  //right to left
  for(int c=32; c>=0; c--){
    for(int r=7; r>=0; r--){
        plot (c, r, 1);    
        }
     delay(15);
     for(int r=7; r>=0; r--){       
        plot (c, r, 0);    
     }
  }
} // end of wipeLeft

////////////////////////////////////////////////////////////////////////////////////////

//wipeTop: wipe-effect from top to bottom
void wipeTop(){
  for(int r=0; r<=8; r++){
    for(int c=0; c<32; c++){
        plot (c, r, 1);
        plot (c, r-1, 0);        
        }
  }
} // end of wipeTop


////////////////////////////////////////////////////////////////////////////////////////

//wipeBottom: wipe-effect from bottom to top
void wipeBottom(){
//bottom to top
  for(int r=7; r>=(-1); r--){
    for(int c=0; c<32; c++){
        plot (c, r, 1);
        plot (c, r+1, 0);        
        }
  }
} // end of wipeBottom


////////////////////////////////////////////////////////////////////////////////////////

//wipeMiddle: wipe-effect from left and right to the middle
void wipeMiddle(){
 for(int c=0; c<=31; c++){   
      for(int r=7; r>=0; r--){
          plot (c, r, 1);
          plot (32-c, r, 1);    
      }
 delay(10);
     
  for(int r=7; r>=0; r--){ 
      plot (c, r, 0);
            if(c != 16){
                plot (32-c, r, 0);
            }
            else{
                plot (c, 0, 0); delay(50);
                plot (c, 7, 0); delay(50);
                plot (c, 1, 0); delay(50);
                plot (c, 6, 0); delay(50);
                plot (c, 2, 0); delay(50);
                plot (c, 5, 0); delay(50);
                plot (c, 3, 0); delay(50);
                plot (c, 4, 0); delay(600);                 
                return;
            }
  }
 }
} // end of wipeMiddle

////////////////////////////////////////////////////////////////////////////////////////

//wipeOutside: wipe-effect from both sides over the middle to the other sides
void wipeOutside(){
  for(int c=0; c<32; c++){
    for(int r=7; r>=0; r--){
        plot (c, r, 1);
        plot (32-c, r, 1);    
        }
     delay(5);
     for(int r=7; r>=0; r--){      
        plot (c, r, 0);
          if(c != 16){
            plot (32-c, r, 0);
            }
     }
  }
delay(300);
} // end of wipeOutside

////////////////////////////////////////////////////////////////////////////////////////


// wipeInside - looks like random-clearing of dots 
// (for testing set all dots to 1)
void wipeInside(){

 int verz=5;  // delay between plotting each dot

 int rh=7;
 int rl=0;
 for(int row=0; row<4; row++){
      for(int col=0; col<8; col++){      
        plot(col,    rh, 0); delay(verz);
        plot(col,    rl, 0); delay(verz); 
        plot(31-col, rh, 0); delay(verz);
        plot(31-col, rl, 0); delay(verz);           
      }
     rh--;
     rl++;
 }

 rh=7;
 rl=0;
 for(int row=0; row<4; row++){        
      for(int col=0; col<8; col++){
        plot(8+col,  rh, 0); delay(verz);
        plot(8+col,  rl, 0); delay(verz);
        plot(23-col, rh, 0); delay(verz); 
        plot(23-col, rl, 0); delay(verz);
      }
      rh--;
      rl++;      
 }

 delay(300);
    
} // end of wipeInside

////////////////////////////////////////////////////////////////////////////////////////

/*
/// scroll: scroll text from right to left - not used at present - too slow.
void scroll() {

  char message[] = {"ABCDEFGH      "};

  cls();
  byte p = 6;      //current pos in string
  byte chara[] = {0, 1, 2, 3, 4, 5, 6, 7}; //chars from string
  int x[] = {0, 6, 12, 18, 24, 30, 36, 42}; //xpos for each char
  byte y = 0;                   //y pos

  // clear_buffer();

  while (message[p] != '\0') {

    //draw all 8 chars
    for (byte c = 0; c < 8; c++) {

      putnormalchar(x[c],y,message[ chara[c] ]);
      
      //draw a line of pixels turned off after each char,otherwise the gaps between the chars have pixels left in them from the previous char
      for (byte yy = 0 ; yy < 8; yy ++) {
        plot(x[c] + 5, yy, 0);
      }

      //take one off each chars position
      x[c] = x[c] - 1;
    }

    //reset a char if it's gone off screen
    for (byte i = 0; i <= 5; i++) {
      if (x[i] < -5 ) {
        x[i] = 31;
        chara[i] = p;
        p++;
      }
    }
  }
}

*/
////////////////////////////////////////////////////////////////////////////////////////
