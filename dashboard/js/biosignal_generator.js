/**
 * VITALIS-Edge: Multi-Modal Biosignal Generator & PhysioNet Simulator
 * Real-time synthesis of ECG, PPG (IR/Red), 3-Axis IMU, and Skin Temperature.
 */

class BiosignalGenerator {
  constructor(sampleRateHz = 250) {
    this.fs = sampleRateHz;
    this.t = 0;
    this.condition = "NSR"; // NSR, AFIB, VTACH, PVC, HYPOXIA, FALL
    this.heartRate = 75.0;
    this.spo2 = 98.5;
    this.temperature = 36.6;
    this.accelMag = 1.0;
    this.pvcTimer = 0;
  }

  setCondition(cond) {
    this.condition = cond;
    switch (cond) {
      case "NSR":
        this.heartRate = 75.0;
        this.spo2 = 98.5;
        this.temperature = 36.6;
        break;
      case "AFIB":
        this.heartRate = 135.0;
        this.spo2 = 96.0;
        this.temperature = 36.8;
        break;
      case "VTACH":
        this.heartRate = 185.0;
        this.spo2 = 88.0;
        this.temperature = 37.2;
        break;
      case "PVC":
        this.heartRate = 78.0;
        this.spo2 = 97.5;
        this.temperature = 36.7;
        break;
      case "HYPOXIA":
        this.heartRate = 125.0;
        this.spo2 = 81.5;
        this.temperature = 38.8; // Fever
        break;
      case "FALL":
        this.heartRate = 110.0;
        this.spo2 = 95.0;
        this.temperature = 36.5;
        break;
    }
  }

  getNextSample() {
    this.t += 1.0 / this.fs;
    const period = 60.0 / this.heartRate;
    let phase = (this.t % period) / period;

    let ecg = 0.0;
    let ppgIr = 0.0;
    let ppgRed = 0.0;
    let accelX = 0.0, accelY = 0.0, accelZ = 1.0;
    let temp = this.temperature + (Math.random() - 0.5) * 0.02;

    if (this.condition === "NSR") {
      // Normal P-Q-R-S-T
      ecg += 0.15 * Math.exp(-Math.pow(phase - 0.15, 2) / (2 * 0.025 * 0.025));
      ecg -= 0.12 * Math.exp(-Math.pow(phase - 0.28, 2) / (2 * 0.010 * 0.010));
      ecg += 1.20 * Math.exp(-Math.pow(phase - 0.30, 2) / (2 * 0.015 * 0.015));
      ecg -= 0.25 * Math.exp(-Math.pow(phase - 0.33, 2) / (2 * 0.012 * 0.012));
      ecg += 0.30 * Math.exp(-Math.pow(phase - 0.55, 2) / (2 * 0.045 * 0.045));

      accelZ = 1.0 + 0.05 * Math.sin(2 * Math.PI * 0.3 * this.t);
    } 
    else if (this.condition === "AFIB") {
      // Fibrillatory baseline + irregular R peaks
      ecg += 0.08 * Math.sin(2 * Math.PI * 7.5 * this.t) + 0.05 * Math.cos(2 * Math.PI * 11.3 * this.t);
      ecg -= 0.10 * Math.exp(-Math.pow(phase - 0.28, 2) / (2 * 0.010 * 0.010));
      ecg += 1.15 * Math.exp(-Math.pow(phase - 0.30, 2) / (2 * 0.015 * 0.015));
      ecg -= 0.20 * Math.exp(-Math.pow(phase - 0.33, 2) / (2 * 0.012 * 0.012));
      ecg += 0.20 * Math.exp(-Math.pow(phase - 0.55, 2) / (2 * 0.040 * 0.040));
    }
    else if (this.condition === "VTACH") {
      // Broad monomorphic sine wave QRS
      ecg += 1.35 * Math.sin(2 * Math.PI * (this.heartRate / 60.0) * this.t);
      ecg += 0.30 * Math.sin(4 * Math.PI * (this.heartRate / 60.0) * this.t + 0.5);
    }
    else if (this.condition === "PVC") {
      this.pvcTimer += 1.0 / this.fs;
      if (this.pvcTimer > 3.0 && phase > 0.25 && phase < 0.7) {
        // Ectopic bizarre complex
        ecg += 1.35 * Math.exp(-Math.pow(phase - 0.30, 2) / (2 * 0.040 * 0.040));
        ecg -= 0.85 * Math.exp(-Math.pow(phase - 0.36, 2) / (2 * 0.035 * 0.035));
        ecg -= 0.40 * Math.exp(-Math.pow(phase - 0.60, 2) / (2 * 0.055 * 0.055));
        if (phase > 0.68) this.pvcTimer = 0;
      } else {
        ecg += 0.15 * Math.exp(-Math.pow(phase - 0.15, 2) / (2 * 0.025 * 0.025));
        ecg -= 0.12 * Math.exp(-Math.pow(phase - 0.28, 2) / (2 * 0.010 * 0.010));
        ecg += 1.20 * Math.exp(-Math.pow(phase - 0.30, 2) / (2 * 0.015 * 0.015));
        ecg -= 0.25 * Math.exp(-Math.pow(phase - 0.33, 2) / (2 * 0.012 * 0.012));
        ecg += 0.30 * Math.exp(-Math.pow(phase - 0.55, 2) / (2 * 0.045 * 0.045));
      }
    }
    else if (this.condition === "HYPOXIA") {
      ecg += 0.15 * Math.exp(-Math.pow(phase - 0.15, 2) / (2 * 0.025 * 0.025));
      ecg += 1.20 * Math.exp(-Math.pow(phase - 0.30, 2) / (2 * 0.015 * 0.015));
      ecg -= 0.25 * Math.exp(-Math.pow(phase - 0.33, 2) / (2 * 0.012 * 0.012));
      ecg += 0.30 * Math.exp(-Math.pow(phase - 0.55, 2) / (2 * 0.045 * 0.045));
      accelZ = 1.0 + 0.25 * Math.sin(2 * Math.PI * 0.65 * this.t); // Tachypneic chest heave
    }
    else if (this.condition === "FALL") {
      ecg += 1.10 * Math.exp(-Math.pow(phase - 0.30, 2) / (2 * 0.015 * 0.015));
      // Flat horizontal posture (0.2g)
      accelX = 0.85;
      accelY = 0.35;
      accelZ = 0.22;
    }

    // PPG Pulse
    const systolic = 1.0 * Math.exp(-Math.pow(phase - 0.20, 2) / (2 * 0.08 * 0.08));
    const dicrotic = 0.35 * Math.exp(-Math.pow(phase - 0.45, 2) / (2 * 0.06 * 0.06));
    const pulse = systolic + dicrotic;

    const rRatio = Math.max(0.4, Math.min(1.6, (110.0 - this.spo2) / 25.0));
    ppgIr = 1.0 + 0.3 * pulse;
    ppgRed = 1.0 + 0.3 * pulse * rRatio;

    // Add mild Gaussian noise
    ecg += (Math.random() - 0.5) * 0.03;
    ppgIr += (Math.random() - 0.5) * 0.015;
    ppgRed += (Math.random() - 0.5) * 0.015;

    const accelMag = Math.sqrt(accelX * accelX + accelY * accelY + accelZ * accelZ);

    return {
      t: this.t,
      ecg,
      ppgIr,
      ppgRed,
      accelX,
      accelY,
      accelZ,
      accelMag,
      temperature: temp,
      heartRate: this.heartRate,
      spo2: this.spo2
    };
  }
}

window.BiosignalGenerator = BiosignalGenerator;
