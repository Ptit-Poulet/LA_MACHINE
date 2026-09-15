#include <ESP32Servo.h>

// === PINS MECANUM ===
#define FL_PIN 13
#define FR_PIN 4 //initiallement 12
#define RL_PIN 14
#define RR_PIN 27

// === PINS ACTUATEUR ===
#define in1 16
#define in2 17

#define in3 23
#define in4 25


// === RC CHANNELS ===
#define RC_NUM_CHANNELS 6
#define RC_CH1 0  // Mecanum X
#define RC_CH2 1  // Mecanum rotation
#define RC_CH3 2  // Mecanum Y
#define RC_CH4 3  // Mecanum strafe
#define RC_CH5 4
#define RC_CH6 5  // Actuateur

#define RC_CH1_INPUT 18
#define RC_CH2_INPUT 21
#define RC_CH3_INPUT 22
#define RC_CH4_INPUT 19
#define RC_CH5_INPUT 26
#define RC_CH6_INPUT 33

// === Actuateur seuils ===
#define activateCH6_0 1600
#define activateCH6_2 1200

// === Variables ===
uint16_t RC_VALUES[RC_NUM_CHANNELS];
uint32_t RC_START[RC_NUM_CHANNELS];
volatile uint16_t RC_SHARED[RC_NUM_CHANNELS];

float store, store1, store2, store4, store5;

// === SERVOS MECANUM ===
Servo fl_servo, fr_servo, rl_servo, rr_servo;
#define SERVO_STOP_US 1500
#define SERVO_MAX_DELTA_US 400

// === CLASSE MECANUM ===
class MecanumChassis {
  private:
    Servo* servos[4] = {&fl_servo,&fr_servo,&rl_servo,&rr_servo};
    int pins[4] = {FL_PIN,FR_PIN,RL_PIN,RR_PIN};
    int stop_us = SERVO_STOP_US;
    int max_delta = SERVO_MAX_DELTA_US;
    const int invert[4] = {1,-1,1,-1};

  public:
    void begin() {
      ESP32PWM::allocateTimer(0);
      ESP32PWM::allocateTimer(1);
      ESP32PWM::allocateTimer(2);
      ESP32PWM::allocateTimer(3);
      for (int i=0;i<4;i++){
        servos[i]->setPeriodHertz(50);
        servos[i]->attach(pins[i],500,2500);
        servos[i]->writeMicroseconds(stop_us);
      }
    }
    void stop(){ for(int i=0;i<4;i++) servos[i]->writeMicroseconds(stop_us); }
    void setWheelSpeeds(int fl,int fr,int rl,int rr){
      int speeds[4]={fl,fr,rl,rr};
      for(int i=0;i<4;i++){
        int val = constrain(speeds[i]*invert[i],-255,255);
        int us = constrain(stop_us + (val*max_delta/255), stop_us-max_delta, stop_us+max_delta);
        servos[i]->writeMicroseconds(us);
      }
    }
    void move(int x,int y,int r){
      int fl=y+x+r; int fr=y-x-r; int rl=y-x+r; int rr=y+x-r;
      int m=max(max(abs(fl),abs(fr)),max(abs(rl),abs(rr)));
      if(m>255){ fl=fl*255/m; fr=fr*255/m; rl=rl*255/m; rr=rr*255/m; }
      setWheelSpeeds(fl,fr,rl,rr);
    }

    void forward(int speed){ move(0,-speed,0); }
    void backward(int speed){ move(0,speed,0); }
    void strafeLeft(int speed){ move(-speed,0,0); }
    void strafeRight(int speed){ move(speed,0,0); }
    void rotateCW(int speed){ move(0,0,-speed); }
    void rotateCCW(int speed){ move(0,0,speed); }
    void diagonalFrontLeft(int speed){ move(-speed,speed,0); }
    void diagonalFrontRight(int speed){ move(speed,speed,0); }
    void diagonalBackLeft(int speed){ move(-speed,-speed,0); }
    void diagonalBackRight(int speed){ move(speed,-speed,0); }
};

MecanumChassis chassis;

// === INTERRUPTS RC ===
void READ_RC(uint8_t ch, uint8_t pin){
  if(digitalRead(pin)) RC_START[ch]=micros();
  else RC_SHARED[ch]=(uint16_t)(micros()-RC_START[ch]);
}

