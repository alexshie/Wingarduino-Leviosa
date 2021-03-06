/*
 Light Organ - Gets audio frequency response input from a MSGEQ7 chip and makes some LEDs flash to the music. 
 
 Author: Alex Shie
 Revision history:
   6/8/14 - created
 Breakaway volume at 15.7db
 */
 

// BOF preprocessor bug prevent - insert me on top of your arduino-code
// From: http://www.a-control.de/arduino-fehler/?lang=en
#if 1
__asm volatile ("nop");
#endif

#define DEBUG //Comment this out to disable serial debug printing
//#define LINEAR_SCALING

#define LED_RED_PIN 9
#define LED_GREEN_PIN 10
#define LED_BLUE_PIN 11

#define MSGEQ_RESET_PIN 6
#define MSGEQ_STROBE_PIN 5
#define MSGEQ_RESPONSE_PIN 0

#define MSGEQ_NUM_FREQ 7 //Number of frequency responses provided by the MSGEQ7
#define STROBE_PULSE_WIDTH 30 //Min 20us
#define OUTPUT_SET_TIME 42 //Min 36us
#define LED_REFRESH_DELAY 10 //Loop delay in ms
#define EXPECTED_MAX_INPUT 1023 //The value of the loudest possible frequency response from analog input, used when mapping inputs
#define NOISE_THRESHOLD 10
#define E 2.71828

uint8_t freqResponses[MSGEQ_NUM_FREQ]; //Holds cleaned up output from MSGEQ, 0 - 63Hz, 1 - 160Hz, 2 - 400Hz, 3 - 1kHz, 4 - 2.5kHz, 5 - 6.25kHz, 6 - 16kHz
uint16_t rawData[MSGEQ_NUM_FREQ];
uint8_t brightnessLookup[256];
uint16_t maxBass;
uint16_t minBass;
uint16_t maxMid;
uint16_t minMid;
uint16_t maxTreble;
uint16_t minTreble;
long lastSampleTime;

void setup() {                
  
  //Initialize all pins as output except for MSGEQ_RESPONSE_PIN, which is an analog input
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(MSGEQ_RESET_PIN, OUTPUT);
  pinMode(MSGEQ_STROBE_PIN, OUTPUT);
  //pinMode(MSGEQ_RESPONSE_PIN, INPUT);

  //Initialize all output pins to 0
  digitalWrite(LED_RED_PIN,LOW);
  digitalWrite(LED_GREEN_PIN,LOW);
  digitalWrite(LED_BLUE_PIN,LOW);
  digitalWrite(MSGEQ_RESET_PIN, LOW);
  digitalWrite(MSGEQ_STROBE_PIN, HIGH);
  maxBass=0;
  minBass=256;
  maxMid=0;
  minMid=256;
  maxTreble=0;
  minTreble=256;
  lastSampleTime=millis();
  
  #if defined(DEBUG)
  Serial.begin(115200);
  #endif
  
  //delay(2000);
  brightnessLookupInit();
}

void loop() {
  long curTime=millis();
  if(curTime-lastSampleTime > 10000) {
    maxBass=0;
    minBass=256;
    maxMid=0;
    minMid=256;
    maxTreble=0;
    minTreble=256;
    lastSampleTime=curTime;
  }
  getFreqResponses();
  
  #if defined(DEBUG)
  printFreqResponses();
  #endif
  
  updateLEDs(); 
  delay(LED_REFRESH_DELAY);
}

