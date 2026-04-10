#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

#include <format>
#include <string>

const char *ssid = "HAULS";
const char *password = "tomato";

WebServer server(80);
bool serverStarted = false;
unsigned long startTimer = 0;
// Servo Configuration

struct Servoservos
{
  int pin;
  int minAngle;
  int maxAngle;
  int restAngle;
  float currentAngle;
  int targetAngle;
  float smoothingFactor;
  Servo servo;
};

Servoservos servos[8] = {
    {13, 90, 150, 90, 90.0, 90, 0.08}, // Gripper
    {12, 0, 180, 90, 90.0, 90, 0.08},  // Wrist Rotate
    {14, 0, 180, 30, 30.0, 30, 0.08},  // Wrist Pitch
    {27, 20, 180, 180, 180.0, 180, 0.08}, // Elbow
    {26, 45, 180, 180, 180.0, 180, 0.08}, // Shoulder
    {25, 10, 170, 90, 90.0, 90, 0.08}, // Base
    {5, 10, 170, 90, 90.0, 90, 0.08}, // Camera Pan
    {18, 10, 170, 90, 90.0, 90, 0.08}  // Camera Tilt
};
std::string html = R"rawliteral(
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
        function createSlider(i, label, parentId, restValue=90) {
            let div = document.createElement('div');
            div.innerHTML = `<label>${label}</label>
                             <input type="range" min="0" max="180" value="${restValue}" 
                             oninput="sendValue(${i}, this.value)">`;
            document.getElementById(parentId).appendChild(div);
        }

        const labels = ["Gripper", "Wrist Rotate", "Wrist Pitch", "Elbow", "Shoulder", "Base", "Cam Pan", "Cam Tilt"];
        for(let i=0; i<6; i++) createSlider(i, labels[i], 'arm-sliders', %d);
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

std::string index_html = std::format(html, servos[0].restAngle, servos[1].restAngle, servos[2].restAngle, 
                                        servos[3].restAngle, servos[4].restAngle, servos[5].restAngle);
void handleRoot() 
{
  server.send(200, "text/html", index_html);
}

void handleSet() 
{
  if (server.hasArg("id") && server.hasArg("val")) {
    int id = server.arg("id").toInt();
    int val = server.arg("val").toInt();
    if (id >= 0 && id < 8)
    {
      servos[id].targetAngle = constrain(val, servos[id].minAngle, servos[id].maxAngle);
      Serial.printf("Received command: Servo %d -> %d\n", id, val);
    }else{
      Serial.println("Invalid ID received: " + String(id));
      server.send(400, "text/plain", "Invalid ID");
      return;
    }
  }
  server.send(200, "text/plain", "OK");
}
void setup()
{
  Serial.begin(115200);
  delay(1000); // Give the hardware a second to stabilize

  // 1. Initialize the servos first
  for (int i = 0; i < 8; i++)
  {
    servos[i].servo.setPeriodHertz(50);               // Standard 50Hz for servos
    servos[i].servo.attach(servos[i].pin, 500, 2400); // Standard pulse width
    servos[i].servo.write(servos[i].restAngle);
  }

  // 2. Setup AP Mode
  Serial.println("Setting up Access Point...");
  if (WiFi.softAP(ssid, password)) {
    Serial.println("AP Started successfully...");
  } else {
    Serial.println("AP Failed to start!");
  }
  Serial.println("AP Started. IP:" + WiFi.softAPIP().toString() + " SSID:" + WiFi.softAPSSID() + "\n");

  // Give the TCPIP stack a moment to lock core functionality
  delay(2000);

  // 3. Define Routes
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
  Serial.println("Server started and ready to accept commands!");
  }

void loop()
{
  unsigned long now = millis();

  server.handleClient();
  
  // Run the smoothing logic every 20ms (consistent timing)
    static unsigned long lastMoveTime = 0;
    if (now - lastMoveTime >= 20)
    {
      lastMoveTime = now;
      for (int i = 0; i < 8; i++)
      {
        float diff = servos[i].targetAngle - servos[i].currentAngle;
        if (abs(diff) > 0.1)
        {
          servos[i].currentAngle += diff * servos[i].smoothingFactor;
          servos[i].servo.write((int)servos[i].currentAngle);
          Serial.printf("Servo %d moving to %.1f (target: %d)\n", i, servos[i].currentAngle, servos[i].targetAngle);
        }
      }
    }

  // yield() lets the WiFi/TCP stack handle background tasks
  yield();
}