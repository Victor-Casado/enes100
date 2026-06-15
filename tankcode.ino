#include "Arduino.h"
#include "Enes100.h"

// ─── Pin Configuration ───────────────────────────────────────────────────────
const int enA1  = 2,  enB1  = 3;
const int in1_1 = 39, in2_1 = 37, in3_1 = 35, in4_1 = 33;
const int enA2  = 4,  enB2  = 5;
const int in1_2 = 38, in2_2 = 36, in3_2 = 34, in4_2 = 32;

const int trigLeft  = 50, echoLeft  = 51;
const int trigRight = 52, echoRight = 53;
const int trigPin1  = 44, echoPin1  = 43;
const int trigPin2  = 48, echoPin2  = 49;
const int irPins[2] = {22, 23};

// ─── Constants ───────────────────────────────────────────────────────────────
#define BOT 0
#define MID 1
#define TOP 2

const float LANE_Y[3]     = { 0.3, 1, 1.6 };
const float blockedVal    = 40.0;
const float CENTER_TOL    = 10.0;
const int   NUDGE_SPEED   = 255;
const int   NUDGE_TURN_MS = 800;
const int   NUDGE_FWD_MS  = 1000;
const int   DRIVE_SPEED   = 255;
const int   SHAKE_TIME_MS = 35;
const int   SHAKE_PAUSE_MS= 60;

// ─── Logging Helpers ─────────────────────────────────────────────────────────
const char* laneName(int l) { return l == TOP ? "TOP" : l == MID ? "MID" : "BOT"; }

// ─── Motor Primitives ────────────────────────────────────────────────────────
void setMotor(int enPin, int dirPin1, int dirPin2, int speed, bool forward) {
  analogWrite(enPin, speed);
  digitalWrite(dirPin1, forward ? HIGH : LOW);
  digitalWrite(dirPin2, forward ? LOW  : HIGH);
}

void stopMotors() {
  analogWrite(enA1, 0); digitalWrite(in1_1, LOW); digitalWrite(in2_1, LOW);
  analogWrite(enB1, 0); digitalWrite(in3_1, LOW); digitalWrite(in4_1, LOW);
  analogWrite(enA2, 0); digitalWrite(in1_2, LOW); digitalWrite(in2_2, LOW);
  analogWrite(enB2, 0); digitalWrite(in3_2, LOW); digitalWrite(in4_2, LOW);
}

void moveBackward(int speed, int timeMs) {
  setMotor(enA1, in1_1, in2_1, speed, false);
  setMotor(enB1, in3_1, in4_1, speed, false);
  setMotor(enA2, in1_2, in2_2, speed * .8, false);
  setMotor(enB2, in3_2, in4_2, speed, false);
  delay(timeMs);
  stopMotors();
}

void moveForward(int speed, int duration) {
  setMotor(enA1, in1_1, in2_1, speed, true);
  setMotor(enB1, in3_1, in4_1, speed, true);
  setMotor(enA2, in1_2, in2_2, speed * 0.8, true);
  setMotor(enB2, in3_2, in4_2, speed, true);
  delay(duration);
  stopMotors();
}

void turnLeft(int speed, int duration) {
  setMotor(enA1, in1_1, in2_1, speed, false);
  setMotor(enB1, in3_1, in4_1, speed, false);
  setMotor(enA2, in1_2, in2_2, speed * 1, true);
  setMotor(enB2, in3_2, in4_2, speed, true);
  delay(duration);
  stopMotors();
}

void turnRight(int speed, int duration) {
  setMotor(enA1, in1_1, in2_1, speed, true);
  setMotor(enB1, in3_1, in4_1, speed, true);
  setMotor(enA2, in1_2, in2_2, speed * 1, false);
  setMotor(enB2, in3_2, in4_2, speed, false);
  delay(duration);
  stopMotors();
}