void READ_RC1(){ READ_RC(RC_CH1,RC_CH1_INPUT); }
void READ_RC2(){ READ_RC(RC_CH2,RC_CH2_INPUT); }
void READ_RC3(){ READ_RC(RC_CH3,RC_CH3_INPUT); }
void READ_RC4(){ READ_RC(RC_CH4,RC_CH4_INPUT); }
void READ_RC5(){ READ_RC(RC_CH5,RC_CH5_INPUT); }
void READ_RC6(){ READ_RC(RC_CH6,RC_CH6_INPUT); }

void rc_read_values(){
  noInterrupts();
  memcpy(RC_VALUES, (const uint16_t*)RC_SHARED, sizeof(RC_SHARED));
  interrupts();
}

// === MOUVEMENT MECANUM ===
void mouvement(float store, float store1, float store2){
  if (store < 70 && store1 < 70){ chassis.diagonalFrontLeft(200); delay(10); }
  else if (store > 120 && store1 < 70){ chassis.diagonalBackLeft(200); delay(10); }
  else if (store < 70 && store1 > 120){ chassis.diagonalFrontRight(200); delay(10); }
  else if (store > 120 && store1 > 120){ chassis.diagonalBackRight(200); delay(10); }
  else if (store > 120){ chassis.forward(200); delay(10); }
  else if (store < 70){ chassis.backward(200); delay(10); }
  else if (store1 > 120){ chassis.strafeRight(200); delay(10); }
  else if (store1 < 70){ chassis.strafeLeft(200); delay(10); }
  else if (store2 > 120){ chassis.rotateCW(200); delay(10); }
  else if (store2 < 70){ chassis.rotateCCW(200); delay(10); }
  else { chassis.stop(); delay(10); }
}

// === SETUP ===
void setup() {
  Serial.begin(115200);
  // RC pins
  pinMode(RC_CH1_INPUT,INPUT); pinMode(RC_CH2_INPUT,INPUT);
  pinMode(RC_CH3_INPUT,INPUT); pinMode(RC_CH4_INPUT,INPUT);
  pinMode(RC_CH5_INPUT,INPUT);
  pinMode(RC_CH6_INPUT,INPUT);
  attachInterrupt(digitalPinToInterrupt(RC_CH1_INPUT),READ_RC1,CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH2_INPUT),READ_RC2,CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH3_INPUT),READ_RC3,CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH4_INPUT),READ_RC4,CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH5_INPUT),READ_RC5,CHANGE);
  attachInterrupt(digitalPinToInterrupt(RC_CH6_INPUT),READ_RC6,CHANGE);

  // Actuateur
  pinMode(in1,OUTPUT); pinMode(in2,OUTPUT);
  pinMode(in3,OUTPUT); pinMode(in4,OUTPUT);

  // Mecanum
  chassis.begin();
}

// === LOOP ===
void loop(){
  rc_read_values();

  // --- Mecanum ---
  store  = (RC_VALUES[RC_CH1])*0.225-250.2;
  store1 = (RC_VALUES[RC_CH4])*0.225-236.7;
  store2 = (RC_VALUES[RC_CH2])*0.225-236.7;
  mouvement(store, store1, store2);

  // --- ActuateurX ---
  store5 = RC_VALUES[RC_CH6];
  if (store5 > activateCH6_0 && store5 < 7000){
    digitalWrite(in1,HIGH); digitalWrite(in2,LOW); 
  } else if (store5 < activateCH6_2 && store5 > 200){
    digitalWrite(in1,LOW); digitalWrite(in2,HIGH); 
  } else {
    digitalWrite(in1,LOW); digitalWrite(in2,LOW);
  }

  // --- ActuateurY ---
  store4 = RC_VALUES[RC_CH5];
  if (store4 > activateCH6_0 && store4 < 7000){
    digitalWrite(in3,HIGH); digitalWrite(in4,LOW); 
  } else if (store4 < activateCH6_2 && store4 > 200){
    digitalWrite(in3,LOW); digitalWrite(in4,HIGH);
  } else {
    digitalWrite(in3,LOW); digitalWrite(in4,LOW);
  }
}
