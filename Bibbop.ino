#include "nRF24L01.h"
#include "RF24.h"
#include "L298N.h"

RF24 radio(9, 10);
byte receivedData, pipeNo; 
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

const uint8_t sensors[8] = {6,19,14,17,4,5,6,7}; //поменять пины
const unsigned int IN1 = 3;
const unsigned int IN2 = 4;
const unsigned int EN1 = 5;
const unsigned int IN3 = 7;
const unsigned int IN4 = 8;
const unsigned int EN2 = 6;
const unsigned int accelerationTime = 1, continueTime = 2000;
const unsigned int timeToStop = 100;

bool autoMode = true;
byte moveDirection[2];

L298N motorLeft(EN2, IN3, IN4);
L298N motorRight(EN1, IN1, IN2);

unsigned long moveTimer, stopTimer, continueTimer;
int moveSpeedR = 200, moveSpeedL = 210;


void move(int index)
{
  if(index == 0)
  {
    motorRight.setSpeed(moveSpeedR);
    motorLeft.setSpeed(moveSpeedL);
    motorRight.forward();
    motorLeft.forward();
  }
  if(index == 1)
  {
    motorRight.setSpeed(moveSpeedR / 3);
    motorLeft.setSpeed(moveSpeedL);
    motorRight.forward();
    motorLeft.forward();
  }
  if(index == 2)
  {
    motorRight.setSpeed(moveSpeedR / 3);
    motorLeft.setSpeed(moveSpeedL);
    motorRight.backward();
    motorLeft.forward();
  }
  if(index == 3)
  {
    motorRight.setSpeed(moveSpeedR);
    motorLeft.setSpeed(moveSpeedL);
    motorRight.backward();
    motorLeft.forward();
  }
  if(index == -1)
  {
    motorRight.setSpeed(moveSpeedR);
    motorLeft.setSpeed(moveSpeedL / 3);
    motorRight.forward();
    motorLeft.forward();
  }
  if(index == -2)
  {
    motorRight.setSpeed(moveSpeedR);
    motorLeft.setSpeed(moveSpeedL / 3);
    motorRight.forward();
    motorLeft.backward();
  }
  if(index == -3)
  {
    motorRight.setSpeed(moveSpeedR);
    motorLeft.setSpeed(moveSpeedL);
    motorRight.forward();
    motorLeft.backward();
  }
}

void stopMove()
{
  motorLeft.stop();
  motorRight.stop();
}

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

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  pinMode(A6, INPUT);
  pinMode(2, INPUT);
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
  if (moveSpeedR != 0 && millis() - stopTimer > 150)
  {
    //stopMove();
    //autoMode = false;
  }
  if (autoMode)
  {
    if(analogRead(A6) < 100 || analogRead(A5) < 100)
    {
      move(0);
      continueTimer = millis();
    }
    else if(!digitalRead(2))
    {
      move(-3);
      continueTimer = millis();
    }
    else if(analogRead(A0) < 100)
    {
      move(-2);
      continueTimer = millis();
    }
    else if(analogRead(A1) < 100)
    {
      move(-1);
      continueTimer = millis();
    }
    else if(analogRead(A2) < 100)
    {
      move(3);
      continueTimer = millis();
    }
    else if(analogRead(A3) < 100)
    {
      move(2);
      continueTimer = millis();
    }
    else if(analogRead(A4) < 100)
    {
      move(1);
      continueTimer = millis();
    }
    else
    {
      if (millis() - continueTimer > continueTime)
      {
        stopMove();
      }
    }
  }
}
