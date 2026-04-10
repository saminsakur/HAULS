#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>

const char* ssid = "Samin Saba";
const char* password = "samin2606";

AsyncWebServer server(80);

// Servo Configuration 

struct ServoConfig {
  int pin;
  int minAngle;
  int maxAngle;
  int restAngle;
  float currentAngle;
  int targetAngle;
  float smoothingFactor;
  Servo servo;
};

ServoConfig servos[8] = {
  {13, 90, 140, 90, 90.0, 90, 0.08},   // Gripper 
  {12, 0, 180, 90, 90.0, 90, 0.08},    // Wrist Rotate
  {14, 0, 180, 90, 90.0, 90, 0.08},    // Wrist Pitch
  {27, 20, 160, 90, 90.0, 90, 0.08},   // Elbow
  {26, 45, 135, 90, 90.0, 90, 0.08},   // Shoulder
  {25, 10, 170, 90, 90.0, 90, 0.08},    // Base
  {33, 10, 170, 90, 90.0, 90, 0.08},    // Camera Pan
  {32, 10, 170, 90, 90.0, 90, 0.08}    // Camera Tilt
};

const char* html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; background: #222; color: white; }
        .container { max-width: 400px; margin: auto; padding: 20px; }
        input[type=range] { width: 100%; margin: 15px 0; }
        h2 { color: #00d4ff; }
        .section { border: 1px solid #444; padding: 10px; margin-bottom: 20px; border-radius: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Robot Arm Control</h2>
        <div class="section">
            <h3>Arm Joints</h3>
            <div id="arm-sliders"></div>
        </div>
        <div class="section">
            <h3>Camera Pan/Tilt</h3>
            <div id="cam-sliders"></div>
        </div>
    </div>
    <script>
        function createSlider(i, label, parentId) {
            let div = document.createElement('div');
            div.innerHTML = `<label>${label}</label>
                             <input type="range" min="0" max="180" value="90" 
                             oninput="sendValue(${i}, this.value)">`;
            document.getElementById(parentId).appendChild(div);
        }

        const labels = ["Base", "Shoulder", "Elbow", "Wrist P", "Wrist R", "Gripper", "Cam Pan", "Cam Tilt"];
        for(let i=0; i<6; i++) createSlider(i, labels[i], 'arm-sliders');
        for(let i=6; i<8; i++) createSlider(i, labels[i], 'cam-sliders');

        let lastSend = 0;
        function sendValue(id, val) {
            let now = Date.now();
            if (now - lastSend > 50) { // Throttling for stability
                fetch(`/set?id=${id}&val=${val}`);
                lastSend = now;
            }
        }
    </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize Servos
  for (int i = 0; i < 8; i++) {
    servos[i].servo.attach(servos[i].pin);
    servos[i].servo.write(servos[i].restAngle);
    servos[i].currentAngle = servos[i].restAngle;
    servos[i].targetAngle = servos[i].restAngle;
  }

  // Web Server Routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", html);
  });

  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("id") && request->hasParam("val")) {
      int id = request->getParam("id")->value().toInt();
      int val = request->getParam("val")->value().toInt();
      if (id >= 0 && id < 8) {
        val = constrain(val, servos[id].minAngle, servos[id].maxAngle);
        servos[id].targetAngle = val;
      }
    }
    request->send(200);
  });

  server.begin();
}

void loop() {
  // Smoothly update servo positions
  for (int i = 0; i < 8; i++) {
    if (servos[i].currentAngle != servos[i].targetAngle) {
      servos[i].currentAngle += (servos[i].targetAngle - servos[i].currentAngle) * servos[i].smoothingFactor;
      servos[i].servo.write((int)servos[i].currentAngle);
    }
  }
}