#include <Arduino.h>

// --- Pin Allocations (From your exact soldered hardware) ---
constexpr uint8_t ENCODER_C1 = 19; // Channel A (Soldered to D19)
constexpr uint8_t ENCODER_C2 = 18; // Channel B (Soldered to D18)

// --- Encoder Calibration Constant ---
// You measured exactly 2058 counts for one full 360-degree round
constexpr float TICKS_PER_REV = 2058.0f; 

// --- Global Tracking Variables ---
volatile long encoderTicks = 0;      // Must be volatile for safe ISR modification

// High-speed Hardware Interrupt Service Routine (ISR) tracking quadrature edges
void IRAM_ATTR readEncoder() {
    // Read Channel B's state when Channel A experiences a RISING edge
    int bState = digitalRead(ENCODER_C2);
    if (bState == HIGH) {
        encoderTicks++; // Rotating Clockwise
    } else {
        encoderTicks--; // Rotating Counter-Clockwise
    }
}

void setup() {
    // Initialize serial communication at 115200 baud
    Serial.begin(115200);

    // Configure pre-soldered Encoder Pins with internal pullup resistors
    pinMode(ENCODER_C1, INPUT_PULLUP);
    pinMode(ENCODER_C2, INPUT_PULLUP);
    
    // Attach hardware interrupt to Channel A (C1) on a RISING edge
    attachInterrupt(digitalPinToInterrupt(ENCODER_C1), readEncoder, RISING);

    Serial.println("=============================================");
    Serial.println("  DC Motor Encoder Degree Monitor Initialized  ");
    Serial.println("=============================================");
    Serial.println("Set your wheel to the 0-degree vertical mark now.");
}

void loop() {
    // 1. Snapshot the raw ticks securely to keep calculations thread-safe
    noInterrupts();
    long currentTicks = encoderTicks;
    interrupts();

    // 2. Map raw ticks to standard continuous physical degrees
    float continuousDegrees = (currentTicks / TICKS_PER_REV) * 360.0f;

    // 3. Constrain the readout to a strict 0 to 360 degree window
    // This replicates how a real position servo references its absolute angle
    float boundedDegrees = fmod(continuousDegrees, 360.0f);
    if (boundedDegrees < 0) {
        boundedDegrees += 360.0f; // Keep the angle reading positive
    }

    // --- Diagnostic Display Stream ---
    Serial.print("Raw Ticks: ");
    Serial.print(currentTicks);
    Serial.print("  |  Continuous: ");
    Serial.print(continuousDegrees, 1);
    Serial.print("°  |  Wheel Angle (0-360°): ");
    Serial.print(boundedDegrees, 1);
    Serial.println("°");

    delay(250); // Stream a fresh angle snapshot 4 times per second
}