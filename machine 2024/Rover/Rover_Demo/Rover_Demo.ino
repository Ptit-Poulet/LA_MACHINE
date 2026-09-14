

/*  Lynxmotion 4WD Rover par série
Utilisant Botboarduino et la drive Sabertooth

Réception de deux Integer se suivant par UART, le premier servant de vitesse avant/reculon
le second de direction à tourner

Lecture du LIDAR et envoie de la distance par UART
arrêt en cas de détection de collision imminente
*/

#include <Servo.h>
#include <Wire.h>
#include <LIDARLite.h>

//********************//
//*Objects definition*//
//********************//

//Configuration Drive
  Servo FWD;        //Sabertooth drive opere comme deux servos RC
  Servo Turn;
  Servo Head;       //Orientation Lidar

//Configuration LIDAR
  LIDARLite myLidar;

//********************
//Variable Definition
//********************

  int Yval;
  int Xval;
  char T;
  int i,k;



  int limitDist = 20;     
  int fwdIdle = 500;        //Valeur du zéro - Avant-Arrière
  int trnIdle = 507;        //Valeur du zéro - Gauche-Droite



//****************
//Initialisation
//****************
  
void setup() 
{
  Serial.begin(57600);
  myLidar.begin(0, true);  // Set configuration to default and I2C to 400 kHz
  myLidar.configure(0);

  Head.attach(7);         // Attaches servo objects to output pins
  FWD.attach(8);
  Turn.attach(9);


  Head.write(90);         //Centrer le Lidar
  Yval=fwdIdle;           //Immobiliser le Rover
  Xval=trnIdle;
}

void loop() {

//Faire un scan avec le LIAR
if(myLidar.distance(false)<limitDist){

  Yval=fwdIdle;            //Immobiliser le Rover
  Xval=trnIdle;
}

if (k==100){
  Serial.print(myLidar.distance(false));     //Lecture du LIDAR vers le terminal
  k=0;
}

//Augmenter et diminuer les valeurs de déplacements selon la réception série.
  i = 0;
if(Serial.available()>0){   
    T = Serial.read();
    switch (T){
      case 'w':
        if (Yval<1000){
          Yval=Yval+100;
        }
        break;
    
      case 's':
        if (Yval>1){
          Yval=Yval-100;
        }
        break;
        
      case 'd':
        if(Xval>1){
          Xval=Xval-100;
        }
        break;
        
      case 'a':
        if(Xval<1000){
          Xval=Xval+100;
        }
        break;
    
      case 'z':
        Yval=fwdIdle;
        Xval=trnIdle;
        break; 
    
      default:
      break;    
  }
}

//Mapper les valeurs obtenus par la table pour qu'elle soit utilisablent par les servos
//Et envoyer les positions à la drive
FWD.write(map(Yval, 1, 1000, 0, 179));
Turn.write(map(Xval, 1, 1000, 179, 0));
k++;
delay(1);
}
