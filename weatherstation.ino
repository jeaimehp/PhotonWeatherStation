

/************************************
 * TACC Weather Station 
 * Designed for Particle Photon  
 * Maintained by: Je'aime Powell
 * Contact: jeaimehp@gmail.com / jpowell@tacc.utexas.edu
 * Initally Coded: 04/23/19
 ***********************************/

// This #include statement was automatically added by the Particle IDE.
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
#include <ArduinoJson.h>

// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_Si7021.h>

// Sparkfun Particle Photon OLED Shield
// This #include statement was automatically added by the Particle IDE.
#include <SparkFunMicroOLED.h>



/*
Micro-OLED-Shield-Example.ino
SparkFun Micro OLED Library Hello World Example
Jim Lindblom @ SparkFun Electronics
Original Creation Date: June 22, 2015
This sketch prints a friendly, recognizable logo on the OLED Shield, then
  goes on to demo the Micro OLED library's functionality drawing pixels,
  lines, shapes, and text.
  Hardware Connections:
  This sketch was written specifically for the Photon Micro OLED Shield, which does all the wiring for you. If you have a Micro OLED breakout, use the following hardware setup:
    MicroOLED ------------- Photon
      GND ------------------- GND
      VDD ------------------- 3.3V (VCC)
    D1/MOSI ----------------- A5 (don't change)
    D0/SCK ------------------ A3 (don't change)
      D2
      D/C ------------------- D6 (can be any digital pin)
      RST ------------------- D7 (can be any digital pin)
      CS  ------------------- A2 (can be any digital pin)
  Development environment specifics:
    IDE: Particle Build
    Hardware Platform: Particle Photon
                       SparkFun Photon Micro OLED Shield
  This code is beerware; if you see me (or any other SparkFun
  employee) at the local, and you've found our code helpful,
  please buy us a round!
  Distributed as-is; no warranty is given.
*/

//////////////////////////////////
// MicroOLED Object Declaration //
//////////////////////////////////
// Declare a MicroOLED object. If no parameters are supplied, default pins are
// used, which will work for the Photon Micro OLED Shield (RST=D7, DC=D6, CS=A2)

MicroOLED oled;

// Pin Definitions

#define WINDDIR A7 //WKP
#define WINDSPEED A2 //Attach Intrupt 
#define TIPPING D5 //Attach Intrupt 




// Tipping Bucket Variables
const unsigned int DEBOUNCE_TIME = 2000;
static unsigned long tip_last_interrupt_time = 0;
int tip = 0;

// Wind Speed (Anenometer) Variables
// Ref: https://circuitcrush.com/arduino/2015/08/10/measuring-wind-speed-with-arduino.html
// diameter of anemometer
float radius= 2.75; //inches from center pin to middle of cup
float diameter = radius * 2; //inches from center pin to middle of cup
float mph;
 
// read RPM
int half_revolutions = 0;
int rpm = 0;
unsigned long lastmillis = 0;
unsigned long lastmillis_pub = 0;
 
 
 // Wind Direction
 //Ref: https://forum.sparkfun.com/viewtopic.php?t=38671
 //Read the wind direction sensor, return heading in degrees
int get_wind_direction() 
{
  unsigned int adc;

  adc = analogRead(WINDDIR); // get the current reading from the sensor
/*
  if (adc < 414) return (90);
  if (adc < 456) return (158);
  if (adc < 508) return (135);
  if (adc < 551) return (203);
  if (adc < 615) return (180);
  if (adc < 680) return (23);
  if (adc < 746) return (45);
  if (adc < 801) return (248);
  if (adc < 833) return (225);
  if (adc < 878) return (338);
  if (adc < 913) return (0);
  if (adc < 940) return (293);
  if (adc < 967) return (315);
  if (adc < 990) return (270);
  return (-1); // error, disconnected?
*/
return adc;
}


// Initate si7021
Adafruit_Si7021 sensor = Adafruit_Si7021();



