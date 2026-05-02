  #include "Arduino.h"
  #include "Enes100.h"

  // ─── Pin Configuration ───────────────────────────────────────────────────────
  const int enA1  = 2;  const int enB1  = 3;
  const int in1_1 = 39; const int in2_1 = 37;
  const int in3_1 = 35; const int in4_1 = 33;

  const int enA2  = 4;  const int enB2  = 5;
  const int in1_2 = 38; const int in2_2 = 36;
  const int in3_2 = 34; const int in4_2 = 32;

  const int trigLeft  = 50; 
  const int echoLeft  = 51; 
  const int trigRight = 52;  
  const int echoRight = 53;  

  
#define BOT 0
#define MID 1
#define TOP 2

const float blockedVal = 40.0;
const float LANE_Y[3]  = { 0.3, 0.75, 1.6 };

  // ─── Constants ───────────────────────────────────────────────────────────────
  const float CENTER_TOL    = 10.0; //tune
  const int   NUDGE_SPEED   = 255; 
  const int   NUDGE_TURN_MS = 800;   // tune
  const int   NUDGE_FWD_MS  = 800;  // tune

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
    setMotor(enA2, in1_2, in2_2, speed * 0.9, true);
    setMotor(enB2, in3_2, in4_2, speed, true);
    delay(duration);
    stopMotors();
  }

  void turnRight(int speed, int duration) {
    setMotor(enA1, in1_1, in2_1, speed, true);
    setMotor(enB1, in3_1, in4_1, speed, true);
    setMotor(enA2, in1_2, in2_2, speed * 0.9, false);
    setMotor(enB2, in3_2, in4_2, speed, false);
    delay(duration);
    stopMotors();
  }

  void turnTo(float target) {
    bool aligned = false;
    while (!aligned) {
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

const int trigPin1 = 44;
const int echoPin1 = 43;
const int trigPin2 = 48;
const int echoPin2 = 49;

  // ─── Sensors ─────────────────────────────────────────────────────────────────
  float readDistance(int trig, int echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);
    long dur = pulseIn(echo, HIGH, 25000);
    if (dur == 0) return -1;
    return dur * 0.0343 / 2.0;
  }

  bool closeEnough(int measured, int target) {
    float TOLERANCE = 1;
  return abs(measured - target) <= TOLERANCE;
}

void printOrientation(int h1, int h2) {
  if (closeEnough(h1, 6) && closeEnough(h2, 10)) {
    Enes100.println("Orientation: C");
  }
  else if (closeEnough(h1, 7) && closeEnough(h2, 5)) {
    Enes100.println("Orientation: A");
  }
  else if (closeEnough(h1, 5) && closeEnough(h2, 7)) {
    Enes100.println("Orientation: B");
  }
  else {
    Enes100.println("Orientation: Unknown");
  }
}

  float sensorAverage(int trig, int echo) {
    float sum = 0;
    int count = 0;
    for (int i = 0; i < 5; i++) {
      float d = readDistance(trig, echo);
      if (d > 0) { sum += d; count++; }
      delay(10);
    }
    return count > 0 ? sum / count : -1;
  }

  float readLeft()  { return sensorAverage(trigLeft,  echoLeft);  }
  float readRight() { return sensorAverage(trigRight, echoRight); }

  // ─── Movement ────────────────────────────────────────────────────────────────
  void nudge(bool goRight, float heading) {
    turnTo(heading);
    if (goRight) {
      Enes100.println("Nudging Right");
      turnRight(255, NUDGE_TURN_MS);
      delay(1000);
      moveForward(NUDGE_SPEED, NUDGE_FWD_MS);
    } else {
      Enes100.println("Nudging Left");
      turnLeft(255, NUDGE_TURN_MS);
      delay(1000);
      moveForward(NUDGE_SPEED, NUDGE_FWD_MS);
    }
    Enes100.println("Turning back to our target");
    turnTo(heading);
  }

  void correctDrift(float heading) {
    delay(1000);
    float left  = readLeft();
    float right = readRight();
    Enes100.print("Left: ");
    Enes100.print(left);
    Enes100.print("Right: ");
    Enes100.print(right);
    if (left < 0 || right < 0) return;
    float diff = left - right;
    if      (diff >  CENTER_TOL) { nudge(true,  heading); correctDrift(heading); }
    else if (diff < -CENTER_TOL) { nudge(false, heading); correctDrift(heading); }
  }

  float getY() {
    while (!Enes100.isVisible()) {}
    return Enes100.getY();
  }
    float getX() {
    while (!Enes100.isVisible()) {}
    return Enes100.getX();
  }

  // ─── Main routine ────────────────────────────────────────────────────────────
  void goToSite() {
    float startY  = getY();
    float heading = startY < 1.0 ? PI / 2 : -PI / 2;
    float targetY = startY < 1.0 ? 1.3    : 0.5;
    bool  goingUp = startY < 1.0;

    turnTo(heading);

    while (!(goingUp ? getY() >= targetY : getY() <= targetY)){
      correctDrift(heading);
      delay(1000);
      Enes100.println("Moving forward as we are aligned"); 
      moveForward(255, 200);
    }
  }

int medianDistance(int trigPin, int echoPin) {
    int readings[5];
    for (int i = 0; i < 5; i++) {
        readings[i] = readDistance(trigPin, echoPin);
        delay(10);
    }
    
    // Simple bubble sort
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4 - i; j++) {
            if (readings[j] > readings[j + 1]) {
                int temp = readings[j];
                readings[j] = readings[j + 1];
                readings[j + 1] = temp;
            }
        }
    }
    
    return readings[2]; // middle value
}

