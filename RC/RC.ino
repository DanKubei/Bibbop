/*   Данный скетч делает следующее: передатчик (TX) отправляет массив
     данных, который генерируется согласно показаниям с кнопки и с
     двух потенциомтеров. Приёмник (RX) получает массив, и записывает
     данные на реле, сервомашинку и генерирует ШИМ сигнал на транзистор.
    by AlexGyver 2016
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

RF24 radio(9, 10); // "создать" модуль на пинах 9 и 10 Для Уно

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

const unsigned int forwardButton = 5;
const unsigned int backwardButton = 8;
const unsigned int leftButton = 7;
const unsigned int rightButton = 4;
const unsigned int autoButton = 3;

byte transmit_data;  // массив, хранящий передаваемые данные

void setup() {
  Serial.begin(9600);   //открываем порт для связи с ПК

  pinMode(forwardButton, INPUT);
  pinMode(backwardButton, INPUT);
  pinMode(leftButton, INPUT);
  pinMode(rightButton, INPUT);
  pinMode(autoButton, INPUT);

  radio.begin();              // активировать модуль
  radio.setAutoAck(1);        // режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    // (время между попыткой достучаться, число попыток)
  radio.enableAckPayload();   // разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);   // размер пакета, в байтах

  radio.openWritingPipe(address[0]);  // мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x69);     // выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX);   // уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_250KBPS); // скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp();        //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
}

void loop() {
  if (digitalRead(autoButton))
  {
    transmit_data = 0;
  }
  else if (digitalRead(forwardButton))
  {
    transmit_data = 1;
  }
  else if (digitalRead(backwardButton))
  {
    transmit_data = 3;
  }
  else if (digitalRead(leftButton))
  {
    transmit_data = 4;
  }
  else if (digitalRead(rightButton))
  {
    transmit_data = 2;
  }
  if (transmit_data != -1){
    radio.powerUp();    // включить передатчик
    radio.write(&transmit_data, sizeof(transmit_data)); // отправить по радио
    radio.powerDown();
    delay(100);
  }
  transmit_data = -1;
}
