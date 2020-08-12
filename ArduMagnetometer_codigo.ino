#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <avr/wdt.h>

RTC_DS1307 rtc; 

/* FG sensor measurement sketch*/
char rxChar= 0; 

/* Define names for used pins */
int sensorX = 0;
int sensorY = 1;
int sensorZ = 7;

const int chipSelect = 10;

/* Define sensor update rate in ms */
unsigned int updateRate = 1000;

/* Define sensor measure time in ms */
unsigned int measureTime = 1000;

/* Define sensor frequency prescaler number */
unsigned int freqPrescaler = 4;

/* Define variables */
int state = 0;                            //Stores current state
volatile unsigned int intEnable = 0;      //0-counter for sensors is not incremented; 1-counter for sensors is incremented
volatile unsigned long sensorXCnt = 0;   //Stores number of counted changes for Lo sensor
volatile unsigned long sensorYCnt = 0;   //Stores number of counted changes for Up sensor
volatile unsigned long sensorZCnt = 0;   //Stores number of counted changes for Up sensor

unsigned long prevMillis = 0;             //Stores previous millis() for calculating refresh rate
unsigned long tiempo=0;

/* Setup loop */
void setup() {
  while (!Serial); // for Leonardo/Micro/Zero
   wdt_disable();

  /* Setup pins */
  Serial.begin(115200);
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  } // end rtc.begin()

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));  
  }    

  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  
  pinMode(sensorX, INPUT);
  pinMode(sensorY, INPUT);
  pinMode(sensorZ, INPUT);

  /* Setup interrupts */
  attachInterrupt(digitalPinToInterrupt(sensorX), sensorXHandler, CHANGE); 
  attachInterrupt(digitalPinToInterrupt(sensorY), sensorYHandler, CHANGE); 
  attachInterrupt(digitalPinToInterrupt(sensorZ), sensorZHandler, CHANGE); 

  /* Some delay to keep the initialization LCD text readable */
  delay(3000);

  wdt_enable (WDTO_8S);  // reset after 8 seconds, if no "pat the dog" received

}

uint32_t old_ts;

/* Main loop */
void loop() {
/* Measure current time */
  unsigned long currMillis = millis();
  
/* Calculate number of actual pulses per [ms] 
  Divide by 2 since we measure change in signal and by measureTime to get number of pulses per [ms]*/
  sensorXCnt = ((sensorXCnt * freqPrescaler)/2);
  sensorYCnt = ((sensorYCnt * freqPrescaler)/2);
  sensorZCnt = ((sensorZCnt * freqPrescaler)/2);
         
  /* Reset variables */  
  /* If defined sensor update rate has passed measure sensors */
  if(currMillis - prevMillis >= updateRate) {
    
    unsigned long long sensorx = sensorXCnt*1000/updateRate; //Counts per second
    unsigned long long sensory = sensorYCnt*1000/updateRate; 
    unsigned long long sensorz = sensorZCnt*1000/updateRate; 
    
    /* Reset variables */
    sensorXCnt = 0;
    sensorYCnt = 0;
    sensorZCnt = 0;
    
    /* Save current time */
    prevMillis = currMillis;

    /* Measure number of changes in signal for defined time */
    intEnable = 1;

    float SX; float SY; float SZ;

    SX = (unsigned long long) sensorx;
    SY = (unsigned long long) sensory;
    SZ = (unsigned long long) sensorz;

    DateTime now = rtc.now();

    char buf[] = "YYMMDD-hh:mm:ss";
    
    Serial.print(SX,0);Serial.print('\t');Serial.print(SY,0);Serial.print('\t');Serial.print(SZ,0);Serial.print('\t');Serial.println(now.toString(buf));

    char bufm[] = "MM";

    String Name(now.toString(bufm));
    Name += ".txt";
   
    File dataFile = SD.open(Name, FILE_WRITE);

    if (dataFile) {
      dataFile.print(SX,0);dataFile.print('\t');dataFile.print(SY,0);dataFile.print('\t');dataFile.print(SZ,0);dataFile.print('\t');dataFile.println(now.toString(buf));
      dataFile.close();
  }

 else {
    Serial.println("error opening datalog.txt");
  }
  
    wdt_reset();
    
    delay(measureTime);
    
    intEnable = 0;

   }
}

/* X sensor interrupt handler */
void sensorXHandler(){
  if (intEnable==1)
    sensorXCnt++;
}

/* Y sensor interrupt handler */
void sensorYHandler(){
  if (intEnable==1)
    sensorYCnt++;
}

/* Z sensor interrupt handler */
void sensorZHandler(){
  if (intEnable==1)
    sensorZCnt++;
}
