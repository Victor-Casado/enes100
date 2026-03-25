#include "Arduino.h"
#include "Enes100.h"
#include "Tank.h"

#define BOT 0
#define MID 1
#define TOP 2
const float blockedVal = .4;
const float LANE_Y[3] = { 0.3, 0.9, 1.6 };  // BOT, MID, TOP

// --- Motion primitives ---

void moveForward(int speed, int time) {
  Enes100.println("moveForward called");
  Tank.setRightMotorPWM(speed);
  Tank.setLeftMotorPWM(speed);
  delay(time);
  Tank.turnOffMotors();
  Enes100.println("moveForward done");
}

void turnLeft(int speed, int time) {
  Enes100.println("turnLeft called");
  Tank.setRightMotorPWM(speed);
  Tank.setLeftMotorPWM(-speed);
  delay(time);
  Tank.turnOffMotors();
}

void turnRight(int speed, int time) {
  Enes100.println("turnRight called");
  Tank.setRightMotorPWM(-speed);
  Tank.setLeftMotorPWM(speed);
  delay(time);
  Tank.turnOffMotors();
}

// --- Orientation ---

void turnTo(float targetTheta) {
  Enes100.println("turnTo: waiting for angle");
  while (abs(Enes100.getTheta() - targetTheta) > 0.03) {

    float diff;
    bool foundAngle = false;
    while(!foundAngle){
      if(Enes100.isVisible()){
      diff = targetTheta - Enes100.getTheta();
      foundAngle = true;
      }
    }
    Enes100.println(diff);

    if (diff > 0) {
      if (diff > 1.0)       turnLeft(255, 200);
      else if (diff > 0.5)  turnLeft(255, 100);
      else                  turnLeft(200, 10);
    } else {
      diff = -diff;
      if (diff > 1.0)       turnRight(255, 200);
      else if (diff > 0.5)  turnRight(255, 100);
      else                  turnRight(200, 10);
    }
  }
  Enes100.println("turnTo: target reached");
}

// --- Lane helpers ---

int getLane() {
  Enes100.println("getLane: waiting for Y");
  float y;
  bool foundY = false;
  while(!foundY){
    if(Enes100.isVisible()){
      y = Enes100.getY();
      foundY = true;
    }
  }
  Enes100.println(y);

  if (y > 1.3) { Enes100.println("getLane: TOP"); return TOP; }
  if (y > 0.75) { Enes100.println("getLane: MID"); return MID; }
  Enes100.println("getLane: BOT");
  return BOT;
}

bool isBlocked(int row, int lane) {
  Enes100.println("isBlocked: reading sensor");
  Enes100.println(Tank.readDistanceSensor(1));
  bool blocked = (Tank.readDistanceSensor(1) < blockedVal && !(Tank.readDistanceSensor(1) == -1));
  if (blocked) Enes100.println("isBlocked: TRUE");
  else Enes100.println("isBlocked: FALSE");
  return blocked;
}

int getNextLane(int current) {
  Enes100.println("getNextLane called");
  if (current == TOP) return BOT;
  if (current == MID) return TOP;
  return MID;  // BOT -> MID
}

void navigateToLane(int target) {
  Enes100.println("navigateToLane: start");
  float targetY = LANE_Y[target];

  float y;
  bool foundY = false;
  while(!foundY){
    if(Enes100.isVisible()){
      y = Enes100.getY();
      foundY = true;
    }
  }
  Enes100.println(y);

  bool goingUp = y < targetY;
  if (goingUp) Enes100.println("navigateToLane: going UP");
  else Enes100.println("navigateToLane: going DOWN");

  float heading = goingUp ? PI / 2 : -PI / 2;
  while (goingUp ? y < targetY : y > targetY) {
    turnTo(heading);
    moveForward(255, 400);
    if(Enes100.isVisible()) y = Enes100.getY();
    Enes100.println(y);
  }
  Enes100.println("navigateToLane: target lane reached");
}

// --- Core row navigation ---

void navigateRow(int row) {
  Enes100.println("navigateRow: start");
  turnTo(0);
  delay(200);
  int current = getLane();
  Enes100.print("Row "); Enes100.print(row);
  Enes100.print(" | Lane: "); Enes100.println(current);

  if (!isBlocked(row, current)) {
    Enes100.println("Clear!");
  } else {
    int firstDodge = getNextLane(current);
    Enes100.print("Blocked! Trying lane: "); Enes100.println(firstDodge);
    navigateToLane(firstDodge);

    turnTo(0);
    delay(200);
    Enes100.print("Row "); Enes100.print(row);
    Enes100.print(" | Lane: "); Enes100.println(firstDodge);

    if (!isBlocked(row, firstDodge)) {
      Enes100.println("Clear!");
    } else {
      int secondDodge = 3 - current - firstDodge;
      Enes100.print("Blocked! Trying lane: "); Enes100.println(secondDodge);
      navigateToLane(secondDodge);

      turnTo(0);
      delay(200);
      Enes100.print("Row "); Enes100.print(row);
      Enes100.print(" | Lane: "); Enes100.println(secondDodge);
      Enes100.println("Clear!");
    }
  }

  float targetX = (row == 1) ? 1.5 : 2.8;
  Enes100.println("navigateRow: driving to targetX");
  Enes100.println(targetX);

  bool notVisible = true;
  while(notVisible){
    if(Enes100.isVisible()){
      while (Enes100.getX() < targetX) {
        Enes100.println(Enes100.getX());
        turnTo(0);
        moveForward(255, 400);
      }
      notVisible = false;
    }
  }
  Enes100.println("navigateRow: complete");
}

// --- Setup ---

void setup() {
  Enes100.begin("FIRETEST", FIRE, 18, 1116, 52, 50);
  Enes100.println("Connected to Vision System");
  Tank.begin();
  Enes100.println("Tank initialized");

  turnTo(0);
  Enes100.println("Initial turnTo(0) complete");

  while (Enes100.getX() < .6) {
    turnTo(0);
    moveForward(255, 400);
  }

  navigateRow(1);
  navigateRow(2);
  
  while(Enes100.getY() > 0.4){
    turnTo(-PI/2);
    moveForward(255, 400);
  }
  
  while (Enes100.getX() < 3.8) {
    turnTo(0);
    moveForward(255, 1000);
  }
}

void loop() { delay(100); }