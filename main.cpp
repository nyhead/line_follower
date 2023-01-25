#include <Arduino.h>

#include "Servo.h"

const int led = 11;
const int button = 2;
const int left = 12, right = 13;
const int sensors[] = {A0,A1,A2,A3,A4};
// const int sensors[] = {3,4,5,6,7};
const size_t sensorCount = 5;
const int black = 0, white = 1;


struct Bot {
  enum States {Straight, Left, Right, Intersection, UntilStraight, UntilLeftRight};
  bool running = false;
  bool leftRoute = true;
  States state;
};

class motor : public Servo
{
public:
motor(void) { _dir=1; }
void go(int percentage) {
writeMicroseconds(1500+_dir*percentage*2); // or so
}
void setDirection(bool there) {
if(there)
_dir = 1;
else
_dir = -1;
}
private:
int _dir;
};

motor leftMotor, rightMotor;
const int lowBlack = 0, highBlack = 500;
const int lowWhite = 700, highWhite = 1024;
bool inRangeBlack(int val){ return lowBlack <= val && val <= highBlack; }
bool inRangeWhite(int val){ return lowWhite <= val && val <= highWhite; }
bool inRangeGray(int val) {return highBlack - 100 <= val && val <= lowWhite - 200;}
bool isIntersection(int middleLeft, int middle, int middleRight) {return (!inRangeWhite(middle) && !inRangeWhite(middleLeft) && !inRangeWhite(middleRight));}
bool isStraight(int middleLeft, int middleRight) {return (inRangeWhite(middleLeft) && inRangeWhite(middleRight));}
bool allWhite(int left, int middleLeft, int middle, int middleRight, int right) {return inRangeWhite(middle) && inRangeWhite(middleLeft) && inRangeWhite(middleRight) && inRangeWhite(left) && inRangeWhite(right);}
bool leftTurn(int middleLeft, int middle, int middleRight) {return !inRangeBlack(middle) && inRangeBlack(middleLeft) && !inRangeBlack(middleRight);}
bool rightTurn(int middleLeft, int middle, int middleRight) {return !inRangeBlack(middle) && !inRangeBlack(middleLeft) && inRangeBlack(middleRight);}

Bot bot;
void setup() {
  Serial.begin(9600);
  pinMode(led, OUTPUT);  
  pinMode(button,INPUT_PULLUP);
  for(size_t i = 0; i < sensorCount; i++) {
    pinMode(sensors[i], INPUT);
  }
  leftMotor.attach(left, 500, 2500);
  rightMotor.attach(right, 500, 2500);
  leftMotor.setDirection(true);
  rightMotor.setDirection(false);
  digitalWrite(led,0);
}

void Debug(int left, int middleLeft, int middle, int middleRight, int right) {
  Serial.println(left);
  Serial.println(middleLeft);
  Serial.println(middle);
  Serial.println(middleRight);
  Serial.println(right);
  Serial.print("\n\n\n\n\n");
}


void interTurnLoop() {
  int middleLeft = analogRead(sensors[1]);
  int middle = analogRead(sensors[2]);
  int middleRight = analogRead(sensors[3]);

  while (!isStraight(middleLeft,middle, middleRight)) {
      middleLeft = analogRead(sensors[1]);
      middle = analogRead(sensors[2]);
      middleRight = analogRead(sensors[3]); 
      if (bot.leftRoute) {
        leftMotor.go(-20);
        rightMotor.go(100);           
      } else {
        leftMotor.go(100);
        rightMotor.go(-20);
        
        // if (!isIntersection(middleLeft,middle, middleRight) && inRangeBlack(middleRight)) break;   
      }
      if (rightTurn(middleLeft,middle, middleRight)) break;
      if (leftTurn(middleLeft,middle, middleRight)) break;

  } 

}

bool  MarkReading(int left, int right) {
    if (inRangeBlack(left)) {
      bot.leftRoute = true;
      return true;
    }
    else if (inRangeBlack(right)) {
      bot.leftRoute = false;
      return true;
    }
    return false;
}
unsigned long last = 0;
unsigned long interLast = 0;
unsigned long interWaitTime = 0;
unsigned long orgWaitTime = 20;
unsigned long waitTime = orgWaitTime;
bool intersectionMode = false;
int left = 0, right = 0;
void loop() {
  // put your main code here, to run repeatedly:
  // 1 -black 0 - white for simplicity
  // black 100 - 300 , white 900-

  int left = analogRead(sensors[0]);
  int middleLeft = analogRead(sensors[1]);
  int middle = analogRead(sensors[2]);
  int middleRight = analogRead(sensors[3]);
  int right = analogRead(sensors[4]);


  unsigned long now = millis();

  if (inRangeBlack(left) && inRangeBlack(right)) {
        bot.running = false;
  }

  if ((now - interLast) >=  interWaitTime) {
    interWaitTime = 0;
    if (MarkReading(left,right)) interWaitTime = 5000; 
    interLast = now;
  }

  if (!digitalRead(button)) {
    bot.running = !bot.running;
  }

  if (bot.running) {
    switch(bot.state) {
      case bot.Straight:
        left = 20;
        right = 20;
        bot.state = bot.UntilStraight;
        break;
      case bot.Left:
        left = -5;
        right = 25;
        bot.state = bot.UntilLeftRight;
        break;      
      case bot.Right:
        left = 25;
        right = -5;      
        bot.state = bot.UntilLeftRight;
        break;      
      case bot.Intersection:
        if (bot.leftRoute) {
          left = 0;
          right = 100;          
        }
        else {
          left = 100;
          right = 0;
        }
        bot.state = bot.UntilStraight;
        break;
      case bot.UntilStraight:
        if (inRangeBlack(right))
          bot.state = bot.Right;
        else if (inRangeBlack(left))
          bot.state = bot.Left;
        else if (isIntersection(middleLeft, middle, middleRight))
          bot.state = bot.Intersection;
        break;
      case bot.UntilLeftRight:
        if (isStraight(middleLeft, middleRight))
          bot.state = bot.Straight;
        break;    
    }
  }

  leftMotor.go(left);
  rightMotor.go(right);
}