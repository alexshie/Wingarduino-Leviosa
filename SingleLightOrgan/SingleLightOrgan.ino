/*
 Light Organ - Gets audio frequency response input from a MSGEQ7 chip and makes some LEDs flash to the music. 
 
 Author: Alex Shie
 Revision history:
   6/8/14 - created
 
 */
 

// BOF preprocessor bug prevent - insert me on top of your arduino-code
// From: http://www.a-control.de/arduino-fehler/?lang=en
#if 1
__asm volatile ("nop");
#endif

#define DEBUG //Comment this out to disable serial debug printing

#define LED_RED_PIN          9
#define LED_GREEN_PIN        10
#define LED_BLUE_PIN         11

#define MSGEQ_RESET_PIN      8
#define MSGEQ_STROBE_PIN     7
#define MSGEQ_RESPONSE_PIN   A0

#define MSGEQ_NUM_FREQ       7 //Number of frequency responses provided by the MSGEQ7
#define STROBE_PULSE_WIDTH   30 //Min 20us
#define OUTPUT_SET_TIME      42 //Min 36us
#define LED_REFRESH_DELAY    30 //Loop delay in ms
#define EXPECTED_MAX_INPUT   200 //The value of the loudest possible frequency response from analog input, used when mapping inputs
#define NOISE_THRESHOLD      10

uint8_t freqResponses[MSGEQ_NUM_FREQ]; //Holds cleaned up output from MSGEQ, 0 - 63Hz, 1 - 160Hz, 2 - 400Hz, 3 - 1kHz, 4 - 2.5kHz, 5 - 6.25kHz, 6 - 16kHz
uint16_t rawData[MSGEQ_NUM_FREQ];

void setup() {                
  
  //Initialize all pins as output except for MSGEQ_RESPONSE_PIN, which is an analog input
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(MSGEQ_RESET_PIN, OUTPUT);
  pinMode(MSGEQ_STROBE_PIN, OUTPUT);
  pinMode(MSGEQ_RESPONSE_PIN, INPUT);

  //Initialize all output pins to 0
  digitalWrite(LED_RED_PIN,LOW);
  digitalWrite(LED_GREEN_PIN,LOW);
  digitalWrite(LED_BLUE_PIN,LOW);
  digitalWrite(MSGEQ_RESET_PIN, LOW);
  digitalWrite(MSGEQ_STROBE_PIN, HIGH);
  
  #if defined(DEBUG)
  Serial.begin(115200);
  #endif
}

void loop() {
  uint16_t input;
  input = analogRead(MSGEQ_RESPONSE_PIN);
  #if defined(DEBUG)
  Serial.print("Raw: ");
  Serial.println(input);
  #endif  
  input=constrain(map(input,0,EXPECTED_MAX_INPUT,0,255),0,255);
  #if defined(DEBUG)
  Serial.print("Output: ");
  Serial.println(input);
  #endif
  if (input > 0)
    analogWrite(LED_RED_PIN, input);
  //getFreqResponses();
  

  
  //updateLEDs(); 
  delay(LED_REFRESH_DELAY);
}

//Read inputs from the MSGEQ7 and process them to be used for PWM
void getFreqResponses() {
  int i;
  uint16_t input;
  digitalWrite(MSGEQ_RESET_PIN,HIGH); 
  //delayMicroseconds(STROBE_PULSE_WIDTH); //Min 100ns for reset, 18us for strobe
  digitalWrite(MSGEQ_RESET_PIN,LOW);
  delayMicroseconds(100);
  
  for(i = 0; i < MSGEQ_NUM_FREQ; i++) {
    digitalWrite(MSGEQ_STROBE_PIN,LOW);  
    delayMicroseconds(OUTPUT_SET_TIME); //Min 36us output setting time
    input=analogRead(MSGEQ_RESPONSE_PIN);    
    input=constrain(map(input,0,EXPECTED_MAX_INPUT,0,255),0,255); //
    rawData[i]=input;
    if(input < NOISE_THRESHOLD) {
      freqResponses[i] = 0;
    } else {
      freqResponses[i]=input;  
    }
    digitalWrite(MSGEQ_STROBE_PIN,HIGH);  
    //delayMicroseconds(STROBE_PULSE_WIDTH); //digitalWrite is probably slow enough that this isn't needed
    delayMicroseconds(100);
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
