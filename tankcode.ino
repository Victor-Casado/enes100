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

void debug(){
  Enes100.println("Connected: " + String(Enes100.isConnected()));
  Enes100.println("Visible: " + String(Enes100.isVisible()));
  Enes100.println("Y: " + String(Enes100.getY()));
  Enes100.println("X: " + String(Enes100.getX()));
  Enes100.println("Theta: " + String(Enes100.getTheta()));
  Enes100.println("Ultrasonic Reading: " + String(Tank.readDistanceSensor(2)));
}

void setup() {
  pinMode(PIN_US_FRONT_TRIG, OUTPUT);
  pinMode(PIN_US_FRONT_ECHO, INPUT);
//This function connects the tank to the vision system. The TX and RX pins of the tanks
//are 52 and 50 respectively.
Enes100.begin("FIRETEST", FIRE, 14, 1116, 52, 50);


//print some text in Serial Monitor
Enes100.println("Sucessfully connected to the Vision System");
}

void loop() {
  while (detectDistance(PIN_US_FRONT_TRIG, PIN_US_FRONT_ECHO) > 20){
    moveForward(255, 100);
  }
  delay(100);
}