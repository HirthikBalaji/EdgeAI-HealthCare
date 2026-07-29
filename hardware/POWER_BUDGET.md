# VITALIS-Edge Power Budget & Battery Longevity Analysis

**DigiKey Presents electronica India Tech Challenge 2026**  
**Theme:** Edge AI & Healthcare  
**Battery Specification:** 3.7V 180 mAh Lithium-Polymer Cell (666 mWh)

---

## 1. Operating State Current Profile Breakdown

The system operates in a deterministic time-division duty cycle:
- **Sampling Window:** 1.0 second (250 samples)
- **Inference Cadence:** Every 500 ms (50% overlap hop)
- **Active Inference Duration:** 11.69 ms per cycle (Nordic nRF5340 @ 64 MHz)
- **Sensor Sampling Period:** 4.0 ms interval (250 Hz DMA burst = ~120 µs active per tick)

### Component Power Consumption Table

| Component | State / Operating Mode | Voltage (V) | Current (mA) | Duty Cycle (%) | Time-Averaged Current (µA) |
|:---|:---|:---:|:---:|:---:|:---:|
| **Nordic nRF5340 (App Core)** | Sleep (System ON, RAM retain) | 3.0 | 0.0018 | 97.6% | 1.76 |
| **Nordic nRF5340 (App Core)** | TinyML Inference @ 64 MHz | 3.0 | 3.2000 | 2.34% | 74.88 |
| **Nordic nRF5340 (Net Core)** | BLE 5.3 Periodic Adv (1 Hz) | 3.0 | 4.8000 | 0.06% | 2.88 |
| **Analog Devices AD8232** | Continuous ECG AFE Active | 3.0 | 0.1700 | 100.0% | 170.00 |
| **Maxim / ADI MAX30102** | Optical PPG (50Hz duty pulse) | 3.0 | 0.6000 | 20.0% | 120.00 |
| **ST LSM6DSOX IMU** | Low-Power MLC mode (26 Hz) | 3.0 | 0.0065 | 100.0% | 6.50 |
| **Texas Instruments TMP117** | 1-Shot Temp Mode (1 Hz) | 3.0 | 0.1350 | 0.15% | 0.20 |
| **Microchip SST26VF064B** | Standby Mode (< 1 µA) | 3.0 | 0.0010 | 99.9% | 1.00 |
| **Microchip SST26VF064B** | Flash Page Write (on event) | 3.0 | 12.0000 | 0.05% | 6.00 |
| **TI BQ25570 PMIC & LDO** | Quiescent Operating Current | 3.0 | 0.0008 | 100.0% | 0.80 |
| **Total System Average** | **Continuous Real-Time Ops**| **3.0**| **---** | **---** | **384.02 µA** |

---

## 2. Total Battery Lifetime Calculation

$$\text{Average Current Draw } (I_{\text{avg}}) = 384.02\ \mu\text{A} \approx 0.384\ \text{mA}$$

$$\text{Usable Battery Capacity } (C_{\text{eff}}) = 180\ \text{mAh} \times 0.85\ (\text{derating for aging/safety}) = 153\ \text{mAh}$$

$$\text{Battery Longevity (Hours)} = \frac{153\ \text{mAh}}{0.384\ \text{mA}} = 398.4\ \text{Hours}$$

$$\mathbf{\text{Battery Longevity (Days)}} = \frac{398.4\ \text{Hours}}{24\ \text{Hours/Day}} = \mathbf{16.6\ \text{Days}}$$

---

## 3. Energy Harvesting Extension (TI BQ25570 + Flexible Thermoelectric / Photovoltaic)

With an optional flexible body-heat thermoelectric generator (TEG) harvesting ~45 µW/cm² ($15\ \mu\text{A}$ at 3.0V continuous):
- Net current draw drops to **369 µA**.
- Extended operational lifespan increases to **over 17.2 Days** without recharging.
