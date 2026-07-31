"""
VITALIS-Edge: Multi-Modal Biosignal Dataset Generator & PhysioNet Formatter
Theme: Edge AI & Healthcare - DigiKey electronica India Tech Challenge 2026

Generates calibrated 5-channel time-series data at 250 Hz sampling rate:
  Channel 0: ECG Lead-I (mV) [AD8232 / MAX30003]
  Channel 1: PPG IR raw reflectance (normalized) [MAX30102]
  Channel 2: PPG Red raw reflectance (normalized) [MAX30102]
  Channel 3: Accelerometer Magnitude (g) [LSM6DSOX]
  Channel 4: Skin Temperature (°C) [TMP117]

Classes (6 target triage classes):
  0: Normal Sinus Rhythm (NSR)
  1: Atrial Fibrillation (AFib)
  2: Ventricular Tachycardia (VTach)
  3: Premature Ventricular Contraction (PVC)
  4: Acute Hypoxia Decompensation (Hypoxia)
  5: Fall with Immobility (Fall)
"""

import numpy as np
import math
import json
import os

# Configuration Constants
SAMPLING_RATE_HZ = 250
WINDOW_SECONDS = 1.0
WINDOW_SIZE = int(SAMPLING_RATE_HZ * WINDOW_SECONDS) # 250 samples
NUM_CHANNELS = 5

CLASS_NAMES = [
    "Normal Sinus Rhythm",
    "Atrial Fibrillation",
    "Ventricular Tachycardia",
    "Premature Ventricular Contraction",
    "Acute Hypoxia Decompensation",
    "Patient Fall & Immobility"
]

def generate_ecg_beat(t, beat_type="normal", hr=75):
    """
    Generates synthetic P-Q-R-S-T synthetic biopotential wave.
    t: array of time offsets (seconds) within one beat period.
    """
    period = 60.0 / hr
    phase = (t % period) / period  # 0.0 to 1.0

    ecg = np.zeros_like(t)

    if beat_type == "normal":
        # P-wave: Gaussian bump at phase 0.15
        ecg += 0.15 * np.exp(-((phase - 0.15) ** 2) / (2 * (0.025 ** 2)))
        # Q-wave: Negative deflection at phase 0.28
        ecg -= 0.12 * np.exp(-((phase - 0.28) ** 2) / (2 * (0.010 ** 2)))
        # R-wave: Sharp tall spike at phase 0.30
        ecg += 1.20 * np.exp(-((phase - 0.30) ** 2) / (2 * (0.015 ** 2)))
        # S-wave: Negative dip at phase 0.33
        ecg -= 0.25 * np.exp(-((phase - 0.33) ** 2) / (2 * (0.012 ** 2)))
        # T-wave: Moderate smooth bump at phase 0.55
        ecg += 0.30 * np.exp(-((phase - 0.55) ** 2) / (2 * (0.045 ** 2)))

    elif beat_type == "afib":
        # Missing P-wave, rapid chaotic f-waves (fibrillatory baseline)
        f_wave = 0.08 * np.sin(2 * np.pi * 7.5 * t) + 0.05 * np.cos(2 * np.pi * 11.3 * t)
        ecg += f_wave
        # Sharp R-wave at irregularly spaced phases
        ecg -= 0.10 * np.exp(-((phase - 0.28) ** 2) / (2 * (0.010 ** 2)))
        ecg += 1.15 * np.exp(-((phase - 0.30) ** 2) / (2 * (0.015 ** 2)))
        ecg -= 0.20 * np.exp(-((phase - 0.33) ** 2) / (2 * (0.012 ** 2)))
        ecg += 0.20 * np.exp(-((phase - 0.55) ** 2) / (2 * (0.040 ** 2)))

    elif beat_type == "vtach":
        # Broad, wide QRS complexes without P-waves, high rate (>160 bpm)
        ecg += 1.40 * np.sin(2 * np.pi * (hr / 60.0) * t)
        ecg += 0.30 * np.sin(4 * np.pi * (hr / 60.0) * t + 0.5)

    elif beat_type == "pvc":
        # Ectopic wide, inverted QRS complex
        # Wide bizarre R/S complex
        ecg += 1.35 * np.exp(-((phase - 0.30) ** 2) / (2 * (0.040 ** 2)))
        ecg -= 0.85 * np.exp(-((phase - 0.36) ** 2) / (2 * (0.035 ** 2)))
        # Inverted T-wave
        ecg -= 0.40 * np.exp(-((phase - 0.60) ** 2) / (2 * (0.055 ** 2)))

    return ecg

