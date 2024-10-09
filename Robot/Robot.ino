#include "nRF24L01.h"
#include "RF24.h"
#include "L298N.h"

//region Radio Settings and Variables
RF24 radio(9, 10);
byte receivedData, pipeNo; 
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};
//endregion
//region Motor Settings and Variables
const unsigned int minPWM = 100, maxPWM = 200, MDC = 0; // MDC - motorDifferneceCorrection at full power. Negative - left motor correction, positive - right motor correction
const unsigned int IN1 = 3;
const unsigned int IN2 = 4;
const unsigned int EN1 = 5;
const unsigned int IN3 = 7;
const unsigned int IN4 = 8;
const unsigned int EN2 = 6;
const unsigned int continueTime = 2000; // value which set how long robot must continue execute command of move, when signals is end
const unsigned int timeToStop = 100; // value which set how long robot must continue execute command without signal from remote control
L298N motorLeft(EN2, IN3, IN4);
L298N motorRight(EN1, IN1, IN2);
//endregion
//region sensors functions
const float sensorsWeights[8] = {-1,-0.718,-0.431,-0.144,0.144,0.431,0.718,1}; // {-0.436,-0.313,-0.188,-0.063,0.063,0.188,0.313,0.436}; {-1,-0.718,-0.431,-0.144,0.144,0.431,0.718,1};
void initSensors()
{
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  pinMode(A6, INPUT);
  pinMode(2, INPUT);
}
/*
* Sensors get signal when they don't see the object
* And for logic comfort, values is reversed
*/
bool getSensor(int index) // from 0 to 7, from left to right
{
  switch (index) // change pins according to the order of the row
  {
    case 0:
      return !digitalRead(2);
    break;
    case 1:
      return analogRead(A1) < 100;
    break;
    case 2:
      return analogRead(A6) < 100;
    break;
    case 3:
      return analogRead(A4) < 100;
    break;
    case 4:
      return analogRead(A2) < 100;
    break;
    case 5:
      return analogRead(A3) < 100;
    break;
    case 6:
      return analogRead(A5) < 100;
    break;
    case 7:
      return analogRead(A0) < 100;
    break;
  }
}
//endregion
//region Local Variables and Settings
bool autoMode = true;
unsigned long stopTimer, continueTimer;
int maxMoveSpeed = 100;
//endregion

void setup() {
  radio.begin(); //активировать модуль
  radio.setAutoAck(1);        // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();   // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);   // размер пакета, в байтах

  radio.openReadingPipe(1, address[0]);     // хотим слушать трубу 0
  radio.setChannel(0x69);  // выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX);   // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); // скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp();          // начать работу
  radio.startListening();

  initSensors();
}

void loop() {
  byte pipeNo;
  while ( radio.available(&pipeNo)) { // есть входящие данные
    // чиатем входящий сигнал
    radio.read(&receivedData, sizeof(receivedData));

    if (receivedData == 0)
    {
      autoMode = true;
      stopTimer = millis();
    }
    else if (receivedData < 5)
    {
      move(receivedData - 1);
      stopTimer - millis();
    }
  }
  if (millis() - stopTimer > 150)
  {
    //stopMove();
    //autoMode = false;
  }
  if (autoMode)
  { 
    autoMove();
  }
}

void autoMove()
{
  float vector = 0;
  bool skip = true;
  for (int i = 0; i < 8; i++)
  {
    if (getSensor(i))
    {
      vector += sensorsWeights[i];
      skip = false;
      continueTimer = millis();
    }
  }
  if (!skip)
  {
    move(vector);
  }
  else if (millis() - continueTimer > continueTime)
  {
    stopMove();
  }
}

void move(float vector) // -1 - move on the left, 0 - forward move, 1 - move on the right
{
  if (vector > 1) vector = 1;
  if (vector < -1) vector = -1;
  //calculating speed and motors direction
  int leftMotorSpeed, rightMotorSpeed;
  bool leftMotorReverse = false, rightMotorReverse = false;
  if (vector >= 0)
  {
    leftMotorSpeed = map(maxMoveSpeed,0,100,minPWM,maxPWM);
    float rightMotorPower = (0.5 - vector) * 2;
    if (vector > 0.5)
    {
      rightMotorReverse = true;
      rightMotorPower *= -1;
    }
    rightMotorSpeed = map((int)(maxMoveSpeed * rightMotorPower),0,100,minPWM,maxPWM);
  }
  else
  {
    rightMotorSpeed = map(maxMoveSpeed,0,100,minPWM,maxPWM);
    float leftMotorPower = (0.5 + vector) * 2;
    if (vector < -0.5)
    {
      leftMotorReverse = true;
      leftMotorPower *= -1;
    }
    leftMotorSpeed = map((int)(maxMoveSpeed * leftMotorPower),0,100,minPWM,maxPWM);
  }
  // correction motor speed
  if (MDC > 0)
  {
    if (rightMotorSpeed + MDC / maxPWM > maxPWM)
    {
      leftMotorSpeed -= MDC / maxPWM;
    }
    else
    {
      rightMotorSpeed += MDC / maxPWM;
    }
  }
  else 
  {
    if (leftMotorSpeed - MDC / maxPWM > maxPWM)
    {
      rightMotorSpeed += MDC / maxPWM;
    }
    else
    {
      leftMotorSpeed -= MDC / maxPWM;
    }
  }
  // set motors
  motorRight.setSpeed(rightMotorSpeed);
  motorLeft.setSpeed(leftMotorSpeed);
  if (rightMotorReverse)
  {
    motorRight.backward();
  }
  else
  {
    motorRight.forward();
  }
  if(leftMotorReverse)
  {
    motorLeft.backward();
  }
  else
  {
    motorLeft.forward();
  }
}

void stopMove()
{
  motorLeft.stop();
  motorRight.stop();
}

void extraStop()
{
  motorLeft.setSpeed(maxPWM);
  motorRight.setSpeed(maxPWM);
  motorLeft.backward();
  motorRight.backward();
}
