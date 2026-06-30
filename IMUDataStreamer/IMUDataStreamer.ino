#include <Wire.h>
#include <Arduino.h>

// --- Pin Allocations ---
constexpr uint8_t I2C_SDA = 21;    
constexpr uint8_t I2C_SCL = 22;    
constexpr uint8_t MPU_ADDR = 0x68;  

unsigned long lastSampleTime = 0;

// --- Low-Pass Filter Variables ---
float filteredX = 0.0f;
float filteredY = 0.0f;
float filteredZ = 0.0f;

// Smoothing factor (Lower value = smoother curves, but introduces minor delay)
constexpr float ALPHA = 0.05f; 

void initMPU6050() {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); 
    Wire.write(0);    
    Wire.endTransmission(true);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL); 
    initMPU6050();
    
    // Labels for the Serial Plotter legend window
    Serial.println("Filtered_X,Filtered_Y,Filtered_Z"); 
}

void loop() {
    // Read at a fast 50 Hz rate (every 20ms) for responsive filtering
    if (millis() - lastSampleTime >= 20) {
        lastSampleTime = millis();

        // Fetch accelerometer registers
        Wire.beginTransmission(MPU_ADDR);
        Wire.write(0x3B); 
        Wire.endTransmission(false);
        Wire.requestFrom(MPU_ADDR, (uint8_t)6, (uint8_t)true); 

        int16_t rawX = (Wire.read() << 8) | Wire.read(); 
        int16_t rawY = (Wire.read() << 8) | Wire.read(); 
        int16_t rawZ = (Wire.read() << 8) | Wire.read(); 

        // Convert to G-units (+/- 2g scale)
        float rawAx = rawX / 16384.0f;
        float rawAy = rawY / 16384.0f;
        float rawAz = rawZ / 16384.0f;

        // --- Apply Low-Pass Filter Math ---
        filteredX = (ALPHA * rawAx) + ((1.0f - ALPHA) * filteredX);
        filteredY = (ALPHA * rawAy) + ((1.0f - ALPHA) * filteredY);
        filteredZ = (ALPHA * rawAz) + ((1.0f - ALPHA) * filteredZ);

        // Print the filtered clean values to the plotter
        Serial.print(filteredX, 3);
        Serial.print(",");
        Serial.print(filteredY, 3);
        Serial.print(",");
        Serial.println(filteredZ, 3);
    }
}