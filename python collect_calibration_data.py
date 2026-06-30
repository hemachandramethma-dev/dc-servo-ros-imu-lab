"""
collect_calibration_data.py
----------------------------
Reads serial output from wheel_angle_measurement.ino,
parses encoder angle and IMU angle from each line,
and saves them to calibration_data.csv.

Expected Arduino serial line format:
  "Encoder Stance: 45.2° | Smoothed IMU Stance: 43.8°"

Run BEFORE plot_calibration.py.
"""

import serial
import csv
import re
import time

# ── Configuration ────────────────────────────────────────────────────────────
SERIAL_PORT  = 'COM11'       # Change to your port (e.g. '/dev/ttyUSB0' on Linux)
BAUD_RATE    = 115200
OUTPUT_FILE  = 'calibration_data.csv'
# ─────────────────────────────────────────────────────────────────────────────

# Regex to pull the two angles out of each Arduino line
# Matches: "Encoder Stance: 45.2° | Smoothed IMU Stance: 43.8°"
LINE_PATTERN = re.compile(
    r'Encoder Stance:\s*([\d.]+).*?Smoothed IMU Stance:\s*([\d.]+)'
)

print(f"Connecting to ESP32 on {SERIAL_PORT} at {BAUD_RATE} baud...")

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
    time.sleep(2)  # Let ESP32 reboot after serial connection opens
    ser.reset_input_buffer()
    print(f"Connected! Logging to '{OUTPUT_FILE}'.")
    print("Spin the wheel slowly through at least 2 full turns.")
    print("Press Ctrl+C when done.\n")

    with open(OUTPUT_FILE, mode='w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(['Time_s', 'Encoder_Deg', 'IMU_Deg'])  # CSV header

        start_time = time.time()
        sample_count = 0

        while True:
            raw = ser.readline().decode('utf-8', errors='ignore').strip()

            if not raw:
                continue

            # Pass through any calibration-event messages to the console
            if 'SYNC CALIBRATION' in raw:
                print(f"  [CAL EVENT] {raw}")
                continue

            # Try to parse a data line
            match = LINE_PATTERN.search(raw)
            if match:
                encoder_deg = float(match.group(1))
                imu_deg     = float(match.group(2))
                elapsed     = round(time.time() - start_time, 2)

                writer.writerow([elapsed, encoder_deg, imu_deg])
                f.flush()

                sample_count += 1
                print(f"  t={elapsed:6.1f}s  Encoder={encoder_deg:6.1f}°  IMU={imu_deg:6.1f}°")

except KeyboardInterrupt:
    print(f"\nStopped. {sample_count} samples saved to '{OUTPUT_FILE}'.")
    print("Now run:  python plot_calibration.py")

except Exception as e:
    print(f"\nError: {e}")
    print("Check that SERIAL_PORT is correct and the Arduino IDE Serial Monitor is closed.")