void setup() {
    Serial.begin(9600);
    oled.begin();    // Initialize the OLED
    oled.clear(ALL); // Clear the display's internal memory
    oled.display();  // Display what's in the buffer (splashscreen)
    delay(1000);     // Delay 1000 ms
    oled.clear(PAGE); // Clear the buffer.
    sensor.begin();
    printTitle("READY!", 1);
    Serial.println("READY!");
    
// Wind Speed (aneometer)
    pinMode(WINDSPEED, INPUT_PULLUP);
    attachInterrupt(WINDSPEED, rpm_fan, FALLING);
    
//Wind Direction
    pinMode(WINDDIR, INPUT);
    
// Tipping bucket
    pinMode(TIPPING, INPUT_PULLUP);
    attachInterrupt(TIPPING, tipcount, FALLING);
}

void loop() {
    if (millis() - lastmillis >= 1000)
    { //Update every one second, this will be equal to reading frequency (Hz).
      
      //JSON -- Note This code is using Arduino JSON v6
      // Old method for JSON v5 -- DynamicJsonBuffer jBuffer;
      //JsonObject& jsondata = jBuffer.createObject();
      const size_t capacity = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(16) + 30;
      DynamicJsonDocument jsondata(capacity);
      //JsonObject& jsondata = jsonBuffer.createObject();
      
      
      detachInterrupt(WINDSPEED);//Disable interrupt when calculating
      rpm = half_revolutions * 30; // Convert frequency to RPM, note: 60 works for one interruption per full rotation. For two interrupts per full rotation use half_revolutions * 30.
      Serial.print("RPM="); //print the word "RPM" and tab.
      Serial.print(rpm); // print the rpm value.
      Serial.print(" Hz="); //print the word "Hz".
      Serial.print(half_revolutions/2); //print revolutions per second or Hz. And print new line or enter. divide by 2 if 2 interrupts per revolution
      half_revolutions = 0; // Restart the RPM counter
      lastmillis = millis(); // Update lastmillis
      attachInterrupt(WINDSPEED, rpm_fan, FALLING); //enable interrupt
      mph = diameter * 3.14 * rpm * 60 / 63360;
      //mph = mph * 3.5; // calibration factor for anemometer accuracy, adjust as necessary
      Serial.print(" MPH="); //print the word "MPH".
      Serial.println(mph);
      
      jsondata["rpm"] = rpm;
      jsondata["hz"] = half_revolutions/2;
      jsondata["mph"] = mph;
      int windir = get_wind_direction(); 
      Serial.print("Wind Direction=");
      Serial.println(windir);
      jsondata["winddir"] = windir;
      Serial.print("Rain Tips=");
      Serial.println(tip);
      jsondata["raintip"] = tip;
      float humidity = sensor.readHumidity();
      float temperature = sensor.readTemperature();
      Serial.print("Humidity:    "); Serial.print(humidity, 2);
      Serial.print("\tTemperature: "); Serial.println(temperature, 2);
      jsondata["humidity"] = humidity;
      jsondata["temerature"] = temperature;
      
      
       // Particle publish conditional 
       if (millis() - lastmillis_pub >= 10000) {
        String data;
        serializeJson(jsondata, data);
        Particle.publish("data", data);

        // Refresh OLED
        oled.clear(PAGE);
        oled.setFontType(1);  // Set font to type 0
        oled.setCursor(0, 0); // Set cursor to top-left
        oled.print("T=");// Print "A0"
        oled.print(temperature);
        oled.setCursor(0, 16);
        oled.print("WS=");
        oled.print(mph);
        oled.setCursor(0, 32);
        oled.print("Rain=");
        oled.print(tip);
        oled.display();
        
        
        lastmillis_pub = millis();
        
       }
      
    }
    
}

void printTitle(String title, int font)
{
  int middleX = oled.getLCDWidth() / 2;
  int middleY = oled.getLCDHeight() / 2;

  oled.clear(PAGE);
  oled.setFontType(font);
  // Try to set the cursor in the middle of the screen
  oled.setCursor(middleX - (oled.getFontWidth() * (title.length()/2)),
                 middleY - (oled.getFontWidth() / 2));
  // Print the title:
  oled.print(title);
  oled.display();
  delay(1500);
  oled.clear(PAGE);
}   

// this code will be executed every time the interrupt for windspeed
void rpm_fan(){
    half_revolutions++;
  }

// this code will be executed every time the interrupt for the tipping bucket
void tipcount(){
    unsigned long interrupt_time = micros();
    if (interrupt_time - tip_last_interrupt_time > DEBOUNCE_TIME) {
    tip++;
    }
    tip_last_interrupt_time = interrupt_time; 
}
  
 

