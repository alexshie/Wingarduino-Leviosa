/*
 Light Organ
 
 
 Author: Alex Shie
 Revision history:
   6/18/14 - created
 
 */
 

// BOF preprocessor bug prevent - insert me on top of your arduino-code
// From: http://www.a-control.de/arduino-fehler/?lang=en
#if 1
__asm volatile ("nop");
#endif

#define DEBUG

#define LED_RED_PIN          9
#define LED_GREEN_PIN        10
#define LED_BLUE_PIN         11

#define MSGEQ_RESET_PIN      8
#define MSGEQ_STROBE_PIN     7
#define MSGEQ_RESPONSE_PIN   A0

#define MSGEQ_NUM_FREQ       7 //Number of frequency responses provided by the MSGEQ7
#define STROBE_PULSE_WIDTH   20 //Min 20us
#define OUTPUT_SET_TIME      50 //Min 36us
#define LED_REFRESH_DELAY    100 //Loop delay in ms

uint16_t freqResponses[MSGEQ_NUM_FREQ]; //Holds output from MSGEQ, 0 - 63Hz, 1 - 160Hz, 2 - 400Hz, 3 - 1kHz, 4 - 2.5kHz, 5 - 6.25kHz, 6 - 16kHz


void setup() {                
  
  //Initialize all pins as output except for MSGEQ_RESPONSE_PIN, which is an analog input
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(MSGEQ_RESET_PIN, OUTPUT);
  pinMode(MSGEQ_STROBE_PIN, OUTPUT);

  //Initialize all output pins to 0
  digitalWrite(LED_RED_PIN,LOW);
  digitalWrite(LED_GREEN_PIN,LOW);
  digitalWrite(LED_BLUE_PIN,LOW);
  digitalWrite(MSGEQ_RESET_PIN, LOW);
  digitalWrite(MSGEQ_STROBE_PIN, LOW);
  
  #if defined(DEBUG)
  Serial.begin(115200);
  #endif
}

void loop() {
  getFreqResponses();
  #if defined(DEBUG)
  printFreqResponses();
  #endif
  updateLEDs(); 
  delay(LED_REFRESH_DELAY);              // wait for a second
}

void getFreqResponses() {
  int i;
  digitalWrite(MSGEQ_RESET_PIN,HIGH); 
  digitalWrite(MSGEQ_STROBE_PIN,HIGH);
  delayMicroseconds(STROBE_PULSE_WIDTH); //Min 100ns for reset, 18us for strobe
  for(i = 0; i < MSGEQ_NUM_FREQ; i++) {
    digitalWrite(MSGEQ_STROBE_PIN,LOW);  
    delayMicroseconds(OUTPUT_SET_TIME); //Min 36us output setting time
    freqResponses[i]=analogRead(MSGEQ_RESPONSE_PIN);
    digitalWrite(MSGEQ_STROBE_PIN,HIGH);  
    delayMicroseconds(STROBE_PULSE_WIDTH);
  }
  digitalWrite(MSGEQ_RESET_PIN,LOW);
}

void printFreqResponses() {
  for(i = 0; i < MSGEQ_NUM_FREQ; i++) {
    Serial.print(freqResponses[i]);
    Serial.print("\t");
  }
  Serial.println("");  
}

void updateLEDs() {
  
}
