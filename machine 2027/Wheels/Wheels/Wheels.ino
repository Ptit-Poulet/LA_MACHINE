// test 4 roues avec manette
// TODO: à modifier pour la manette

//    The direction of the car's movement
//  IN1   IN2   IN3   IN4   Description  
//  HIGH  LOW   LOW   HIGH  Car is runing forward
//  LOW   HIGH  HIGH  LOW   Car is runing back
// Setup wheels in the front
//  FIN1   FIN2   FIN3   FIN4   
//  LOW  HIGH   LOW  HIGH   Car is running Left
//  HIGH  LOW   HIGH  LOW   Car is running Right
// Setup wheels in the Back
//  BIN1   BIN2   BIN3   BIN4
//  HIGH  LOW   HIGH  LOW   Car is running Left
//  LOW  HIGH   LOW  HIGH   Car is running Right
//  LOW   HIGH  LOW   HIGH  Car is turning left
//  HIGH  LOW   HIGH  LOW   Car is turning right
//  LOW   LOW   LOW   LOW   Car is stoped
//  HIGH  HIGH  HIGH  HIGH  Car is stoped
 
//define L298n module IO Pin
#define FIN1 7
#define FIN2 8
#define FIN3 9
#define FIN4 11

#define BIN1 7
#define BIN2 8
#define BIN3 9
#define BIN4 11

void forward(){ 
  //FrontWheels
  digitalWrite(FIN1,HIGH); //set FIN1 hight level
  digitalWrite(FIN2,LOW);  //set FIN2 low level
  digitalWrite(FIN3,LOW);  //set FIN3 low level
  digitalWrite(FIN4,HIGH); //set FIN4 hight level
  //BackWheels
  digitalWrite(BIN1,HIGH); 
  digitalWrite(BIN2,LOW);  
  digitalWrite(BIN3,LOW);  
  digitalWrite(BIN4,HIGH); 

  Serial.println("Forward");//send message to serial monitor
}

void back(){
  digitalWrite(FIN1,LOW);
  digitalWrite(FIN2,HIGH);
  digitalWrite(FIN3,HIGH);
  digitalWrite(FIN4,LOW);
  
  digitalWrite(BIN1,LOW);
  digitalWrite(BIN2,HIGH);
  digitalWrite(BIN3,HIGH);
  digitalWrite(BIN4,LOW);

  Serial.println("Back");
}

void left(){
  digitalWrite(FIN1,LOW);
  digitalWrite(FIN2,HIGH);
  digitalWrite(FIN3,LOW);
  digitalWrite(FIN4,HIGH);

  digitalWrite(BIN1,HIGH);
  digitalWrite(BIN2,LOW);
  digitalWrite(BIN3,HIGH);
  digitalWrite(BIN4,LOW);

  Serial.println("Left");
}

void right(){
  digitalWrite(FIN1,HIGH);
  digitalWrite(FIN2,LOW);
  digitalWrite(FIN3,HIGH);
  digitalWrite(FIN4,LOW);
  
  digitalWrite(BIN1,LOW);
  digitalWrite(BIN2,HIGH);
  digitalWrite(BIN3,LOW);
  digitalWrite(BIN4,HIGH);


  Serial.println("Right");
}

void turnLeft(){
  digitalWrite(FIN1,LOW);
  digitalWrite(FIN2,HIGH);
  digitalWrite(FIN3,LOW);
  digitalWrite(FIN4,HIGH); 
  
  digitalWrite(BIN1,LOW);
  digitalWrite(BIN2,HIGH);
  digitalWrite(BIN3,LOW);
  digitalWrite(BIN4,HIGH); 

  Serial.println("Left");
}

void turnRight(){
  digitalWrite(FIN1,HIGH);
  digitalWrite(FIN2,LOW);
  digitalWrite(FIN3,HIGH);
  digitalWrite(FIN4,LOW);
  
  digitalWrite(BIN1,HIGH);
  digitalWrite(BIN2,LOW);
  digitalWrite(BIN3,HIGH);
  digitalWrite(BIN4,LOW);
  
  Serial.println("Right");
}

//before execute loop() function, 
//setup() function will execute first and only execute once
void setup() {
  Serial.begin(9600);//open serial and set the baudrate
  pinMode(FIN1,OUTPUT);//before useing io pin, pin mode must be set first 
  pinMode(FIN2,OUTPUT);
  pinMode(FIN3,OUTPUT);
  pinMode(FIN4,OUTPUT);
  
  pinMode(BIN1,OUTPUT);
  pinMode(BIN2,OUTPUT);
  pinMode(BIN3,OUTPUT);
  pinMode(BIN4,OUTPUT);
}

//Repeat execution
void loop() {
  forward();      //go forward
  delay(1000);    //delay 1000 ms
  back();         //go back
  delay(1000);
  left();         // go left
  delay(1000);
  right();        // go right
  delay(1000);
  turnLeft();     //turning left
  delay(1000);
  turnRight();    //turning right
  delay(1000);
}