const int DRIVE_SPEED = 255;
const int SHAKE_TIME_MS = 35;
const int SHAKE_PAUSE_MS = 60;

void shakeOverFans(int cycles) {
  for (int i = 0; i < cycles; i++) {
    moveForward(DRIVE_SPEED, SHAKE_TIME_MS);
    delay(SHAKE_PAUSE_MS);
    moveBackward(DRIVE_SPEED, SHAKE_TIME_MS);
    delay(SHAKE_PAUSE_MS);
  }
}


  void doMission(){
    int amountOn = 1;
    const int irPins[2] = {22, 23};
     for (int j = 0; j < 2; j++) {
    pinMode(irPins[j], INPUT);
  }


    float goodStartingDist = 30;
    while(abs(medianDistance(trigLeft, echoLeft) - goodStartingDist) > 1){
      if (medianDistance(trigLeft, echoLeft) > goodStartingDist){
        moveBackward(255, 100);
      }
      else{
        moveForward(255, 100);
      }
    }


for (int j = 0; j < 2; j++) {
    if(digitalRead(irPins[j]) == LOW){
      amountOn++;
    }
  }


    float goodFanDist = 20;
    while(abs(medianDistance(trigLeft, echoLeft) - goodFanDist) > 1){
      if (medianDistance(trigLeft, echoLeft) > goodFanDist){
        moveBackward(255, 100);
      }
      else{
        moveForward(255, 100);
      }
    }
  shakeOverFans(60);

      float goodSecondReadDist = 20;
    while(abs(medianDistance(trigLeft, echoLeft) - goodSecondReadDist) > 1){
      if (medianDistance(trigLeft, echoLeft) > goodSecondReadDist){
        moveBackward(255, 100);
      }
      else{
        moveForward(255, 100);
      }
    }


for (int j = 0; j < 2; j++) {
    if(digitalRead(irPins[j]) == LOW){
      amountOn++;
    }
  }  
    float goodSecondFanDist = 20;
    while(abs(medianDistance(trigLeft, echoLeft) - goodSecondFanDist) > 1){
      if (medianDistance(trigLeft, echoLeft) > goodSecondFanDist){
        moveBackward(255, 100);
      }
      else{
        moveForward(255, 100);
      }
    }
  shakeOverFans(60);


    float getOrdientationDist = 40;
    while(abs(medianDistance(trigLeft, echoLeft) - getOrdientationDist) > 1){
      if (medianDistance(trigLeft, echoLeft) > getOrdientationDist){
        moveBackward(255, 100);
      }
      else{
        moveForward(255, 100);
      }

    }

      Enes100.println(amountOn);

    

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
int val1 = medianDistance(trigPin1, echoPin1);

  int val2 = (medianDistance(trigPin2, echoPin2));

      printOrientation(val1, val2);



      moveBackward(255, 5000);

  }