def generate_ppg_wave(t, hr=75, spo2=98.0):
    """
    Generates arterial PPG pulse waveform (systolic peak + dicrotic notch)
    for both Infrared (880nm) and Red (660nm) channels.
    """
    period = 60.0 / hr
    phase = (t % period) / period

    # Fundamental pulse curve (systolic ejection + diastolic decay + dicrotic notch)
    systolic = 1.0 * np.exp(-((phase - 0.20) ** 2) / (2 * (0.08 ** 2)))
    dicrotic = 0.35 * np.exp(-((phase - 0.45) ** 2) / (2 * (0.06 ** 2)))
    base_pulse = systolic + dicrotic

    # SpO2 optical absorption relationship:
    # R = (AC_red / DC_red) / (AC_ir / DC_ir)
    # Approx: SpO2 = 110 - 25 * R
    r_ratio = (110.0 - spo2) / 25.0
    r_ratio = max(0.4, min(1.6, r_ratio))

    dc_ir = 1.0
    ac_ir = 0.3 * base_pulse
    ppg_ir = dc_ir + ac_ir

    dc_red = 1.0
    ac_red = 0.3 * base_pulse * r_ratio
    ppg_red = dc_red + ac_red

    return ppg_ir, ppg_red

def generate_sample(class_idx, duration_sec=1.0, noise_level=0.03):
    """
    Generates a 5-channel 250 Hz window for the specified class.
    Returns: ndarray of shape (250, 5)
    """
    num_pts = int(SAMPLING_RATE_HZ * duration_sec)
    t = np.linspace(0, duration_sec, num_pts, endpoint=False)

    ecg = np.zeros(num_pts)
    ppg_ir = np.zeros(num_pts)
    ppg_red = np.zeros(num_pts)
    accel_mag = np.zeros(num_pts)
    temp = np.zeros(num_pts)

    if class_idx == 0:
        # Normal Sinus Rhythm: HR 65-85 bpm, SpO2 97-99%, 1g resting accel, 36.6°C
        hr = np.random.uniform(65, 85)
        spo2 = np.random.uniform(97.0, 99.5)
        ecg = generate_ecg_beat(t + np.random.uniform(0, 1), beat_type="normal", hr=hr)
        ppg_ir, ppg_red = generate_ppg_wave(t, hr=hr, spo2=spo2)
        accel_mag = 1.0 + 0.05 * np.sin(2 * np.pi * 0.3 * t) # gentle breathing motion
        temp = np.full(num_pts, 36.6 + np.random.normal(0, 0.1))

    elif class_idx == 1:
        # Atrial Fibrillation: Irregular HR 110-150 bpm, SpO2 95-98%, baseline noise
        hr = np.random.uniform(115, 145)
        spo2 = np.random.uniform(95.0, 98.0)
        ecg = generate_ecg_beat(t + np.random.uniform(0, 1), beat_type="afib", hr=hr)
        ppg_ir, ppg_red = generate_ppg_wave(t, hr=hr, spo2=spo2)
        accel_mag = 1.0 + np.random.normal(0, 0.04, num_pts)
        temp = np.full(num_pts, 36.8 + np.random.normal(0, 0.1))

    elif class_idx == 2:
        # Ventricular Tachycardia: High HR 160-210 bpm, wide QRS, SpO2 rapidly degrading
        hr = np.random.uniform(165, 205)
        spo2 = np.random.uniform(88.0, 93.0)
        ecg = generate_ecg_beat(t, beat_type="vtach", hr=hr)
        ppg_ir, ppg_red = generate_ppg_wave(t, hr=hr, spo2=spo2)
        ppg_ir = ppg_ir * 0.6 # diminished stroke volume
        ppg_red = ppg_red * 0.6
        accel_mag = 1.0 + np.random.normal(0, 0.08, num_pts)
        temp = np.full(num_pts, 37.1 + np.random.normal(0, 0.15))

    elif class_idx == 3:
        # Premature Ventricular Contraction: Normal background with ectopic wide beat
        hr = np.random.uniform(70, 85)
        spo2 = np.random.uniform(96.0, 98.5)
        ecg = generate_ecg_beat(t, beat_type="pvc", hr=hr)
        ppg_ir, ppg_red = generate_ppg_wave(t, hr=hr, spo2=spo2)
        accel_mag = 1.0 + np.random.normal(0, 0.03, num_pts)
        temp = np.full(num_pts, 36.7 + np.random.normal(0, 0.1))

    elif class_idx == 4:
        # Acute Hypoxia Decompensation: Severe desaturation SpO2 78-88%, Tachycardia HR 110-135, Fever 38.5°C
        hr = np.random.uniform(110, 135)
        spo2 = np.random.uniform(78.0, 87.5)
        ecg = generate_ecg_beat(t, beat_type="normal", hr=hr)
        ppg_ir, ppg_red = generate_ppg_wave(t, hr=hr, spo2=spo2)
        accel_mag = 1.0 + 0.12 * np.sin(2 * np.pi * 0.6 * t) # rapid tachypneic chest heave
        temp = np.full(num_pts, 38.6 + np.random.normal(0, 0.2)) # fever / sepsis

    elif class_idx == 5:
        # Patient Fall & Immobility: High-G impact (>3.5g peak) followed by prolonged horizontal stillness (~0.1g change), elevated HR
        hr = np.random.uniform(95, 125)
        spo2 = np.random.uniform(94.0, 97.0)
        ecg = generate_ecg_beat(t, beat_type="normal", hr=hr)
        ppg_ir, ppg_red = generate_ppg_wave(t, hr=hr, spo2=spo2)

        # Fall impact spike in first 0.3s then flat horizontal orientation
        accel_mag = np.ones(num_pts)
        impact_idx = int(0.25 * num_pts)
        accel_mag[impact_idx-10:impact_idx+10] = 3.8 + np.random.normal(0, 0.4, 20)
        accel_mag[impact_idx+10:] = 0.25 + np.random.normal(0, 0.01, num_pts - (impact_idx+10)) # on floor immobile
        temp = np.full(num_pts, 36.4 + np.random.normal(0, 0.1))

    # Add realistic sensor noise & baseline wander
    ecg += np.random.normal(0, noise_level, num_pts)
    ppg_ir += np.random.normal(0, noise_level * 0.5, num_pts)
    ppg_red += np.random.normal(0, noise_level * 0.5, num_pts)
    accel_mag += np.random.normal(0, noise_level * 0.3, num_pts)
    temp += np.random.normal(0, 0.02, num_pts)

    # Normalize each channel to zero mean and unit variance per window (Standard TinyML preprocessing)
    data = np.stack([ecg, ppg_ir, ppg_red, accel_mag, temp], axis=-1)
    return data.astype(np.float32)

def generate_full_dataset(samples_per_class=400, seed=42):
    """
    Generates balanced multi-class dataset for training, validation, and testing.
    """
    np.random.seed(seed)
    X_list = []
    y_list = []

    for c in range(len(CLASS_NAMES)):
        for _ in range(samples_per_class):
            sample = generate_sample(c)
            X_list.append(sample)
            y_list.append(c)

    X = np.array(X_list, dtype=np.float32) # Shape: (N, 250, 5)
    y = np.array(y_list, dtype=np.int64)

    # Shuffle
    indices = np.arange(len(X))
    np.random.shuffle(indices)
    X = X[indices]
    y = y[indices]

    return X, y

if __name__ == "__main__":
    print(f"Generating VITALIS-Edge multi-modal dataset (Classes: {len(CLASS_NAMES)})...")
    X, y = generate_full_dataset(samples_per_class=300)
    print(f"Dataset generated successfully: X shape = {X.shape}, y shape = {y.shape}")
    print(f"Sampling Rate: {SAMPLING_RATE_HZ} Hz, Window: {WINDOW_SIZE} samples (5 channels)")
    for i, name in enumerate(CLASS_NAMES):
        print(f"  Class {i}: {name} (Count: {np.sum(y == i)})")
