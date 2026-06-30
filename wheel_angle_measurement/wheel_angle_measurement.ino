#include <Wire.h>
#include <Arduino.h>

// --- Pin Allocations ---
constexpr uint8_t ENCODER_C1 = 19; // Channel A (Soldered to D19) [cite: 1160]
constexpr uint8_t ENCODER_C2 = 18; // Channel B (Soldered to D18) [cite: 1161]
constexpr uint8_t I2C_SDA = 21;    // MPU6050 SDA [cite: 1432]
constexpr uint8_t I2C_SCL = 22;    // MPU6050 SCL [cite: 1432]

constexpr uint8_t MPU_ADDR = 0x68;  // [cite: 1451]
constexpr float TICKS_PER_REV = 2058.0f; // [cite: 1346]

// --- Calibration Offset ---
// Use the exact base reading you got at true 0-degrees.
constexpr float IMU_OFFSET = 445.6f; 

// --- Global Variables ---
volatile long encoderTicks = 0;
unsigned long lastCalibrationTime = 0;

// --- Low-Pass Filter Variables ---
float filteredX = 0.0f;
float filteredY = 0.0f;
constexpr float ALPHA = 0.05f; // 95% noise removal [cite: 1487]

// Hardware Interrupt Service Routine tracking encoder position [cite: 1298]
void IRAM_ATTR readEncoder() {
    int bState = digitalRead(ENCODER_C2);
    if (bState == HIGH) encoderTicks++;
    else encoderTicks--;
}

void initMPU6050() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); 
    Wire.write(0);    // Wake up the sensor [cite: 1475]
    Wire.endTransmission(true);
}

void setup() {
    Serial.begin(115200);

    pinMode(ENCODER_C1, INPUT_PULLUP);
    pinMode(ENCODER_C2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_C1), readEncoder, RISING);

    Wire.begin(I2C_SDA, I2C_SCL);
    initMPU6050();

    Serial.println("=================================================");
    Serial.println("   WARMED IMU Gravity Calibration Engine         ");
    Serial.println("=================================================");
    Serial.println("Warming up filter sensors...");

    // 🔄 FORCE FILTER WARMUP BEFORE MAIN LOOP RUNS
    // Read the sensor directly to give our filter a real baseline starting posture
    delay(500); 
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); 
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, (uint8_t)4, (uint8_t)true); 

    int16_t rawX = (Wire.read() << 8) | Wire.read();
    int16_t rawY = (Wire.read() << 8) | Wire.read();

    // Set initial filter variables directly to current gravity state
    filteredX = rawX / 16384.0f;
    filteredY = rawY / 16384.0f;

    Serial.println("Filter initialized! System Active.");
}

void loop() {
    // Perform processing at a stable 10 Hz rate
    if (millis() - lastCalibrationTime >= 100) {
        lastCalibrationTime = millis();

        // Fetch raw data from MPU6050 [cite: 1475]
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x3B); 
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, (uint8_t)4, (uint8_t)true); 

        int16_t rawX = (Wire.read() << 8) | Wire.read();
        int16_t rawY = (Wire.read() << 8) | Wire.read();

        float rawAx = rawX / 16384.0f;
        float rawAy = rawY / 16384.0f;

        // Apply our Low-Pass Filter [cite: 1486]
        filteredX = (ALPHA * rawAx) + ((1.0f - ALPHA) * filteredX);
        filteredY = (ALPHA * rawAy) + ((1.0f - ALPHA) * filteredY);

        // Compute absolute gravity angle using the clean vectors 
        float rawImuAngle = atan2(filteredY, filteredX) * 180.0f / M_PI;
        if (rawImuAngle < 0) rawImuAngle += 360.0f; 

        // Apply your custom offset shift calibration [cite: 1539]
        float imuAngle = rawImuAngle - IMU_OFFSET;
        if (imuAngle < 0) imuAngle += 360.0f; 

        // Collect thread-safe encoder degrees data profile
        noInterrupts();
        long currentTicks = encoderTicks;
        interrupts();
        float encoderAngle = fmod((currentTicks / TICKS_PER_REV) * 360.0f, 360.0f);
        if (encoderAngle < 0) encoderAngle += 360.0f;

        // Run Calibration Comparison Engine [cite: 1557]
        float driftError = abs(encoderAngle - imuAngle);
        if (driftError > 180.0f) driftError = 360.0f - driftError; 

        // Auto-correct if tracking ticks slip over time [cite: 1491]
        // This won't glitch at startup anymore because the filter is pre-warmed!
        if (driftError > 4.0f && abs(filteredX) > 0.05f) { 
            long correctedTicks = (imuAngle / 360.0f) * TICKS_PER_REV;
            
            noInterrupts();
            encoderTicks = correctedTicks;
            interrupts();
            
            Serial.print(">> [SYNC CALIBRATION] Reset Wheel Ticks to match IMU. Adjusted: ");
            Serial.print(driftError, 1);
            Serial.println("°");
        }

        // Print aligned states
        Serial.print("Encoder Stance: "); Serial.print(encoderAngle, 1);
        Serial.print("° | Smoothed IMU Stance: "); Serial.print(imuAngle, 1);
        Serial.println("°");
    }
}