// ─── Lane helpers ────────────────────────────────────────────────────────────

int getLane() {
  float y;
  bool foundY = false;
  while (!foundY) {
    if (Enes100.isVisible()) { y = Enes100.getY(); foundY = true; }
  }
  int lane = (y > 1.3) ? TOP : (y > 0.75) ? MID : BOT;
  Enes100.print("[LANE] Y="); Enes100.print(y);
  Enes100.print(" -> lane=");
  Enes100.println(lane == TOP ? "TOP" : lane == MID ? "MID" : "BOT");
  return lane;
}

bool isBlocked(int row, int lane) {
  Enes100.print("[BLOCKED?] Row="); Enes100.print(row);
  Enes100.print(" Lane="); Enes100.println(lane == TOP ? "TOP" : lane == MID ? "MID" : "BOT");
  float dist = medianDistance(trigLeft, echoLeft);
  bool blocked = (dist < blockedVal && dist != -1);
  Enes100.print("[BLOCKED?] dist="); Enes100.print(dist);
  Enes100.print(" threshold="); Enes100.print(blockedVal);
  Enes100.println(blocked ? " -> BLOCKED" : " -> CLEAR");
  return blocked;
}

int getNextLane(int current) {
  int next = (current == TOP) ? BOT : (current == MID) ? TOP : MID;
  Enes100.print("[DODGE] From ");
  Enes100.print(current == TOP ? "TOP" : current == MID ? "MID" : "BOT");
  Enes100.print(" -> try ");
  Enes100.println(next == TOP ? "TOP" : next == MID ? "MID" : "BOT");
  return next;
}

void navigateToLane(int target) {
  float targetY = LANE_Y[target];
  float y;
  bool foundY = false;
  while (!foundY) {
    if (Enes100.isVisible()) { y = Enes100.getY(); foundY = true; }
  }
  Enes100.print("[LANE NAV] Y="); Enes100.print(y);
  Enes100.print(" -> targetY="); Enes100.print(targetY);
  Enes100.print(" ("); 
  Enes100.print(target == TOP ? "TOP" : target == MID ? "MID" : "BOT");
  Enes100.println(")");

  bool goingUp = y < targetY;
  float heading = goingUp ? PI / 2 : -PI / 2;
  Enes100.println(goingUp ? "[LANE NAV] Direction: UP" : "[LANE NAV] Direction: DOWN");

  int steps = 0;
  while (goingUp ? y < targetY : y > targetY) {
    turnTo(heading);
    moveForward(255, 250);
    if (Enes100.isVisible()) y = Enes100.getY();
    steps++;
    Enes100.print("[LANE NAV] step="); Enes100.print(steps);
    Enes100.print(" Y="); Enes100.print(y);
    Enes100.print(" targetY="); Enes100.println(targetY);
  }
  Enes100.print("[LANE NAV] Arrived | Y="); Enes100.print(y);
  Enes100.print(" in "); Enes100.print(steps); Enes100.println(" steps");
}

