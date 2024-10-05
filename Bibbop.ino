#include "nRF24L01.h"
#include "RF24.h"
#include "L298N.h"

RF24 radio(9, 10);
byte receivedData, pipeNo; 
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

const unsigned int sensors[8] = {0,1,2,3,4,5,6,7}; //поменять пины
const unsigned int IN1 = 3;
const unsigned int IN2 = 4;
const unsigned int EN1 = 5;
const unsigned int IN3 = 7;
const unsigned int IN4 = 8;
const unsigned int EN2 = 6;
const unsigned int accelerationTime = 500;
const unsigned int timeToStop = 100;

bool autoMode = false;
byte moveDirection[2];

L298N motorLeft(EN2, IN3, IN4);
L298N motorRight(EN1, IN1, IN2);

unsigned long moveTimer, autoModeTimer;
int moveSpeed = 0;
int lastMoveIndex = -1;

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
}

void loop() {
  byte pipeNo;
  while ( radio.available(&pipeNo)) { // есть входящие данные
    // чиатем входящий сигнал
    radio.read(&receivedData, sizeof(receivedData));

    if (receivedData == 0)
    {
      autoMode = true;
      autoModeTimer = millis();
    }
    else if (receivedData < 5)
    {
      move(receivedData - 1);
    }
  }
  if (autoMode)
  {
    if(sensors[1])
    {
      if (sensors[0])
      {
        move(0);
      }
      else if (sensors[2])
      {
        move(1);
      }
      else if (sensors[3])
      {
        move(3);
      }
      else
      {
        move(0);
      }
    }
  }
}

void move(int index)
{
  if (moveSpeed == 0)
  {
    moveTimer == millis();
    moveSpeed++;
  }
  if (moveSpeed != 255 && millis() - moveTimer >= accelerationTime / 255)
  {
    motorLeft.setSpeed(moveSpeed);
    motorRight.setSpeed(moveSpeed);
    moveSpeed += (millis() - moveTimer) / accelerationTime / 255;
    if (moveSpeed > 255)
    {
      moveSpeed = 255;
    }
  }
  if (index == lastMoveIndex)
  {
    return;
  }
  switch(index)
  {
    case 0:
      motorLeft.forward();
      motorRight.forward();
      break;
    case 1:
      motorLeft.forward();
      motorRight.backward();
      break;
    case 2:
      motorLeft.backward();
      motorRight.backward();
      break;
    case 3:
      motorLeft.backward();
      motorRight.forward();
      break;
  }
  lastMoveIndex = index;
}

void stopMove()
{
  moveSpeed = 0;
  lastMoveIndex = -1;
  motorLeft.stop();
  motorRight.stop();
}
