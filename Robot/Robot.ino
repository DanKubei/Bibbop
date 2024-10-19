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
#define max_pwm 38000

#define commandTimeout 10

float sensorsPos[8] = {4,3,2,1,-1,-2,-3,-4};

typedef struct struct_message {
    int code;
} struct_message;

struct_message myData;
uint8_t newMACAddress[] = {0x30, 0xAE, 0xA4, 0x07, 0x0D, 0x66};

const int frequency = 30;
const int resolution = 16;

long lastCommandTimer;
bool autoMode = false;

void OnDataRecv(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len) {
  memcpy(&myData, data, sizeof(myData));
  switch(myData.code)
  {
    case 0:
      autoMode = true;
    break;
    case 1:
      set_motors_speed(max_pwm, max_pwm, false, false);
    break;
    case 2:
      set_motors_speed(max_pwm, max_pwm, true, true);
    break;
    case 3:
      set_motors_speed(max_pwm, max_pwm, false, true);
    break;
    case 4:
      set_motors_speed(max_pwm, max_pwm, true, false);
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
  ledcWrite(motor_right_pwm, right_motor_speed);
  ledcWrite(motor_left_pwm, left_motor_speed);
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

int err_arr[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int err_p = -1;

float I()
{
  int error_sum = 0;
  for (int i = 0; i < 10; i++) 
  {
    error_sum += err_arr[i];
  }
  return error_sum / 10;
}

float D(float error)
{
  err_p = (err_p + 1) % 10;
  err_arr[err_p] = error;
  return err_arr[err_p] - err_arr[(err_p+11) % 10];
}

float PID(float setPoint, float input, float Pk, float Ik, float Dk)
{
  float error = P(setPoint, input);
  return error * Pk + D(error) * Dk + I() * Ik;
}

void autoMove()
{
  float setPoint = 0;
  float input = 0;
  for (int i = 0; i < 8; i++)
  {
    if (!get_sensor(i))
    {
      input += sensorsPos[i];
    }
  }
  move(PID(setPoint, input, 325, 0, 50));
}

void move(float pid)
{
  int medianSpeed = (min_pwm + max_pwm) / 2;
  float right_motor_speed = 0, left_motor_speed = 0;
  bool right_motor_reverse = false;
  bool left_motor_reverse = false;
  if (medianSpeed - pid * 3 < min_pwm)
  {
    right_motor_speed = 2 * min_pwm - medianSpeed + pid * 3;
    right_motor_reverse = true;
  }
  else
  {
    if(pid < 0)
    {
      right_motor_speed = medianSpeed - pid * 3;
    }
    else
    {
      right_motor_speed = medianSpeed - pid;
    }
  }
  if (medianSpeed + pid * 3 < min_pwm)
  {
    left_motor_speed = 2 * min_pwm - medianSpeed - pid * 3;
    left_motor_reverse = true;
  }
  else
  {
    if(pid > 0)
    {
      left_motor_speed = medianSpeed + pid * 3;
    }
    else
    {
      left_motor_speed = medianSpeed + pid;
    }
  }
  left_motor_speed = clamp(left_motor_speed, -max_pwm, max_pwm);
  right_motor_speed = clamp(right_motor_speed, -max_pwm, max_pwm);
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
    autoMode = false;
  }
  if (autoMode)
  {
    autoMove();
  }
}