void navigateRow(int row) {
  Enes100.print("\n=== ROW "); Enes100.print(row); Enes100.println(" START ===");
  Enes100.print("[ROW] X="); Enes100.print(Enes100.getX());
  Enes100.print(" Y="); Enes100.print(Enes100.getY());
  Enes100.print(" theta="); Enes100.println(Enes100.getTheta());

  turnTo(0);
  delay(200);
  int current = getLane();

  if (!isBlocked(row, current)) {
    Enes100.println("[ROW] Lane clear, no dodge needed");
  } else {
    int firstDodge = getNextLane(current);
    Enes100.print("[ROW] Lane blocked, navigating to dodge lane ");
    Enes100.println(firstDodge == TOP ? "TOP" : firstDodge == MID ? "MID" : "BOT");
    navigateToLane(firstDodge);
    turnTo(0); delay(200);

    if (!isBlocked(row, firstDodge)) {
      Enes100.println("[ROW] Dodge lane clear");
    } else {
      int secondDodge = 3 - current - firstDodge;
      Enes100.print("[ROW] Dodge lane also blocked, trying last lane ");
      Enes100.println(secondDodge == TOP ? "TOP" : secondDodge == MID ? "MID" : "BOT");
      navigateToLane(secondDodge);
      turnTo(0); delay(200);
      Enes100.println("[ROW] Using last available lane");
    }
  }

  float targetX = (row == 1) ? 1.5 : 2.8;
  Enes100.print("[ROW] Driving forward to X="); Enes100.println(targetX);

  bool notVisible = true;
  while (notVisible) {
    if (Enes100.isVisible()) {
      while (Enes100.getX() < targetX) {
        Enes100.print("[ROW] X="); Enes100.print(Enes100.getX());
        Enes100.print(" Y="); Enes100.print(Enes100.getY());
        Enes100.print(" theta="); Enes100.println(Enes100.getTheta());
        turnTo(0);
        moveForward(255, 250);
      }
      notVisible = false;
    }
  }
  Enes100.print("=== ROW "); Enes100.print(row); Enes100.println(" COMPLETE ===\n");
}
  void nav(){
  Enes100.println("=============================");
  Enes100.println("[INIT] System online");
  Enes100.print("[INIT] X="); Enes100.print(Enes100.getX());
  Enes100.print(" Y="); Enes100.print(Enes100.getY());
  Enes100.print(" theta="); Enes100.println(Enes100.getTheta());
  Enes100.println("=============================");



  Enes100.println("[PHASE 1] Leaving start zone");
  turnTo(0);
  while (Enes100.getX() < 0.5) {
    Enes100.print("[PHASE 1] X="); Enes100.println(Enes100.getX());
    turnTo(0);
    moveForward(255, 600);
  }
  Enes100.println("[PHASE 1] Start zone cleared");

  Enes100.println("[PHASE 2] Navigating obstacle rows");
  navigateRow(1);
  navigateRow(2);
  Enes100.println("[PHASE 2] Obstacle rows cleared");

  Enes100.println("[PHASE 3] Moving to bottom lane");
  while (Enes100.getY() > 0.3) {
    Enes100.print("[PHASE 3] Y="); Enes100.println(Enes100.getY());
    turnTo(-PI / 2);
    moveForward(255, 600);
  }
  Enes100.println("[PHASE 3] Bottom lane reached");

  Enes100.println("[PHASE 4] Driving to end of field");
turnTo(0);
  moveForward(255, 1000000);
  Enes100.println("[PHASE 4] End of field reached — mission complete");

}

  // ─── Init ────────────────────────────────────────────────────────────────────
  void initPins() {
    pinMode(enA1, OUTPUT); pinMode(enB1, OUTPUT);
    pinMode(in1_1, OUTPUT); pinMode(in2_1, OUTPUT);
    pinMode(in3_1, OUTPUT); pinMode(in4_1, OUTPUT);
    pinMode(enA2, OUTPUT); pinMode(enB2, OUTPUT);
    pinMode(in1_2, OUTPUT); pinMode(in2_2, OUTPUT);
    pinMode(in3_2, OUTPUT); pinMode(in4_2, OUTPUT);
    pinMode(trigLeft,  OUTPUT); pinMode(echoLeft,  INPUT);
    pinMode(trigRight, OUTPUT); pinMode(echoRight, INPUT);
     

  }

  void setup() {
    delay(1000);
    Enes100.begin("FIRETEST", FIRE, 275, 1116, 10, 11);
    delay(1000);
    initPins();

//    turnLeft(255, 10000000);

    goToSite();
     doMission();
     nav();
  }

  void loop() { delay(100); }