void turnTo(float target) {
    Enes100.println("Turning to ");
  Enes100.print(target);
  bool aligned = false;
  while (!aligned) {
    Enes100.println("Adjusting");
    while (!Enes100.isVisible()) {}
    delay(100);
    float diff = target - Enes100.getTheta();
    if (abs(diff) <= 0.04) {
      aligned = true;
    } else if (diff > 0) {
      if      (diff > 1.0) turnLeft(255, 400);
      else if (diff > 0.5) turnLeft(255, 200);
      else if (diff > 0.1) turnLeft(255, 150);
      else                 turnLeft(255, 90);
    } else {
      diff = -diff;
      if      (diff > 1.0) turnRight(255, 400);
      else if (diff > 0.5) turnRight(255, 200);
      else if (diff > 0.1) turnRight(255, 150);
      else                 turnRight(255, 90);
    }
  }
}

// ─── Sensors ─────────────────────────────────────────────────────────────────
float readDistance(int trig, int echo) {
  digitalWrite(trig, LOW);  delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long dur = pulseIn(echo, HIGH, 25000);
  return dur == 0 ? -1 : dur * 0.0343 / 2.0;
}

int medianDistance(int trig, int echo) {
  int r[5]; int count = 0;
  for (int i = 0; i < 5; i++) {
    float d = readDistance(trig, echo);
    if (d > 0) r[count++] = d;
    delay(10);
  }
  if (count == 0) return -1;
  for (int i = 0; i < count - 1; i++)
    for (int j = 0; j < count - 1 - i; j++)
      if (r[j] > r[j+1]) { int t = r[j]; r[j] = r[j+1]; r[j+1] = t; }
  return r[count / 2];
}

float readLeft()  { return medianDistance(trigLeft,  echoLeft);  }
float readRight() { return medianDistance(trigRight, echoRight); }

int   leftDist()  { return medianDistance(trigLeft, echoLeft);  }

void seekDist(int target, int trig, int echo) {
  int dist = medianDistance(trig, echo);
  while (abs(dist - target) > 1) {
    dist > target ? moveBackward(255, 100) : moveForward(255, 100);
    dist = medianDistance(trig, echo);
  }
}

// ─── Positioning & Drift ─────────────────────────────────────────────────────
float getX() { while(Enes100.getX() == -1){} return Enes100.getX();}
float getY() { while(Enes100.getY() == -1){} return Enes100.getY(); }

void nudge(bool goRight, float heading) {
  turnTo(heading);
  Enes100.print("NUDGE >> "); Enes100.println(goRight ? "Right" : "Left");
  goRight ? turnRight(255, NUDGE_TURN_MS) : turnLeft(255, NUDGE_TURN_MS);
  delay(1000);
  moveForward(NUDGE_SPEED, NUDGE_FWD_MS);
  Enes100.println("NUDGE >> Reacquiring heading");
  turnTo(heading);
  moveBackward(NUDGE_SPEED, NUDGE_FWD_MS - 200);
}

void correctDrift(float heading) {
  delay(1000);
  float targetX = .21;
  if (heading == PI / 2){targetX = .1;}
  if      (getX() >  targetX + .03) { 
    if (heading == PI / 2){nudge(false,  heading);}
    else {nudge(true, heading);} 
    correctDrift(heading); }
  else if (getX() < targetX - .03) {
    if (heading == PI / 2){nudge(true,  heading);}
    else {nudge(false, heading);}
    correctDrift(heading); }
  else Enes100.println("DRIFT CHECK | Centered, no correction needed");
}

// ─── Mission ─────────────────────────────────────────────────────────────────
void shakeOverFans(int cycles) {
  for (int i = 0; i < cycles; i++) {
    moveForward (DRIVE_SPEED, SHAKE_TIME_MS); delay(SHAKE_PAUSE_MS);
    moveBackward(DRIVE_SPEED, SHAKE_TIME_MS); delay(SHAKE_PAUSE_MS);
  }
}

int readIR() {
  int count = 0;
  for (int j = 0; j < 2; j++) if (digitalRead(irPins[j]) == LOW) count++;
  return count;
}

bool closeEnough(int measured, int target) { return abs(measured - target) <= 1; }

void printOrientation(int h1, int h2) {
  const char* ori =
    (closeEnough(h1,6) && closeEnough(h2,10)) ? "C" :
    (closeEnough(h1,7) && closeEnough(h2, 5)) ? "A" :
    (closeEnough(h1,5) && closeEnough(h2, 7)) ? "B" : "Unknown";
  Enes100.print("Orientation: "); Enes100.println(ori);
}