//Read inputs from the MSGEQ7 and process them to be used for PWM
void getFreqResponses() {
  int i;
  uint16_t input;
  uint16_t threshold;
  digitalWrite(MSGEQ_RESET_PIN,HIGH); 
  delayMicroseconds(STROBE_PULSE_WIDTH); //Min 100ns for reset, 18us for strobe
  //delay(1);
  digitalWrite(MSGEQ_RESET_PIN,LOW);
  //delayMicroseconds(100);
  
  for(i = 0; i < MSGEQ_NUM_FREQ; i++) {
    digitalWrite(MSGEQ_STROBE_PIN,LOW);  
    delayMicroseconds(OUTPUT_SET_TIME); //Min 36us output setting time
    input=analogRead(MSGEQ_RESPONSE_PIN);    
    rawData[i]=input;
    if(i < 2) {
      if(input > maxBass)
        maxBass=input;
      if(input < minBass)
        minBass=input;
      #if defined(LINEAR_SCALING)
      //input=constrain(map(input,350,EXPECTED_MAX_INPUT,0,255),0,255); //
      input=constrain(map(input,minBass,maxBass,0,255),0,255); //
      #else
      input=constrain(map(input,minBass,maxBass,0,255),0,255); //
      #endif
    } else if (i < 5) {
      if(input > maxMid)
        maxMid=input;
      if(input < minMid)
        minMid=input;
      #if defined(LINEAR_SCALING)
      //input=constrain(map(input,275,450,0,255),0,255); //
      input=constrain(map(input,minMid,maxMid,0,255),0,255); //
      #else
      input=constrain(map(input,minMid,maxMid-100,0,255),0,255); //
      #endif
    } else {
      if(input > maxTreble)
        maxTreble=input;
      if(input < minTreble)
        minTreble=input;
      #if defined(LINEAR_SCALING)
      //input=constrain(map(input,275,600,0,255),0,255); //
      input=constrain(map(input,minTreble,maxTreble,0,255),0,255); //
      #else
      input=constrain(map(input,minTreble,maxTreble,0,255),0,255); //
      #endif
    }
    #if defined(LINEAR_SCALING)
    threshold=100;
    #else
    threshold=40;
    input=brightnessLookup[input];
    #endif
    
    /*
    
    if(input < threshold) {
      freqResponses[i] = 0;
    } else {
      freqResponses[i]=input;  
    }*/
    if(maxBass<250 && maxMid < 250 && maxTreble < 250)
      freqResponses[i] = 0;
    else if((input < minBass+(maxBass-minBass)*0.05 && i < 2)||(input < minMid+(maxMid-minMid)*0.05 && i < 5)||(input < minTreble+(maxTreble-minTreble)*0.05 && i > 4)) {
      freqResponses[i] = 0;
    } else {
      freqResponses[i] = input;  
    }
    
    digitalWrite(MSGEQ_STROBE_PIN,HIGH);  
    //delayMicroseconds(STROBE_PULSE_WIDTH); //digitalWrite is probably slow enough that this isn't needed
   // delayMicroseconds(100);
  }
}

//Just prints some debug info (mapped and constrained values)
void printFreqResponses() {
  int i;
  Serial.print("Raw: ");
  for(i = 0; i < MSGEQ_NUM_FREQ; i++) {
    Serial.print(rawData[i]);
    Serial.print("\t");
  }
  Serial.println();  
  Serial.print("Processed: ");
  for(i = 0; i < MSGEQ_NUM_FREQ; i++) {
    Serial.print(freqResponses[i]);
    Serial.print("\t");
  }
  Serial.println();  
}

//Drive LEDs based on frequency response inputs. Change visual behavior here
void updateLEDs() {
  uint16_t bassPWM;
  uint16_t midPWM;
  uint16_t treblePWM;
  uint16_t average;
  int i;
  for(i = 0; i < MSGEQ_NUM_FREQ; i++) {
    average+=freqResponses[i];
  }
  average/=MSGEQ_NUM_FREQ;
  
  bassPWM = (freqResponses[0]+freqResponses[1])/2; //Average responses of 2 lowest frequency ranges
  //bassPWM = freqResponses[0];
//bassPWM = average;
  midPWM = (freqResponses[2]+freqResponses[3]+freqResponses[4])/3; //Average responses of 3 midrange frequency ranges
  //midPWM=freqResponses[3];
  treblePWM = (freqResponses[5]+freqResponses[6])/2; //Average responses of 2 highest frequency ranges
  
  analogWrite(LED_RED_PIN, bassPWM);
  analogWrite(LED_GREEN_PIN, midPWM);
  analogWrite(LED_BLUE_PIN, treblePWM);
  
  #if defined(DEBUG)
  Serial.print("Outputs (RGB): ");
  Serial.print(bassPWM);
  Serial.print("\t");
  Serial.print(midPWM);
  Serial.print("\t");
  Serial.print(treblePWM);
  Serial.println();
  #endif
}

void brightnessLookupInit() {
 int i;
 double val;
 double val2;
 for(i = 0; i < 256; i++) {
   val = (double)i;
   val2 = 1.0/(1+pow(E,((val/21)-6)*-1))*255;
   Serial.println((uint8_t)val2);
   brightnessLookup[i]=(uint8_t)val2;
 }
}

