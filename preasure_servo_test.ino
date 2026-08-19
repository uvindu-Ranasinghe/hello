#include <Arduino.h>
#include <Servo.h>

// Hardware Pin Definitions
const int FSR_PIN = A0; 
const int SERVO_PIN = 9;
const int LED_PIN = 13;

const int SERVO_REST_ANGLE = 0;       
const int SERVO_TRIGGER_ANGLE = 90;   

// Circuit Parameters
const float VCC = 5.0;
const float ADC_RESOLUTION = 1023.0;
const float R_DIV = 100000.0;         // 100k Ohm pulldown resistor for ultra-sensitivity

const float ULTRA_LIGHT_TRIGGER_RESISTANCE = 1000000.0; 
const int ADC_THRESHOLD_ABOVE_BASELINE = 3;             

// Objects & State Variables
Servo gripServo;
int baselineADC = 0;       // Calibration baseline
bool isGripped = false;    // Tracks current grip state (prevents servo jitter)

//
void setup() {
  Serial.begin(9600);

  // Initialize LED indicator
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Attach and position Servo to Rest
  gripServo.attach(SERVO_PIN);
  gripServo.write(SERVO_REST_ANGLE);

  while (!Serial) { ; } 
  Serial.println("  Test Type SHit ");

  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(FSR_PIN);
    delay(50);
  }
  baselineADC = sum / 20;

  Serial.print("Baseline ADC Calibrated: ");
  Serial.println(baselineADC);
  Serial.println("System Ready! Waiting for contact...");
  Serial.println("--------------------------------------------------");
}

void loop() {
  int rawADC = analogRead(FSR_PIN);
  float voltage = (float)rawADC * (VCC / ADC_RESOLUTION);

  // Measure delta above static baseline
  int adcDelta = rawADC - baselineADC;

  // Determine if active contact is detected
  bool currentGripState = false;

  if (adcDelta > ADC_THRESHOLD_ABOVE_BASELINE && rawADC > 2) {
    float fsrResistance = R_DIV * ((VCC / voltage) - 1.0);

    // If resistance drops below 1M-Ohm, contact is confirmed
    if (fsrResistance < ULTRA_LIGHT_TRIGGER_RESISTANCE) {
      currentGripState = true;
    }
  }

  // State Machine: Trigger Servo on State Change
  if (currentGripState && !isGripped) {
    // STATE CHANGE: Released --> Gripped
    isGripped = true;
    digitalWrite(LED_PIN, HIGH);
    
    // Actuate Servo
    gripServo.write(SERVO_TRIGGER_ANGLE);
    
    Serial.print("[ACTION] Glass Gripped! Servo Moved to ");
    Serial.print(SERVO_TRIGGER_ANGLE);
    Serial.println(" degrees.");
  } 
  else if (!currentGripState && isGripped) {
    // STATE CHANGE: Gripped --> Released
    isGripped = false;
    digitalWrite(LED_PIN, LOW);
    
    // Retract Servo
    gripServo.write(SERVO_REST_ANGLE);
    
    Serial.print("[ACTION] Glass Released. Servo Returned to ");
    Serial.print(SERVO_REST_ANGLE);
    Serial.println(" degrees.");
  }

  delay(50); // Fast 50ms polling loop for instant reaction time
}