#include <Wire.h>
#include <Arduino.h>

// --- Pin Allocations ---
constexpr uint8_t ENCODER_C1 = 19; // Channel A (D19)
constexpr uint8_t ENCODER_C2 = 18; // Channel B (D18)
constexpr uint8_t I2C_SDA = 21;    // MPU6050 SDA
constexpr uint8_t I2C_SCL = 22;    // MPU6050 SCL

constexpr uint8_t MPU_ADDR = 0x68;  
constexpr float TICKS_PER_REV = 2058.0f; 
constexpr float IMU_OFFSET = 187.7f; // Your verified functional offset

// --- Global Tracking Variables ---
volatile long encoderTicks = 0;
unsigned long lastSampleTime = 0;

// --- Low-Pass Filter Structure ---
float filteredX = 0.0f;
float filteredY = 0.0f;
float filteredZ = 0.0f;
constexpr float ALPHA = 0.2f; // Your optimized 95% smoothing factor

// Hardware Interrupt Service Routine
void IRAM_ATTR readEncoder() {
    int bState = digitalRead(ENCODER_C2);
    if (bState == HIGH) encoderTicks++;
    else encoderTicks--;
}

void initMPU6050() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); 
    Wire.write(0);    // Wake up MPU6050
    Wire.endTransmission(true);
}

void setup() {
    Serial.begin(115200);

    pinMode(ENCODER_C1, INPUT_PULLUP);
    pinMode(ENCODER_C2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER_C1), readEncoder, RISING);

    Wire.begin(I2C_SDA, I2C_SCL);
    initMPU6050();

    // Warm up filter variables instantly to prevent startup spikes
    delay(500); 
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B); 
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, (uint8_t)6, (uint8_t)true); 
    filteredX = ((Wire.read() << 8) | Wire.read()) / 16384.0f;
    filteredY = ((Wire.read() << 8) | Wire.read()) / 16384.0f;
    filteredZ = ((Wire.read() << 8) | Wire.read()) / 16384.0f;

    // --- CSV HEADER PROFILE FOR EXCEL/MATLAB ---
    // This forms your column labels
    Serial.println("Wheel_Position_Deg,Accel_X_g,Accel_Y_g,Accel_Z_g");
}

void loop() {
    // Collect a clean data point exactly every 100ms (10 Hz collection rate)
    if (millis() - lastSampleTime >= 100) {
        lastSampleTime = millis();

        // 1. Unpack Raw Accumulator Measurements from MPU6050
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x3B); 
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, (uint8_t)6, (uint8_t)true); 

        int16_t rawX = (Wire.read() << 8) | Wire.read();
        int16_t rawY = (Wire.read() << 8) | Wire.read();
        int16_t rawZ = (Wire.read() << 8) | Wire.read();

        float rawAx = rawX / 16384.0f;
        float rawAy = rawY / 16384.0f;
        float rawAz = rawZ / 16384.0f;

        // 2. Apply your Low-Pass Filter to all three axes
        filteredX = (ALPHA * rawAx) + ((1.0f - ALPHA) * filteredX);
        filteredY = (ALPHA * rawAy) + ((1.0f - ALPHA) * filteredY);
        filteredZ = (ALPHA * rawAz) + ((1.0f - ALPHA) * filteredZ);

        // 3. Collect thread-safe wheel position angle from encoder
        noInterrupts();
        long currentTicks = encoderTicks;
        interrupts();
        float wheelPositionDegrees = fmod((currentTicks / TICKS_PER_REV) * 360.0f, 360.0f);
        if (wheelPositionDegrees < 0) wheelPositionDegrees += 360.0f;

        // 4. Print clean CSV Row: WheelPosition, AccelX, AccelY, AccelZ
        Serial.print(wheelPositionDegrees, 1);
        Serial.print(",");
        Serial.print(filteredX, 4);
        Serial.print(",");
        Serial.print(filteredY, 4);
        Serial.print(",");
        Serial.println(filteredZ, 4);
    }
}