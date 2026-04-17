#include <esp_now.h>
#include "BTS7960.h"
#include <WiFi.h>

#define RPWM1 25
#define LPWM1 26
#define REN1  27
#define LEN1  14

#define RPWM2 16
#define LPWM2 17
#define REN2  18
#define LEN2  19

#define LED 2
#define TIMEOUT 5000

int speed = 180;

BTS7960 motorController1(LEN1, REN1, LPWM1, RPWM1);
BTS7960 motorController2(LEN2, REN2, LPWM2, RPWM2);

typedef struct struct_message {
  char a[32];
  int x, y;
  bool button;
} struct_message;

struct_message myData;
unsigned long lastRecvTime = 0;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));

  Serial.print("Message: "); Serial.println(myData.a);
  Serial.print("Xaxis: "); Serial.println(myData.x);
  Serial.print("Yaxis: "); Serial.println(myData.y);
  Serial.print("Button: "); Serial.println(myData.button ? "Pressed" : "Released");
  Serial.println();

  digitalWrite(LED, HIGH);
  lastRecvTime = millis();

  // Basic Joystick-based Movement
  if (myData.y == 0 && myData.x!=0) {
    motorController1.TurnLeft(speed);
    motorController2.TurnLeft(speed);
    Serial.println("Forward");
  }
  else if (myData.y==100 &&  myData.x!=100) {
    motorController1.TurnRight(speed);
    motorController2.TurnRight(speed);
    Serial.println("Backward");
  }
  else if (myData.x==0 && myData.y!=0) {
    motorController1.TurnLeft(speed);
    motorController2.TurnRight(speed);
    Serial.println("Left");
  }
  else if (myData.x==100 && myData.y!=100) {
    motorController2.TurnLeft(speed);
    motorController1.TurnRight(speed);
    Serial.println("Right");
  }
  else {
    motorController1.Stop();
    motorController2.Stop();
    Serial.println("Stop");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  motorController1.Stop(); // Ensure motors are off on start
}

void loop() {
    motorController1.Enable();
    motorController2.Enable();

  if (millis() - lastRecvTime > TIMEOUT) {
    digitalWrite(LED, LOW);
    motorController1.Stop(); // Stop bot if no data received
    motorController2.Stop(); // Stop bot if no data received
  }
}
