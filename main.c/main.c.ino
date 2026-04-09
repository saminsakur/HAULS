#include <Servo.h>

/*
  Servo (1) - claw            - 3
  Servo (2) - Wrist rotate    - 5
  Servo (3) - Wrist Pitch     - 6
  Servo (4) - Elbow           - 9
  Servo (5) - Shoulder        - 10
  Servo (6) - Base            - 11    
  */

// Define a structure for servo limits to keep things clean
struct ServoConfig {
  int pin;
  int minAngle;
  int maxAngle;
  int restAngle;
};

// --- CONFIGURATION AREA: Adjust these based on your specific build ---
// Order: {Pin, Min, Max, Initial/Rest}
ServoConfig config[6] = {
  {3, 90, 140, 90},   // 3: Gripper (Prevent motor strain when closed)
  {5, 0,  180, 90},  // 5: Wrist Rotate
  {6,  0,  180, 90},  // 6: Wrist Pitch
  {9,  20, 160, 90},  // 9: Elbow
  {10,  45, 135, 90},  // 10: Shoulder (Avoid crashing into base)
  {11,  10, 170, 90}  // 11: Base (Wide range)
};

Servo servos[6];
int targetAngles[6];
float currentAngles[6];
float smoothingFactor = 0.08; // Increase for speed, decrease for smoothness

void setup() {
  Serial.begin(9600);
  
  for(int i=0; i<6; i++) {
    servos[i].attach(config[i].pin);
    
    // Initialize both target and current at the designated "Rest" position
    targetAngles[i] = config[i].restAngle;
    currentAngles[i] = config[i].restAngle;
    servos[i].write(config[i].restAngle);
  }
}

void loop() {
  // 1. READ SERIAL COMMANDS
  if (Serial.available() > 0) {
    int index = Serial.parseInt(); 
    int angle = Serial.parseInt();
    
    if (index >= 0 && index < 6) {
      // Apply the constraints defined in our list
      targetAngles[index] = constrain(angle, config[index].minAngle, config[index].maxAngle);
    }
  }

  // 2. SMOOTH MOVEMENT LOGIC
  for(int i=0; i<6; i++) {
    float diff = targetAngles[i] - currentAngles[i];
    
    // Only move if the difference is significant to prevent jitter
    if (abs(diff) > 0.1) {
      currentAngles[i] += diff * smoothingFactor; 
      servos[i].write((int)currentAngles[i]);
    }
  }
  
  delay(15); 
}