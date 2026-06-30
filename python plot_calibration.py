"""
plot_calibration.py
--------------------
Reads calibration_data.csv (produced by collect_calibration_data.py)
and generates a professional two-panel calibration plot:

  Panel 1 — Encoder angle vs IMU angle over time (the comparison)
  Panel 2 — Drift error (difference) over time

This is the calibration evidence graph your marker is looking for.
"""

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
import numpy as np

INPUT_FILE  = 'calibration_data.csv'
OUTPUT_FILE = 'calibration_comparison.png'

# ── Load Data ─────────────────────────────────────────────────────────────────
try:
    df = pd.read_csv(INPUT_FILE)
except FileNotFoundError:
    print(f"ERROR: '{INPUT_FILE}' not found.")
    print("Run collect_calibration_data.py first to gather serial data.")
    exit()

# Column safety check
required = {'Time_s', 'Encoder_Deg', 'IMU_Deg'}
if not required.issubset(df.columns):
    print(f"ERROR: CSV must contain columns: {required}")
    print(f"Found: {list(df.columns)}")
    exit()

print(f"Loaded {len(df)} samples from '{INPUT_FILE}'.")

# ── Calculate Drift Error ──────────────────────────────────────────────────────
# Shortest angular difference (handles 0°/360° wraparound correctly)
raw_diff = df['Encoder_Deg'] - df['IMU_Deg']
df['Drift_Deg'] = ((raw_diff + 180) % 360) - 180

mean_drift = df['Drift_Deg'].mean()
std_drift  = df['Drift_Deg'].std()
max_drift  = df['Drift_Deg'].abs().max()

# ── Plot ──────────────────────────────────────────────────────────────────────
fig = plt.figure(figsize=(12, 7))
fig.suptitle(
    'Wheel Angle Calibration: Encoder vs IMU (MPU6050)',
    fontsize=14, fontweight='bold', y=0.98
)

gs = gridspec.GridSpec(2, 1, height_ratios=[2.5, 1], hspace=0.35)

# ── Panel 1: Angle comparison ──────────────────────────────────────────────────
ax1 = fig.add_subplot(gs[0])

ax1.plot(
    df['Time_s'], df['Encoder_Deg'],
    label='Encoder angle',
    color='royalblue', linewidth=1.8, zorder=3
)
ax1.plot(
    df['Time_s'], df['IMU_Deg'],
    label='IMU angle  (atan2 calibrated)',
    color='darkorange', linewidth=1.6, linestyle='--', zorder=2
)

# Shade the region between the two lines to make drift visible
ax1.fill_between(
    df['Time_s'], df['Encoder_Deg'], df['IMU_Deg'],
    alpha=0.12, color='red', label='Drift region'
)

ax1.set_ylabel('Wheel Angle (°)', fontsize=11)
ax1.set_ylim(-10, 370)
ax1.set_yticks(range(0, 361, 45))
ax1.set_yticklabels([f'{d}°' for d in range(0, 361, 45)])
ax1.axhline(0,   color='gray', linewidth=0.4, linestyle=':')
ax1.axhline(360, color='gray', linewidth=0.4, linestyle=':')
ax1.grid(True, linestyle='--', alpha=0.4)
ax1.legend(loc='upper left', fontsize=10)
ax1.set_title('Angle tracking — encoder vs IMU', fontsize=11, pad=6)

# Annotate statistics box in panel 1
stats_text = (
    f"Mean drift: {mean_drift:+.1f}°\n"
    f"Std dev:    {std_drift:.1f}°\n"
    f"Max error:  {max_drift:.1f}°"
)
ax1.text(
    0.99, 0.97, stats_text,
    transform=ax1.transAxes,
    fontsize=9, verticalalignment='top', horizontalalignment='right',
    bbox=dict(boxstyle='round,pad=0.4', facecolor='white', alpha=0.75, edgecolor='lightgray')
)

# ── Panel 2: Drift error ───────────────────────────────────────────────────────
ax2 = fig.add_subplot(gs[1], sharex=ax1)

ax2.axhline(0, color='gray', linewidth=0.8, linestyle='--', zorder=1)

# Color code: green if within ±5°, orange if within ±15°, red otherwise
colors = np.where(
    df['Drift_Deg'].abs() <= 5, 'mediumseagreen',
    np.where(df['Drift_Deg'].abs() <= 15, 'darkorange', 'crimson')
)
ax2.bar(df['Time_s'], df['Drift_Deg'], width=0.12, color=colors, alpha=0.75, zorder=2)

# Tolerance band
ax2.axhspan(-5, 5, alpha=0.08, color='green', label='±5° tolerance band')

ax2.set_xlabel('Time (seconds)', fontsize=11)
ax2.set_ylabel('Drift error (°)', fontsize=11)
ax2.set_title('Calibration drift error  (encoder − IMU)', fontsize=11, pad=6)
ax2.grid(True, linestyle='--', alpha=0.3, axis='y')
ax2.legend(loc='upper right', fontsize=9)

# Sync x-axis label hiding on panel 1
plt.setp(ax1.get_xticklabels(), visible=False)

# ── Save ───────────────────────────────────────────────────────────────────────
plt.tight_layout()
plt.savefig(OUTPUT_FILE, dpi=300, bbox_inches='tight')
plt.show()

print(f"\nPlot saved to '{OUTPUT_FILE}'")
print(f"Statistics: mean drift={mean_drift:+.1f}°, std={std_drift:.1f}°, max error={max_drift:.1f}°")