void doMission() {
  int amountOn = 1;

  seekDist(38.2+13.9, trigLeft, echoLeft);
  amountOn += readIR();
  Enes100.print("MISSION | IR read 1 done, fires so far="); Enes100.println(amountOn);

  seekDist(38.2+9.9, trigLeft, echoLeft);
  Enes100.println("MISSION | Shaking over fan 1");
  shakeOverFans(60);

  seekDist(38.2+4, trigLeft, echoLeft);
  amountOn += readIR();
  Enes100.print("MISSION | IR read 2 done, fires so far="); Enes100.println(amountOn);

  seekDist(37.2, trigLeft, echoLeft);
  Enes100.println("MISSION | Shaking over fan 2");
  shakeOverFans(60);

  seekDist(37.2-1.5, trigLeft, echoLeft);

  Enes100.print("RESULT | Fires detected: "); Enes100.println(amountOn);

  int v1 = medianDistance(trigPin1, echoPin1);
  int v2 = medianDistance(trigPin2, echoPin2);
  Enes100.print("RESULT | Sensor vals: h1="); Enes100.print(v1);
  Enes100.print(" h2="); Enes100.println(v2);
  printOrientation(v1, v2);

  Enes100.println("MISSION | Backing out of site");
  moveBackward(255, 5000);
}

// ─── Navigation ──────────────────────────────────────────────────────────────
void goToSite() {
  float y       = getY();
  bool  goingUp = y < 1.0;
  float heading = goingUp ?  PI / 2 : -PI / 2;
  float targetY = goingUp ? 1.45     :  0.55;

  Enes100.print("GO TO SITE | startY="); Enes100.print(y);
  Enes100.print(" targetY="); Enes100.println(targetY);
  turnTo(heading);
  Enes100.println("Correct Angle Reached");
  while (goingUp ? getY() < targetY : getY() > targetY) {
    correctDrift(heading);
    delay(1000);
    Enes100.print("GO TO SITE | Y="); Enes100.print(getY());
    Enes100.print(" -> target="); Enes100.println(targetY);
    moveForward(255, 200);
  }
  Enes100.println("GO TO SITE | Target Y reached");
}

int getLane() {
  float y = getY();
  int lane = y > 1.3 ? TOP : y > 0.75 ? MID : BOT;
  Enes100.print("LANE | Y="); Enes100.print(y);
  Enes100.print(" -> "); Enes100.println(laneName(lane));
  return lane;
}

bool isBlocked(int row, int lane) {
  float dist    = medianDistance(trigLeft, echoLeft);
  bool  blocked = dist < blockedVal && dist != -1;
  Enes100.print("OBSTACLE | Row="); Enes100.print(row);
  Enes100.print(" Lane="); Enes100.print(laneName(lane));
  Enes100.print(" dist="); Enes100.print(dist);
  Enes100.println(blocked ? " -> BLOCKED" : " -> CLEAR");
  return blocked;
}

int getNextLane(int current) {
  int next = current == TOP ? BOT : current == MID ? TOP : MID;
  Enes100.print("DODGE | "); Enes100.print(laneName(current));
  Enes100.print(" blocked -> trying "); Enes100.println(laneName(next));
  return next;
}

void navigateToLane(int target) {
  float y       = getY();
  float targetY = LANE_Y[target];
  bool  goingUp = y < targetY;
  float heading = goingUp ? PI / 2 : -PI / 2;

  Enes100.print("LANE NAV | Y="); Enes100.print(y);
  Enes100.print(" -> "); Enes100.print(laneName(target));
  Enes100.print(" (targetY="); Enes100.print(targetY);
  Enes100.println(goingUp ? ") dir=UP" : ") dir=DOWN");

  // Coarse approach
  int steps = 0;
  while (goingUp ? y < targetY - 0.05 : y > targetY + 0.05) {
    turnTo(heading);
    moveForward(255, 250);
    if (Enes100.isVisible()) y = Enes100.getY();
    steps++;
  }

  // Fine correction
  y = getY();
  float err = targetY - y;
  while (abs(err) > 0.02) {
    Enes100.print("LANE NAV | Fine correction | Y="); Enes100.print(y);
    Enes100.print(" err="); Enes100.println(err);
    turnTo(err > 0 ? PI / 2 : -PI / 2);
    moveForward(180, 100);
    y = getY();
    err = targetY - y;
  }

  Enes100.print("LANE NAV | Arrived at Y="); Enes100.print(y);
  Enes100.print(" in "); Enes100.print(steps); Enes100.println(" steps");
}

