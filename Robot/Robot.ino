#include "esp32-hal-ledc.h"
#include <esp_wifi.h>
#include <esp_now.h>
#include <WiFi.h>

#define motor_right_pwm 4
#define motor_right_forward 23
#define motor_right_backward 22
#define motor_left_pwm 15
#define motor_left_forward 19
#define motor_left_backward 18

#define sensor_0 14
#define sensor_1 27
#define sensor_2 26
#define sensor_3 25
#define sensor_4 33
#define sensor_5 32
#define sensor_6 35
#define sensor_7 34

#define min_pwm 35000
#define max_pwm 40000

#define commandTimeout 10

float sensorsPos[8] = {3.5,2.5,1.5,0.5,-0.5,-1.5,-2.5,-3.5};

typedef struct struct_message {
    int a;
} struct_message;

struct_message myData;
uint8_t newMACAddress[] = {0x30, 0xAE, 0xA4, 0x07, 0x0D, 0x66};

const int frequency = 30;
const int resolution = 16;

long lastCommandTimer;

void OnDataRecv(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len) {
  memcpy(&myData, data, sizeof(myData));
  switch(myData.a)
  {
    case 0:
      autoMove();
    break;
    case 1:
      set_motors_speed(100, 100, false, false);
    break;
    case 2:
      set_motors_speed(100, 100, true, true);
    break;
    case 3:
      set_motors_speed(100, 100, false, true);
    break;
    case 4:
      set_motors_speed(100, 100, true, false);
    break;
  }
  lastCommandTimer = millis();
}

int get_speed(double percent)
{
    return map(percent,0,100,min_pwm,max_pwm);
}

float clamp(float value, float min, float max)
{
    if (value < min)
    {
        value = min;
    }
    if (value > max)
    {
        value = max;
    }
    return value;
}

void set_motors_speed(int right_motor_speed, int left_motor_speed, bool right_motor_reverse, bool left_motor_reverse)
{
  ledcWrite(motor_right_pwm, get_speed(right_motor_speed));
  ledcWrite(motor_left_pwm, get_speed(left_motor_speed));
  digitalWrite(motor_right_forward, right_motor_reverse ? 0 : 1);
  digitalWrite(motor_right_backward, right_motor_reverse ? 1 : 0);
  digitalWrite(motor_left_forward, left_motor_reverse ? 0 : 1);
  digitalWrite(motor_left_backward, left_motor_reverse ? 1 : 0);
}

void stopMotors()
{
  ledcWrite(motor_right_pwm, 0);
  ledcWrite(motor_left_pwm, 0);
}

float P(float setPoint, float input)
{
  return setPoint - input;
}

long ITimer;
float lastI;

float I(float setPoint, float input)
{
  float output = lastI + (setPoint - input) * (millis() - ITimer) / 1000;
  ITimer = millis();
  lastI = output;
  return output;
}

long DTimer;
float lastError;

float D(float setPoint, float input)
{
  float error = setPoint - input;
  float output = (error - lastError) / (millis() - DTimer) / 1000;
  DTimer = millis();
  lastError = error;
  return output;
}

float PID(float setPoint, float input, float Pk, float Ik, float Dk)
{
  return P(setPoint, input) * Pk + I(setPoint, input) * Ik + D(setPoint, input) * Dk;
}

void autoMove()
{
  float target = 0;
  float position = 0;
  for (int i = 0; i < 8; i++)
  {
    if (!get_sensor(i))
    {
      position += sensorsPos[i];
    }
  }
  move(PID(target, position, 0.2, 0.07, 0));
}

void move(float vector)
{
    vector = clamp(vector,-1,1);
    int right_motor_speed = (int)(100.0 * clamp(0.5 - vector, -0.5, 0.5) * 2.0);
    int left_motor_speed = (int)(100.0 * clamp(0.5 + vector, -0.5, 0.5) * 2.0);
    bool right_motor_reverse = false;
    bool left_motor_reverse = false;
    if (right_motor_speed < 0)
    {
        right_motor_reverse = true;
        right_motor_speed *= -1;
    }
    if (left_motor_speed < 0)
    {
        left_motor_reverse = true;
        left_motor_speed *= -1;
    }
    set_motors_speed(right_motor_speed, left_motor_speed, right_motor_reverse, left_motor_reverse);
}

bool get_sensor(int index)
{
    switch(index)
    {
        case 0:
            return digitalRead(sensor_0);
        break;
        case 1:
            return digitalRead(sensor_1);
        break;
        case 2:
            return digitalRead(sensor_2);
        break;
        case 3:
            return digitalRead(sensor_3);
        break;
        case 4:
            return digitalRead(sensor_4);
        break;
        case 5:
            return digitalRead(sensor_5);
        break;
        case 6:
            return digitalRead(sensor_6);
        break;
        case 7:
            return digitalRead(sensor_7);
        break;
    }
}

void initMotors()
{
  pinMode(motor_right_forward, OUTPUT);
  pinMode(motor_right_backward, OUTPUT);
  pinMode(motor_left_forward, OUTPUT);
  pinMode(motor_left_backward, OUTPUT);

  ledcAttach(motor_right_pwm, frequency, resolution);
  ledcAttach(motor_left_pwm, frequency, resolution);
}

void initSensors()
{
  pinMode(sensor_0, INPUT);
  pinMode(sensor_1, INPUT);
  pinMode(sensor_2, INPUT);
  pinMode(sensor_3, INPUT);
  pinMode(sensor_4, INPUT);
  pinMode(sensor_5, INPUT);
  pinMode(sensor_6, INPUT);
  pinMode(sensor_7, INPUT);
}

void setup()
{
  lastCommandTimer = millis();
  initMotors();
  initSensors();
  Serial.begin(115200);
  // Выставляем режим работы WiFi
  WiFi.mode(WIFI_STA);
  // Запускаем протокол ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Получаем состояние отправки
  esp_now_register_recv_cb(OnDataRecv);
  uint8_t baseMac[6];
  
  // Get MAC address of the WiFi station interface
  esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  Serial.print("Station MAC: ");
  for (int i = 0; i < 5; i++) {
    Serial.printf("%02X:", baseMac[i]);
  }
  Serial.printf("%02X\n", baseMac[5]);
}

void loop()
{
  if (millis() - lastCommandTimer > commandTimeout)
  {
    stopMotors();
  }
}