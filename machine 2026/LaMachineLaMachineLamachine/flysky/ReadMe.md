# Ce projet utilise une radio Spektrum D18e et un ESP32 pour contrôler :
- des roues (servos) avec direction différentielle,
- deux actionneurs linéaires DC,
- un servo à rotation continue avec fins de course.

- L’ESP32 lit les signaux PWM du récepteur RC et pilote les moteurs en conséquence.

## Commandes (canaux RC)

CH2 : avancer / reculer

CH4 : gauche / droite

CH1 : actionneur linéaire 1

CH3 : actionneur linéaire 2

CH9 : switch 3 positions pour le servo continu (gauche / stop / droite)

## Pins ESP32

*Entrées RC :* 

CH1 → GPIO32   
CH2 → GPIO35  
CH3 → GPIO39  
CH4 → GPIO34  
CH9 → GPIO4

Actionneur 1 : GPIO16 / GPIO17

Actionneur 2 : GPIO18 / GPIO19

Servos roues : GPIO25, 33, 27, 26

Servo continu : GPIO23

Fins de course : GPIO14 (min), GPIO13 (max)

_En cas de perte du signal radio, tous les moteurs et servos s’arrêtent automatiquement._
