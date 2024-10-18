/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/
 
#include <esp_now.h>
#include <WiFi.h>

#define forwardButton 13
#define backwardButton 14
#define rightButton 27
#define leftButton 12
#define autoButton 26

// ЗАМЕНИТЕ МАС-АДРЕСОМ ПЛАТЫ-ПОЛУЧАТЕЛЯ
uint8_t broadcastAddress[] = {0x40, 0x22, 0xD8, 0x7A, 0x26, 0x98};
 
// Структура в скетче платы-отправителя
// должна совпадать с оной для получателя
typedef struct struct_message {
  int code;
} struct_message;
 
// Создаем структуру сообщения myData
struct_message myData;
 
// Обратная функция отправки
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void setup() {
  pinMode(forwardButton, INPUT);
  pinMode(backwardButton, INPUT);
  pinMode(rightButton, INPUT);
  pinMode(leftButton, INPUT);
  pinMode(autoButton, INPUT);
  // Запускаем монитор порта
  Serial.begin(115200);
 
  // Выбираем режим WiFi
  WiFi.mode(WIFI_STA);
 
  // Запускаем протокол ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
 
  // Регистрируем отправку сообщения
  esp_now_register_send_cb(OnDataSent);
  
  // Указываем получателя
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}
 
void loop() {
  // Указываем данные, которые будем отправлять
  if (digitalRead(autoButton))
  {
    myData.code = 0;
  }
  else if (digitalRead(forwardButton))
  {
    myData.code = 1;
  }
  else if (digitalRead(backwardButton))
  {
    myData.code = 2;
  }
  else if (digitalRead(leftButton))
  {
    myData.code = 3;
  }
  else if (digitalRead(rightButton))
  {
    myData.code = 4;
  }
  else
  {
    delay(5);
    return;
  }
 
  // Отправляем сообщение
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  delay(5);
}