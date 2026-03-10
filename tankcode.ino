#include "Arduino.h"
#include "Enes100.h" //libraries are included at the very top of the program
#include "Tank.h"

#define PIN_US_FRONT_TRIG 18
#define PIN_US_FRONT_ECHO 19
#define PIN_US_REAR_TRIG 27
#define PIN_US_REAR_ECHO 26

float duration, distance;

int detectDistance(int trigPin, int echoPin){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (duration*.0343)/2;
  Enes100.print("Distance: ");
  Enes100.println(distance);
  return distance;
}

void moveForward(int speed, int time){
  Tank.setRightMotorPWM(speed);
  Tank.setLeftMotorPWM(speed);
  delay(time);
  Tank.turnOffMotors();
}

void turnLeft(int speed, int time){
  Tank.setRightMotorPWM(speed);
  Tank.setLeftMotorPWM(-speed);
  delay(time);
  Tank.turnOffMotors();
}

void turnRight(int speed, int time){
  Tank.setRightMotorPWM(-speed);
  Tank.setLeftMotorPWM(speed);
  delay(time);
  Tank.turnOffMotors();
}

void debug(){
  Enes100.println("Connected: " + String(Enes100.isConnected()));
  Enes100.println("Visible: " + String(Enes100.isVisible()));
  Enes100.println("Y: " + String(Enes100.getY()));
  Enes100.println("X: " + String(Enes100.getX()));
  Enes100.println("Theta: " + String(Enes100.getTheta()));
  Enes100.println("Ultrasonic Reading: " + String(Tank.readDistanceSensor(2)));
}

void turnTo(int direction){
  float targetTheta;

  if (direction == 0){
    targetTheta = PI/2;
  }
    if (direction == 1){
    targetTheta = 0;
  }
  if (direction == 2){
    targetTheta = -PI/2;
  }

    while (Enes100.getTheta() < targetTheta - .02){
  if (Enes100.getTheta() < targetTheta - 1){
    turnLeft(100, 20);
  }
  
    else if (Enes100.getTheta() < targetTheta - .3){
    turnLeft(50, 10);
  }
  
      else if (Enes100.getTheta() < targetTheta - .1){
    turnLeft(20, 10);
  }
  
    else if (Enes100.getTheta() < targetTheta - .02){
    turnLeft(10, 1);
  }}
  
      while (Enes100.getTheta() > targetTheta + .02){
  if (Enes100.getTheta() > targetTheta + 1){
    turnRight(100, 20);
  }
  
    else if (Enes100.getTheta() > targetTheta + .3){
    turnRight(50, 10);
  }
  
      else if (Enes100.getTheta() > targetTheta + .1){
    turnRight(20, 10);
  }
  
    else if (Enes100.getTheta() > targetTheta + .02){
    turnRight(10, 1);
  }}
  
  if ((Enes100.getTheta() < targetTheta - .02 || Enes100.getTheta() > targetTheta + .02)){
      turnTo(direction);
  }

}

void setup() {
  //pinMode(PIN_US_FRONT_TRIG, OUTPUT);
  //pinMode(PIN_US_FRONT_ECHO, INPUT);
  //This function connects the tank to the vision system. The TX and RX pins of the tanks
  //are 52 and 50 respectively.
  Enes100.begin("FIRETEST", FIRE, 16, 1116, 52, 50);
  Tank.begin();


  //print some text in Serial Monitor
  Enes100.println("Sucessfully connected to the Vision System");
  
    while(Enes100.getY() < 1.5){
            turnTo(0);

  moveForward(255, 50);}
  while(Enes100.getX() < 1.8){
        turnTo(1);

  moveForward(255, 50);}
  
 while(Enes100.getY() > 0.5){
    turnTo(2);
    moveForward(255, 50);
     
 }

while(Enes100.getX() < 3.8){
        turnTo(1);

    moveForward(255, 100);
}
}

void loop() {
  /*
  while (detectDistance(PIN_US_FRONT_TRIG, PIN_US_FRONT_ECHO) > 20){
    moveForward(255, 100);
  }
  delay(100);
  */


}