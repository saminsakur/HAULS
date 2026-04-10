#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

const char *ssid = "HAULS";
const char *password = "tomato";

WebServer server(80);
bool serverStarted = false;
unsigned long startTimer = 0;
// Servo Configuration

struct ServoConfig
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

ServoConfig servos[8] = {
    {13, 90, 150, 90, 90.0, 90, 0.08}, // Gripper
    {12, 0, 180, 30, 30.0, 30, 0.08},  // Wrist Rotate
    {14, 0, 180, 90, 90.0, 90, 0.08},  // Wrist Pitch
    {27, 20, 180, 180, 180.0, 180, 0.08}, // Elbow
    {26, 45, 180, 180, 180.0, 180, 0.08}, // Shoulder
    {25, 10, 170, 90, 90.0, 90, 0.08}, // Base
    {5, 10, 170, 90, 90.0, 90, 0.08}, // Camera Pan
    {18, 10, 170, 90, 90.0, 90, 0.08}  // Camera Tilt
};
// The HTML uses a JavaScript array to store the initial values
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body { font-family: sans-serif; text-align: center; background: #1a1a1a; color: #eee; padding: 20px; }
  .section { background: #2d2d2d; border-radius: 15px; padding: 15px; margin-bottom: 20px; }
  input[type=range] { width: 80%; height: 20px; margin: 15px 0; accent-color: #00d4ff; }
  h2 { color: #00d4ff; }
  .val-label { font-weight: bold; color: #00d4ff; }
</style>
</head>
<body>
  <h2>Robot Arm & Camera</h2>
    <div class="section">
      <h3>Arm Control</h3>
      <div id="arm"></div>
    </div>
  <div class="section">
    <h3>Camera Pan/Tilt</h3>
    <div id="cam"></div>
  </div>
  <script>
    // These values are injected by the ESP32
    const labels = ["Gripper", "Wrist P", "Wrist R", "Elbow", "Shoulder", "Base", "Cam Pan", "Cam Tilt"];

    function createS(i, label, pId) {
    const parent = document.getElementById(pId);
    if(!parent) return;

      let d = document.createElement('div');
      d.innerHTML = `<label>${label}: <span class="val-label" id="v${i}">${startVals[i]}</span>°</label><br>
                    <input type="range" min="0" max="180" value="${startVals[i]}" oninput="send(${i},this.value)">`;
      parent.appendChild(d);
    }

    // This functions runs on page load to create sliders for each servo. It uses the startVals array for initial positions and labels for display.
    function initSliders(){
    for(let i=0; i<6; i++) createS(i, labels[i], 'arm');
    for(let i=6; i<8; i++) createS(i, labels[i], 'cam');
}

    let last = 0;
    function send(id, val) {
      document.getElementById('v'+id).innerText = val; // Update the UI number
      if (Date.now() - last > 50) {
        fetch(`/set?id=${id}&val=${val}`);
        last = Date.now();
      }
    }
      window.onload = initSliders;
  </script>
  </body>
</html>
)rawliteral";

void handleRoot() 
{
  // We create a string defines the JavaScript array with the current rest angles of the servos, which will be injected into the HTML template. This allows the sliders to start at the correct positions when the page loads.
  String scriptInjection = "<script> const startVals = [";
  for (int i =0; i < 8; i++){
    scriptInjection += String(servos[i].restAngle);
    if (i<7) scriptInjection += ", ";
  }
  scriptInjection += "]; </script>";

  String full_html = scriptInjection + String(index_html);
  // Send the script part first, then the rest of the HTML
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/html", full_html);
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