void navigateRow(int row) {
  Enes100.print("\n--- ROW "); Enes100.print(row); Enes100.println(" ---");
  turnTo(0); delay(200);
  int current = getLane();

  if (isBlocked(row, current)) {
    int firstDodge = getNextLane(current);
    navigateToLane(firstDodge);
    turnTo(0); delay(200);

    if (isBlocked(row, firstDodge)) {
      int secondDodge = 3 - current - firstDodge;
      Enes100.print("ROW | Both lanes blocked, falling back to ");
      Enes100.println(laneName(secondDodge));
      navigateToLane(secondDodge);
      turnTo(0); delay(200);
    } else {
      Enes100.println("ROW | Dodge lane clear, proceeding");
    }
  } else {
    Enes100.println("ROW | Lane clear, proceeding");
  }

  float targetX = row == 1 ? 2 : 2.8;
  Enes100.print("ROW | Driving to X="); Enes100.println(targetX);
  float currentX = getX();
  while (currentX < targetX) {
      Enes100.print("ROW | X="); Enes100.print(currentX);
      Enes100.print(" Y="); Enes100.println(getY());
      turnTo(0);
      moveForward(255, 250);
      currentX = getX();  // getX() blocks until visible, so no bad reads
  }
  Enes100.print("ROW "); Enes100.print(row); Enes100.println(" complete\n");
}

void nav() {
  Enes100.println("=============================");
  while (!Enes100.isVisible()) {}
  Enes100.print("NAV | X="); Enes100.print(Enes100.getX());
  Enes100.print(" Y="); Enes100.print(Enes100.getY());
  Enes100.print(" theta="); Enes100.println(Enes100.getTheta());
  Enes100.println("=============================");

  Enes100.println("NAV | Phase 1: Leaving start zone");
  turnTo(0);
  while (Enes100.getX() < 0.5) {
    Enes100.print("NAV | X="); Enes100.println(getX());
    turnTo(0); moveForward(255, 600);
  }
  Enes100.println("NAV | Phase 1 complete");

  Enes100.println("NAV | Phase 2: Clearing obstacle rows");
  navigateRow(1);
  navigateRow(2);
  Enes100.println("NAV | Phase 2 complete");

  Enes100.println("NAV | Phase 3: Moving to bottom lane");
  while (Enes100.getY() > 0.3) {
    Enes100.print("NAV | Y="); Enes100.println(getY());
    turnTo(-PI / 2); moveForward(255, 600);
  }
  Enes100.println("NAV | Phase 3 complete");

  Enes100.println("NAV | Phase 4: Final drive to end of field");
  turnTo(0);
  moveForward(255, 1000000);
  Enes100.println("NAV | Mission complete");
}

// ─── Init ────────────────────────────────────────────────────────────────────
void initPins() {
  int outs[] = {enA1,enB1,in1_1,in2_1,in3_1,in4_1,enA2,enB2,in1_2,in2_2,in3_2,in4_2,trigLeft,trigRight,trigPin1,trigPin2};
  for (int p : outs) pinMode(p, OUTPUT);
  pinMode(echoLeft, INPUT); pinMode(echoRight, INPUT);
  pinMode(echoPin1, INPUT); pinMode(echoPin2, INPUT);
  for (int j = 0; j < 2; j++) pinMode(irPins[j], INPUT);
}

void setup() {
    initPins();


  delay(1000);
  Enes100.begin("FIRETEST", FIRE, 275, 1201, 10, 11);
  delay(1000);
  goToSite();
  doMission();
  nav();
}

void loop() { delay(100); }
