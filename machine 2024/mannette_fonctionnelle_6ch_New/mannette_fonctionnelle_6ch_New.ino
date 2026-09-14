#include <ESP32Servo.h>

//Servo rotationZ;

#define RC_NUM_CHANNELS 6

#define RC_CH1  0 // Right Stick LR
#define RC_CH2  1 // Right Stick UD
#define RC_CH3  2 // Left  Stick UD
#define RC_CH4  3 // Left  Stick LR
#define RC_CH6  4 // Left  Stick UD
#define RC_CH8  5 // Left  Stick LR

#define RC_CH1_INPUT  14 // receiver pin 1
#define RC_CH2_INPUT  27 // receiver pin 2
#define RC_CH3_INPUT  26 // receiver pin 3
#define RC_CH4_INPUT  25 // receiver pin 4
#define RC_CH6_INPUT  33 // receiver pin 3
#define RC_CH8_INPUT  32 // receiver pin 4 //33,32 si on veut d'autre port 35,34 

// Set up some arrays to store our pulse starts and widths
uint16_t RC_VALUES[RC_NUM_CHANNELS];
uint32_t RC_START[RC_NUM_CHANNELS];
volatile uint16_t RC_SHARED[RC_NUM_CHANNELS];

const int IN1_PIN_BOOM = 2; // the Arduino pin connected to the IN1 pin L298N
const int IN2_PIN_BOOM = 0; // the Arduino pin connected to the IN2 pin L298N

const int IN1_PIN_STICK = 5; // Changer port
const int IN2_PIN_STICK = 15; //Changer port

const int IN3_PIN_BUCKET = 18; // Changer port
const int IN4_PIN_BUCKET = 19; // Changer port

const int UP_LIMIT = 1600;
const int LOW_LIMIT = 1400;

//int pastPosition = 0;

int moteurVisee;



// Setup our program
void setup() {
  // Set the speed to communicate with the host PC
  Serial.begin(115200); // Adjust baud rate as needed
 
  // Set our pin modes to input for the pins connected to the receiver
  pinMode(RC_CH1_INPUT, INPUT);
  pinMode(RC_CH2_INPUT, INPUT);
  pinMode(RC_CH3_INPUT, INPUT);
  pinMode(RC_CH4_INPUT, INPUT);
  pinMode(RC_CH6_INPUT, INPUT);
  pinMode(RC_CH8_INPUT, INPUT);

  pinMode(IN1_PIN_BOOM, OUTPUT);
  pinMode(IN2_PIN_BOOM, OUTPUT);

  pinMode(IN1_PIN_STICK, OUTPUT);
  pinMode(IN2_PIN_STICK, OUTPUT);

  pinMode(IN3_PIN_BUCKET, OUTPUT);
  pinMode(IN4_PIN_BUCKET, OUTPUT);

  // Attach interrupts to our pins
  attachInterrupt(digitalPinToInterrupt(RC_CH1_INPUT), READ_RC1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH2_INPUT), READ_RC2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH3_INPUT), READ_RC3, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH4_INPUT), READ_RC4, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH6_INPUT), READ_RC6, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH8_INPUT), READ_RC8, CHANGE);

  //rotationZ.attach(13);
  
}

