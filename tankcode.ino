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
    float diff;
    bool foundAngle = false;
    while (!foundAngle) {
      if (Enes100.isVisible()) {
        diff = targetTheta - Enes100.getTheta();
        foundAngle = true;
      }
    }

    if (diff > 0) {
      if (diff > 1.0)      turnLeft(255, 200);
      else if (diff > 0.5) turnLeft(255, 100);
      else                 turnLeft(200, 10);
    } else {
      diff = -diff;
      if (diff > 1.0)      turnRight(255, 200);
      else if (diff > 0.5) turnRight(255, 100);
      else                 turnRight(200, 10);
    }
  }
}

// --- Lane helpers ---

int getLane() {
  float y;
  bool foundY = false;
  while (!foundY) {
    if (Enes100.isVisible()) {
      y = Enes100.getY();
      foundY = true;
    }
  }

  if (y > 1.3) return TOP;
  if (y > 0.75) return MID;
  return BOT;
}

bool isBlocked(int row, int lane) {
  float dist = Tank.readDistanceSensor(1);
  Enes100.print("Sensor: "); Enes100.println(dist);
  bool blocked = (dist < blockedVal && dist != -1);
  Enes100.println(blocked ? "isBlocked: TRUE" : "isBlocked: FALSE");
  return blocked;
}

int getNextLane(int current) {
  if (current == TOP) return BOT;
  if (current == MID) return TOP;
  return MID;
}

void navigateToLane(int target) {
  float targetY = LANE_Y[target];
  float y;
  bool foundY = false;
  while (!foundY) {
    if (Enes100.isVisible()) {
      y = Enes100.getY();
      foundY = true;
    }
  }

  Enes100.print("navigateToLane: "); Enes100.print(y);
  Enes100.print(" -> "); Enes100.println(targetY);

  bool goingUp = y < targetY;
  float heading = goingUp ? PI / 2 : -PI / 2;
  while (goingUp ? y < targetY : y > targetY) {
    turnTo(heading);
    moveForward(255, 400);
    if (Enes100.isVisible()) y = Enes100.getY();
  }
  Enes100.println("navigateToLane: done");
}

// --- Core row navigation ---

void navigateRow(int row) {
  Enes100.print("=== Row "); Enes100.print(row); Enes100.println(" ===");
  turnTo(0);
  delay(200);
  int current = getLane();
  Enes100.print("Lane: "); Enes100.println(current);

  if (!isBlocked(row, current)) {
    Enes100.println("Clear!");
  } else {
    int firstDodge = getNextLane(current);
    Enes100.print("Blocked! Trying: "); Enes100.println(firstDodge);
    navigateToLane(firstDodge);
    turnTo(0);
    delay(200);

    if (!isBlocked(row, firstDodge)) {
      Enes100.println("Clear!");
    } else {
      int secondDodge = 3 - current - firstDodge;
      Enes100.print("Blocked! Trying: "); Enes100.println(secondDodge);
      navigateToLane(secondDodge);
      turnTo(0);
      delay(200);
      Enes100.println("Clear!");
    }
  }

  float targetX = (row == 1) ? 1.5 : 2.8;
  Enes100.print("Driving to X="); Enes100.println(targetX);

  bool notVisible = true;
  while (notVisible) {
    if (Enes100.isVisible()) {
      while (Enes100.getX() < targetX) {
        turnTo(0);
        moveForward(255, 400);
      }
      notVisible = false;
    }
  }
  Enes100.print("Row "); Enes100.print(row); Enes100.println(" complete");
}

// --- Setup ---

void setup() {
  Enes100.begin("FIRETEST", FIRE, 18, 1116, 52, 50);
  Enes100.println("Connected");
  Tank.begin();

  turnTo(0);

  while (Enes100.getX() < .6) {
    turnTo(0);
    moveForward(255, 400);
  }

  navigateRow(1);
  navigateRow(2);

  while (Enes100.getY() > 0.4) {
    turnTo(-PI / 2);
    moveForward(255, 400);
  }

  while (Enes100.getX() < 3.8) {
    turnTo(0);
    moveForward(255, 1000);
  }
}

void loop() { delay(100); }