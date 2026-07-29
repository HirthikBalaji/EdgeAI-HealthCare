# VITALIS-Edge Hardware Architecture & Pinout Specifications

**DigiKey Presents electronica India Tech Challenge 2026**  
**Theme:** Edge AI & Healthcare  
**Target Platform:** Nordic Semiconductor nRF5340 Dual-Core System-on-Chip (SoC)

---

## 1. System Block Diagram

```
 +-----------------------------------------------------------------------------------+
 |                             VITALIS-Edge WEARABLE PATCH                           |
 |                                                                                   |
 |  +--------------------+                                                           |
 |  | Analog Devices     |--- (Analog ECG_OUT) ---> [ADC0 (P0.04)]                   |
 |  | AD8232 ECG AFE     |--- (Digital LO+/LO-) -> [GPIO (P0.05, P0.06)]             |
 |  +--------------------+                                                           |
 |                                                                                   |
 |  +--------------------+                                                           |
 |  | Maxim / ADI        |--- (I2C0 @ 400kHz) ----> [TWIM0 (SDA:P0.26, SCL:P0.27)]  |
 |  | MAX30102 PPG/SpO2  |--- (INT Pin) ----------> [GPIO P0.28 (Ext Interrupt)]     |
 |  +--------------------+                                                           |
 |                                                                                   |
 |  +--------------------+                                                           |
 |  | STMicroelectronics |--- (I2C0 @ 400kHz) ----> [Shared TWIM0 Bus]              |
 |  | LSM6DSOX 6-DoF IMU |--- (MLC INT1 / INT2) --> [GPIO P0.29, P0.30]              |
 |  +--------------------+                                                           |
 |                                                                                   |
 |  +--------------------+                                                           |
 |  | Texas Instruments  |--- (I2C0 @ 400kHz) ----> [Shared TWIM0 Bus]              |
 |  | TMP117 Temp Sensor |--- (ALERT Pin) --------> [GPIO P0.31]                     |
 |  +--------------------+                                                           |
 |                                                                                   |
 |  +--------------------+                                                           |
 |  | Microchip          |--- (QSPI / SPI1) ------> [SPIM1 (SCK:P0.13, CS:P0.14,    |
 |  | SST26VF064B Flash  |                          MISO:P0.15, MOSI:P0.16)]         |
 |  +--------------------+                                                           |
 |                                                                                   |
 |  +--------------------+                                                           |
 |  | Texas Instruments  |--- (I2C0 @ 400kHz) ----> [Shared TWIM0 Bus]              |
 |  | DRV2605L Haptics   |--- (TRIG Pin) ---------> [GPIO P1.01]                     |
 |  +--------------------+                                                           |
 |                                                                                   |
 |  +--------------------+                                                           |
 |  | Murata Piezo       |--- (PWM Channel 0) ----> [PWM0 (P1.02)]                   |
 |  | Audio Transducer   |                                                           |
 |  +--------------------+                                                           |
 |                                                                                   |
 |  +--------------------+      +-------------------------------------------------+  |
 |  | TI BQ25570 PMIC    | ---> | Nordic nRF5340 Dual ARM Cortex-M33              |  |
 |  | & Murata Supercap  |      |   - App Core @ 128MHz: TinyML & DSP Filter      |  |
 |  | + Molex 180mAh LiPo|      |   - Net Core @ 64MHz:  BLE 5.3 Emergency Beacon |  |
 |  +--------------------+      +-------------------------------------------------+  |
 +-----------------------------------------------------------------------------------+
```

---

## 2. MCU Pinout Assignment Table (Nordic nRF5340 aQFN-94)

| Pin Name | Port / Pin | Connected Subsystem | Functional Role | Bus Speed / Characteristics |
|:---|:---|:---|:---|:---|
| **ECG_ADC_IN** | P0.04 (AIN0) | AD8232 ECG Out | High-speed Single-Ended ADC biopotential input | 250 Hz sample rate, 12-bit DMA |
| **ECG_LO_PLUS** | P0.05 | AD8232 Leads-Off Positive | Digital Leads-Off detection (LO+) | Pull-down GPIO Interrupt |
| **ECG_LO_MINUS**| P0.06 | AD8232 Leads-Off Negative | Digital Leads-Off detection (LO-) | Pull-down GPIO Interrupt |
| **ECG_SDN** | P0.07 | AD8232 Shutdown Control | Active-low power-down switch | < 1 µA sleep current |
| **I2C_SDA** | P0.26 | MAX30102, LSM6DSOX, TMP117, DRV2605L | Shared I2C Serial Data Bus | Fast Mode 400 kHz (4.7kΩ Pullup) |
| **I2C_SCL** | P0.27 | MAX30102, LSM6DSOX, TMP117, DRV2605L | Shared I2C Serial Clock Bus | Fast Mode 400 kHz (4.7kΩ Pullup) |
| **PPG_INT** | P0.28 | MAX30102 Interrupt | FIFO almost-full / sample-ready | Active-low Edge-triggered IRQ |
| **IMU_INT1** | P0.29 | ST LSM6DSOX INT1 | MLC Activity / Fall Impact IRQ | Active-high Hardware Wakeup |
| **IMU_INT2** | P0.30 | ST LSM6DSOX INT2 | Free-fall hardware threshold IRQ | Active-high Hardware Wakeup |
| **TMP_ALERT** | P0.31 | TI TMP117 ALERT | Pyrexia / Hypothermia threshold IRQ | Open-drain active-low IRQ |
| **QSPI_SCK** | P0.13 | Microchip SST26VF064B | SPI Flash Serial Clock | 32 MHz High-Speed SPI |
| **QSPI_CS** | P0.14 | Microchip SST26VF064B | Chip Select (Active Low) | Dedicated Flash CS |
| **QSPI_SIO0** | P0.15 | Microchip SST26VF064B | Serial Data Input (MOSI / SIO0) | Quad-SPI Data 0 |
| **QSPI_SIO1** | P0.16 | Microchip SST26VF064B | Serial Data Output (MISO / SIO1)| Quad-SPI Data 1 |
| **HAPTIC_EN** | P1.01 | TI DRV2605L EN/TRIG | Tactile emergency pattern trigger | 3.3V Logic Level Output |
| **PIEZO_PWM** | P1.02 | Murata Piezo Audio | Emergency acoustic resonant tone | 4.0 kHz Resonant Frequency |
| **RF_ANT** | ANT_2G4 | Murata Ceramic Chip Antenna | BLE 5.3 PHY @ +3 dBm TX power | 50Ω Matched Impedance |

---

## 3. Power Architecture & Component Interconnects

1. **Analog Isolation:** Dedicated LDO (TI TPS7A02 ultra-low noise 25 nA IQ) regulates 3.0V for the AD8232 ECG analog front-end to eliminate switching noise from the digital MCU core.
2. **Dynamic Voltage Scaling (DVS):** Nordic nRF5340 Application Core operates at 64 MHz during standard DSP / sleep cycles and bursts to 128 MHz for 11.6ms during TinyML inference windows.
3. **Power-Failure Resilience:** Murata 100mF supercapacitor ensures complete blackbox event flushing to SPI NOR flash even during sudden battery detachment.