void loop() {

  rc_read_values();
  
  // output our values to the serial port in a format the plotter can use
  Serial.print(  RC_VALUES[RC_CH1]);  Serial.print(",");
  Serial.print(  RC_VALUES[RC_CH2]);  Serial.print(",");
  Serial.print(  RC_VALUES[RC_CH3]);  Serial.print(",");
  Serial.print(  RC_VALUES[RC_CH4]);  Serial.print(",");
  Serial.print(  RC_VALUES[RC_CH6]);  Serial.print(",");
  Serial.println(RC_VALUES[RC_CH8]);

  whichpart();

  if (moteurVisee == 2) {
    if (RC_VALUES[RC_CH3] > UP_LIMIT) {
      digitalWrite(IN1_PIN_BOOM, HIGH);
      digitalWrite(IN2_PIN_BOOM, LOW);
    } 
    else if (RC_VALUES[RC_CH3] < LOW_LIMIT) {
      digitalWrite(IN1_PIN_BOOM, LOW);
      digitalWrite(IN2_PIN_BOOM, HIGH);
    }
    else if (RC_VALUES[RC_CH3] > LOW_LIMIT && RC_VALUES[RC_CH3] < UP_LIMIT){
      digitalWrite(IN1_PIN_BOOM, LOW);
      digitalWrite(IN2_PIN_BOOM, LOW);
    }
  } 
  
  if (moteurVisee == 0) {
    if (RC_VALUES[RC_CH3] > UP_LIMIT) {
      digitalWrite(IN1_PIN_STICK, HIGH);
      digitalWrite(IN2_PIN_STICK, LOW);
    } 
    else if (RC_VALUES[RC_CH3] < LOW_LIMIT) {
      digitalWrite(IN1_PIN_STICK, LOW);
      digitalWrite(IN2_PIN_STICK, HIGH);
    }
    else if (RC_VALUES[RC_CH3] > LOW_LIMIT && RC_VALUES[RC_CH3] < UP_LIMIT){
      digitalWrite(IN1_PIN_STICK, LOW);
      digitalWrite(IN2_PIN_STICK, LOW);
    }
  } 

  if(moteurVisee == 1) {
    if (RC_VALUES[RC_CH3] > UP_LIMIT) {
      digitalWrite(IN3_PIN_BUCKET, HIGH);
      digitalWrite(IN4_PIN_BUCKET, LOW);
    } 
    else if (RC_VALUES[RC_CH3] < LOW_LIMIT) {
      digitalWrite(IN3_PIN_BUCKET, LOW);
      digitalWrite(IN4_PIN_BUCKET, HIGH);
    }
     else if (RC_VALUES[RC_CH3] > LOW_LIMIT && RC_VALUES[RC_CH3] < UP_LIMIT){
      digitalWrite(IN3_PIN_BUCKET, LOW);
      digitalWrite(IN4_PIN_BUCKET, LOW);
    }
  }
}

// The functions are called by the interrupts. We send them all to the same place to measure the pulse width
void READ_RC1() { 
   Read_Input(RC_CH1, RC_CH1_INPUT); 
}
void READ_RC2() { 
   Read_Input(RC_CH2, RC_CH2_INPUT);
}
void READ_RC3() { 
   Read_Input(RC_CH3, RC_CH3_INPUT); 
}
void READ_RC4() { 
   Read_Input(RC_CH4, RC_CH4_INPUT); 
}
void READ_RC6() { 
   Read_Input(RC_CH6, RC_CH6_INPUT); 
}
void READ_RC8() { 
   Read_Input(RC_CH8, RC_CH8_INPUT); 
}

// This function reads the pulse starts and uses the time between rise and fall to set the value for pulse width
void Read_Input(uint8_t channel, uint8_t input_pin) {
  if (digitalRead(input_pin) == HIGH) {
    RC_START[channel] = micros();
  } else {
    uint16_t rc_compare = static_cast<uint16_t>(micros() - RC_START[channel]);
    RC_SHARED[channel] = rc_compare;
  }
}

// this function pulls the current values from our pulse arrays for us to use. 
void rc_read_values() {
  noInterrupts();
  memcpy(RC_VALUES, (const void *)RC_SHARED, sizeof(RC_SHARED));
  interrupts();
}

void whichpart(){
  if (RC_VALUES[RC_CH6] < LOW_LIMIT){
    moteurVisee = 0;
  }

  if (RC_VALUES[RC_CH6] > UP_LIMIT){
    moteurVisee = 1;
  }

  if (RC_VALUES[RC_CH6] > LOW_LIMIT && RC_VALUES[RC_CH6] < UP_LIMIT){
    moteurVisee = 2;
  }
}
