#include <Arduino.h>

// --- Pin Allocations (From your exact physical setup) ---
constexpr uint8_t STBY = 27;   // Standby Pin [cite: 845]
constexpr uint8_t PWMA = 33;   // Speed Control Pin [cite: 845]
constexpr uint8_t AIN1 = 26;   // Direction Pin 1 [cite: 845]
constexpr uint8_t AIN2 = 25;   // Direction Pin 2 [cite: 845]

constexpr uint8_t ENCODER_C1 = 19; // Channel A (Soldered to D19) [cite: 830]
constexpr uint8_t ENCODER_C2 = 18; // Channel B (Soldered to D18) [cite: 831]

// --- PWM Configuration ---
constexpr uint32_t MOTOR_FREQ = 500; // 500 Hz for DC Motor Control [cite: 844]
constexpr uint8_t PWM_RES = 8;        // 8-bit resolution (0 - 255) [cite: 847]

// --- Global Tracking Variables ---
volatile long encoderTicks = 0;      // Tracks raw quadrature counts 

// Hardware Interrupt Service Routine tracking encoder edges
void IRAM_ATTR readEncoder() {
    // Check Channel B's state when Channel A goes from LOW to HIGH [cite: 749]
    int bState = digitalRead(ENCODER_C2);
    if (bState == HIGH) {
        encoderTicks++; // Rotating Clockwise [cite: 749]
    } else {
        encoderTicks--; // Rotating Counter-Clockwise [cite: 749]
    }
}

void setup() {
    Serial.begin(115200); [cite: 849]

    // Configure H-Bridge Control Pins
    pinMode(STBY, OUTPUT); [cite: 845]
    pinMode(AIN1, OUTPUT); [cite: 845]
    pinMode(AIN2, OUTPUT); [cite: 845]
    digitalWrite(STBY, HIGH); // Wake up the motor driver [cite: 845]

    // Configure pre-soldered Encoder Pins with internal pullups
    pinMode(ENCODER_C1, INPUT_PULLUP);
    pinMode(ENCODER_C2, INPUT_PULLUP);
    
    // Attach hardware interrupt to Channel A (C1) on a RISING edge
    attachInterrupt(digitalPinToInterrupt(ENCODER_C1), readEncoder, RISING); [cite: 762]

    // Initialize ESP32 PWM (Core 3.0+ syntax)
    ledcAttach(PWMA, MOTOR_FREQ, PWM_RES); [cite: 847]
    ledcWrite(PWMA, 0); // Start stopped [cite: 852]

    // Default direction setup (Forward)
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);

    Serial.println("--- Dual Motor Drive & Encoder Ticks Active ---"); [cite: 853]
    Serial.println("Commands: Send 0-255 for speed. Send 'r' to reset encoder counts to 0.");
}

void loop() {
    // Handle incoming computer commands
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.equalsIgnoreCase("r")) {
            encoderTicks = 0;
            Serial.println(">> Encoder count reset to 0.");
        } else {
            int targetPWM = input.toInt();
            targetPWM = constrain(targetPWM, 0, 255); [cite: 852]
            ledcWrite(PWMA, targetPWM); [cite: 852]
            Serial.print(">> Motor speed adjusted to: ");
            Serial.println(targetPWM);
        }
    }

    // Continuously stream the live positioning ticks to the PC terminal
    Serial.print("Encoder Count: ");
    Serial.println(encoderTicks);
    delay(100); 
}