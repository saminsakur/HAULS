#include <esp_now.h>
#include <WiFi.h>

const int xPin = 34;
const int yPin = 35;
const int buttonPin = 32;
const int potPin = 33;

#define LED 2

uint8_t broadcastAddress[] = {0x68, 0x25, 0xDD, 0x32, 0x82, 0x5C};

typedef struct struct_message {
  char a[32];
  int x, y, pot;
  bool button;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

bool isConnected = false;
unsigned long lastSendTime = 0;
unsigned long timeoutDuration = 2000; // 2 seconds timeout

void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    isConnected = true;
    lastSendTime = millis(); // reset timeout
    // Serial.println("✅ Delivery Success");
  } else {
    // Serial.println("❌ Delivery Fail");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  bool buttonPressed = digitalRead(buttonPin) == LOW;
  int potValue = analogRead(potPin);

  int mappedX = map(xValue, 0, 4095, 0, 100);
  int mappedY = map(yValue, 0, 4095, 0, 100);
  int mappedPot = map(potValue, 0, 4095, 0, 100);

  myData.x = mappedX;
  myData.y = mappedY;
  myData.button = buttonPressed;
  myData.pot = mappedPot;
  strcpy(myData.a, "Joystick Data");

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.printf("📤 Sent: X=%d, Y=%d, Pot=%d, Btn=%s\n", mappedX, mappedY, mappedPot, buttonPressed ? "Pressed" : "Released");
  } else {
    Serial.println("🚫 Error sending the data");
  }

  // ⚠️ Simulate connection timeout (if no successful sends recently)
  if (millis() - lastSendTime > timeoutDuration) {
    isConnected = false;
  }

  digitalWrite(LED, isConnected ? HIGH : LOW);

  delay(200);
}