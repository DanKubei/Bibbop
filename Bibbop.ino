#include "nRF24L01.h"
#include "RF24.h"
#include "L298N.h"

RF24 radio(9, 10);
byte recived_data, pipeNo; 
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"};

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

L298N motorLeft(EN2, IN3, IN4); // переиминовать
L298N motorRight(EN1, IN1, IN2);

unsigned long moveTimer;
byte moveSpeed = 0;
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
  motorLeft.setSpeed(255);
  motorRight.setSpeed(255);
  motorLeft.forward();
  motorRight.forward();
}

unsigned long commandTimer;

void loop() {
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
