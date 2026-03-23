#include "Arduino.h"
#include "Enes100.h"
#include "Tank.h"

#define BOT 0
#define MID 1
#define TOP 2
const float blockedVal = .8;
const float LANE_Y[3] = { 0.5, 1.0, 1.6 };  // BOT, MID, TOP

// --- Motion primitives ---

void moveForward(int speed, int time) {
  Tank.setRightMotorPWM(speed);
  Tank.setLeftMotorPWM(speed);
  delay(time);
  Tank.turnOffMotors();
}

void turnLeft(int speed, int time) {
  Tank.setRightMotorPWM(speed);
  Tank.setLeftMotorPWM(-speed);
  delay(time);
  Tank.turnOffMotors();
}

void turnRight(int speed, int time) {
  Tank.setRightMotorPWM(-speed);
  Tank.setLeftMotorPWM(speed);
  delay(time);
  Tank.turnOffMotors();
}

// --- Orientation ---

void turnTo(float targetTheta) {
  while (abs(Enes100.getTheta() - targetTheta) > 0.03) {
    float diff = targetTheta - Enes100.getTheta();
    if (diff > 0) {
      if (diff > 1.0)      turnLeft(255, 70);
      else if (diff > 0.5) turnLeft(200, 20);
      else if (diff > 0.15) turnLeft(80, 5);
      else                 turnLeft(20, 1);
    } else {
      diff = -diff;
      if (diff > 1.0)      turnRight(255, 70);
      else if (diff > 0.5) turnRight(200, 20);
      else if (diff > 0.15) turnRight(80, 5);
      else                 turnRight(20, 1);
    }
  }
}

// --- Lane helpers ---

int getLane() {
  float y = Enes100.getY();
  if (y > 1.3) return TOP;
  if (y > 0.75) return MID;
  return BOT;
}

bool isBlocked(int row, int lane) { 
  Enes100.println(Tank.readDistanceSensor(1));
  return (Tank.readDistanceSensor(1) < blockedVal && !(Tank.readDistanceSensor(1) == -1));
}

int getNextLane(int current) {
  if (current == TOP) return BOT;
  if (current == MID) return TOP;
  return MID;  // BOT -> MID
}

void navigateToLane(int target) {
  float targetY = LANE_Y[target];
  bool goingUp = Enes100.getY() < targetY;
  float heading = goingUp ? PI / 2 : -PI / 2;
  while (goingUp ? Enes100.getY() < targetY : Enes100.getY() > targetY) {
    turnTo(heading);
    moveForward(255, 50);
  }
}

// --- Core row navigation ---

void navigateRow(int row) {
  // Face obstacle and check current lane
  turnTo(0);
  delay(200);
  int current = getLane();
  Enes100.print("Row "); Enes100.print(row);
  Enes100.print(" | Lane: "); Enes100.println(current);

  if (!isBlocked(row, current)) {
    Enes100.println("Clear!");
  } else {
    // First dodge — up one lane (or BOT if at TOP)
    int firstDodge = getNextLane(current);
    Enes100.print("Blocked! Trying lane: "); Enes100.println(firstDodge);
    navigateToLane(firstDodge);

    // Re-face and re-check from new lane
    turnTo(0);
    delay(200);
    Enes100.print("Row "); Enes100.print(row);
    Enes100.print(" | Lane: "); Enes100.println(firstDodge);

    if (!isBlocked(row, firstDodge)) {
      Enes100.println("Clear!");
    } else {
      // Second dodge — third remaining lane
      int secondDodge = 3 - current - firstDodge;
      Enes100.print("Blocked! Trying lane: "); Enes100.println(secondDodge);
      navigateToLane(secondDodge);

      // Re-face and confirm
      turnTo(0);
      delay(200);
      Enes100.print("Row "); Enes100.print(row);
      Enes100.print(" | Lane: "); Enes100.println(secondDodge);
      Enes100.println("Clear!");
    }
  }

  // Drive forward past the row
  float targetX = (row == 1) ? 1.9 : 2.8;
  while (Enes100.getX() < targetX) {
    turnTo(0);
    moveForward(255, 50);
  }
}

// --- Setup ---

void setup() {
  Enes100.begin("FIRETEST", FIRE, 11, 1116, 52, 50);
  Enes100.println("Connected to Vision System");
  Tank.begin();

  while (Enes100.getX() < .9) {
    turnTo(0);
    moveForward(255, 100);
  }

  navigateRow(1);
  navigateRow(2);
  
  while(Enes100.getY() > 0.2){
    turnTo(-PI/2);
    moveForward(255, 100);
  }
  
  while (Enes100.getX() < 3.8) {
    turnTo(0);
    moveForward(255, 100);
  }
  
}

void loop() { delay(100);}