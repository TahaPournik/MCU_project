# Sensing Module Documentation

**Project:** Temperature Monitoring System  
**Author:** Taha Pournik  
**Version:** 1.0

---

## Table of Contents

1. [Overview](#overview)
2. [Sensor Architecture](#sensor-architecture)
3. [MAX31865 - PT100 RTD Sensor](#max31865---pt100-rtd-sensor)
4. [MAX31855 - K-Type Thermocouple](#max31855---k-type-thermocouple)
5. [Implementation Code](#implementation-code)
6. [Data Processing Logic](#data-processing-logic)
7. [Error Handling](#error-handling)
8. [References](#references)

---

## Overview

The sensing module manages data acquisition from two independent temperature sensors via SPI/DMA. It provides **non-blocking sensor reads**, **moving average filtering**, **fault detection**, and **automatic calibration**. The module serves as the bridge between hardware sensors and the application layer.

### Module Features

- **Dual Sensor Support:**
  - MAX31865: High-precision PT100 RTD (-50°C to +200°C)
  - MAX31855: K-type thermocouple (0°C to +1024°C)
- **DMA-Based Communication:** Non-blocking, efficient SPI transfers
- **5-Sample Moving Average:** Noise reduction and stability
- **Comprehensive Fault Detection:** Hardware diagnostics for both sensors
- **Calibration Support:** Software offset correction
- **Thread-Safe Design:** Busy flags prevent concurrent access

### File Locations

- **Source:** `Core/Src/sensing.c`
- **Header:** `Core/Inc/sensing.h`

---

## Sensor Architecture

### System Overview

```
┌────────────────────────────────────────────────────┐
│                    Sensing Module                  │
│                                                    │
│  ┌──────────────┐            ┌──────────────┐      │
│  │  MAX31865    │            │  MAX31855    │      │
│  │  Interface   │            │  Interface   │      │
│  └──────┬───────┘            └──────┬───────┘      │
│         │                           │              │
│         ▼                           ▼              │
│  ┌──────────────────────────────────────────┐      │
│  │    5-Sample Moving Average Filter        │      │
│  └──────────────┬───────────────────────────┘      │
│                 │                                  │
│                 ▼                                  │
│  ┌──────────────────────────────────────────┐      │
│  │   Temperature Conversion & Calibration   │      │
│  └──────────────┬───────────────────────────┘      │
│                 │                                  │
│                 ▼                                  │
│  ┌──────────────────────────────────────────┐      │
│  │       sensor_data (Output)               │      │
│  │  - temp_max31865 (°C)                    │      │
│  │  - temp_max31855 (°C)                    │      │
│  └──────────────────────────────────────────┘      │
└────────────────────────────────────────────────────┘
```

### Data Flow

```
Timer Interrupt (500ms)
        │
        ▼
┌──────────────────┐
│ max31865_read()  │
│ max31855_read()  │
└────────┬─────────┘
         │
         ▼ (DMA Transfer)
┌──────────────────────────┐
│ HAL_SPI_TxRxCpltCallback │
│ - Store in buffer        │
│ - Update index           │
│ - Clear busy flag        │
└────────┬─────────────────┘
         │
         │ (After 5 samples)
         ▼
┌──────────────────────────┐
│ process_max31865_data()  │
│ process_max31855_data()  │
│ - Average 5 samples      │
│ - Convert to °C          │
│ - Apply calibration      │
└────────┬─────────────────┘
         │
         ▼
  sensor_data structure
  (Ready for display)
```

---

## MAX31865 - PT100 RTD Sensor

### Sensor Characteristics

- **Type:** Platinum Resistance Temperature Detector (RTD)
- **Element:** PT100 (100Ω at 0°C)
- **Range:** -50°C to +200°C (ambient temperature)
- **Resolution:** 15-bit ADC (0.03125°C)
- **Interface:** SPI with register-based access

### Communication Protocol

**MAX31865 Register Map:**

| Register Addr | Name | Function |
|---------------|------|----------|
| 0x00 | Configuration | Mode, wire config, fault detection |
| 0x01-0x02 | RTD MSB/LSB | 15-bit ADC value |
| 0x03-0x04 | High Fault Threshold | Upper alarm limit |
| 0x05-0x06 | Low Fault Threshold | Lower alarm limit |
| 0x07 | Fault Status | Diagnostic flags |

### Initialization Code

```c
/** Configuration buffer for MAX31865 */
uint8_t config_data_31865[MAX31865_DATA_SIZE] = {
    0x80,          // Write to register 0x00
    0xC2,          // Config: VBIAS ON, Auto-conversion, 3-wire, 60Hz filter
    0xFF, 0xFF,    // Dummy bytes for RTD ADC values regiser (Read Only)
    0x4C, 0x6A,    // High threshold
    0x40, 0x00,    // Low threshold
    0x00           // Dummy bytes for Fault registe (Read Only)
};

static void config_max31865(void) {
    if (max31865_busy_flag == 0) {
        max31865_busy_flag = 1;
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
        static uint8_t dummy_rx[MAX31865_DATA_SIZE];
        HAL_SPI_TransmitReceive_DMA(&hspi1, config_data_31865, dummy_rx, MAX31865_DATA_SIZE);   
    }
}
```

**Configuration Logic:**
- **0xC2 Breakdown:**
  - Bit 7: VBIAS on (enables reference resistor)
  - Bit 6: Auto-conversion mode
  - Bit 5: 1-shot disabled
  - Bit 4: 4-wire RTD
  - bit 2~3: Fault Detection Cycle Control (use for manual conversion)
  - Bit 1: Fault Status Clear (auto-clear)
  - Bit 0: 60Hz filter

### Reading Process

**Transmit Buffer (read command):**
```c
uint8_t tx_buf_31865[MAX31865_DATA_SIZE] = {
    0x00,  // Read from register 0x00
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // Dummy bytes for clock
};
```

**Read Function:**
```c
void max31865_read(void) {
    if (max31865_busy_flag == 0) {
        max31865_busy_flag = 1;
        /* Assert CS (active low) */
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
        /* Start DMA transfer */
        HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buf_31865, rx_buf_31865, MAX31865_DATA_SIZE);
    }
}
```

**Logic:**
1. Check if sensor is available (busy_flag == 0)
2. Set busy flag to prevent concurrent access
3. Pull CS low to select MAX31865
4. Initiate DMA transfer (9 bytes):
   - TX: Register address + dummy bytes
   - RX: Register contents

### Data Processing

```c
void process_max31865_data(void) {
    int32_t raw_accumulated = 0;
    
    /* Accumulate 5 samples */
    for (int i = 0; i < 5; i++) {
        /* Check fault bit (LSB of byte 3) */
        if ((max31865_data.data[i][3] & 0x01) || (max31865_data.data[i][3] == 0x00)) {
            max31865_ErrorCallback();
            return;
        }
        /* Assemble 15-bit ADC value from bytes 2 and 3 */
        raw_accumulated += ((max31865_data.data[i][2] << 7) | (max31865_data.data[i][3] >> 1));
    }
    
    /* Calculate average */
    int32_t raw_average = raw_accumulated / 5;
    
    /* Convert to temperature (°C) */
    sensor_data.temp_max31865 = ((float)raw_average / 32.0f) - 256.0f + max31865_calibration;
    
    /* Prepare LCD buffer */
    sprintf(max31865_lcd_buffer, "AMB:%.2f", sensor_data.temp_max31865);
}
```

**Conversion Logic:**
- **ADC Value:** 15-bit unsigned (0-32767)
- **Formula:** `Temperature = (ADC / 32) - 256 + calibration`
- **Averaging:** Sum of 5 samples divided by 5
- **Output:** Floating-point temperature in Celsius

---

## MAX31855 - K-Type Thermocouple

### Sensor Characteristics

- **Type:** Cold-Junction Compensated Thermocouple-to-Digital Converter
- **Thermocouple:** K-type (Chromel-Alumel)
- **Range:** 0°C to +1024°C (furnace temperature)
- **Resolution:** 14-bit signed (0.25°C)
- **Interface:** SPI read-only (32-bit stream)

### Data Format

MAX31855 outputs a **32-bit data stream** when CS is LOW:

```
Bit:  31  30  29 ... 18  17   16   15 ... 4  3   2   1   0
      │────────────────│   │─────│─────────│   │───────────│
      │     TC Temp    │   │fault│IC Temp  │   │  Faults   │
      │     (14-bit)   │   │     │(12-bit) │   │  (3-bit)  │
      └────────────────┴───┴─────┴─────────┴───┴───────────┘
      
Bytes: [0]      [1]      [2]      [3]
       D31-D24  D23-D16  D15-D8   D7-D0
```

**Bit Assignments:**
- **Bits 31-18:** Thermocouple temperature (14-bit signed, 0.25°C/bit)
- **Bit 16:** Fault flag (1 = error detected)
- **Bits 15-4:** Internal temperature (12-bit signed, 0.0625°C/bit)
- **Bit 2:** Short to VCC
- **Bit 1:** Short to GND
- **Bit 0:** Open circuit

### Reading Process

```c
void max31855_read(void) {
    if (max31855_busy_flag == 0) {
        max31855_busy_flag = 1;
        /* Assert CS (active low) */
        HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_RESET);
        /* Start DMA transfer with dummy TX to generate clock */
        static uint8_t tx_dummy[4] = {0x00, 0x00, 0x00, 0x00};
        HAL_SPI_TransmitReceive_DMA(&hspi2, tx_dummy, rx_buf_31855, MAX31855_DATA_SIZE);
    }
}
```

**Logic:**
1. Check availability (busy_flag == 0)
2. Assert CS to start data output
3. Send 4 dummy bytes to generate SPI clock
4. DMA receives 32-bit data stream into rx_buf_31855

### Data Processing

```c
void process_max31855_data(void) {
    int32_t temp_sum = 0;
    int16_t internal_temp = 0;
    int16_t thermocouple_temp = 0;

    for (int i = 0; i < 5; i++) {
        /* Check fault bit (bit 16, which is bit 0 of byte 1) */
        if (max31855_data.data[i][1] & 0x01) {
            max31855_ErrorCallback();
            return;
        }
        
        /* Extract Internal Temperature (12-bit signed) */
        if (max31855_data.data[i][2] & 0x80) {
            /* Negative temperature: sign extend */
            internal_temp = ((max31855_data.data[i][2] << 2) | 
                           (max31855_data.data[i][3] >> 6)) | 0xC000;
        } else {
            internal_temp = (max31855_data.data[i][2] << 2) | 
                          (max31855_data.data[i][3] >> 6);
        }
        
        /* Extract Thermocouple Temperature (14-bit signed) */
        if (max31855_data.data[i][0] & 0x80) {
            /* Negative temperature: sign extend */
            thermocouple_temp = ((max31855_data.data[i][0] << 6) | 
                               (max31855_data.data[i][1] >> 2)) | 0xC000;
        } else {
            thermocouple_temp = (max31855_data.data[i][0] << 6) | 
                              (max31855_data.data[i][1] >> 2);
        }
        
        /* Accumulate temperature difference */
        temp_sum += (thermocouple_temp - internal_temp);
    }
    
    /* Calculate average and convert to Celsius */
    sensor_data.temp_max31855 = (int32_t)round(((float)temp_sum / 5.0f) * 0.25f + 
                                              max31855_calibration);
    
    /* Prepare LCD buffer */
    sprintf(max31855_lcd_buffer, "FURN:%d C", (int)sensor_data.temp_max31855);
}
```

**Conversion Logic:**
1. **Extract 14-bit thermocouple temperature** from bits 31-18
2. **Extract 12-bit internal temperature** from bits 15-4
3. **Sign extension:** If MSB=1, extend to 16-bit signed
4. **Calculate difference:** TC_temp - IC_temp (cold-junction compensation)
5. **Average:** Sum of 5 samples / 5
6. **Convert:** Multiply by 0.25°C per LSB
7. **Apply calibration:** Add offset

---

## Implementation Code

### Key Data Structures

```c
/** Processed sensor output */
typedef struct {
    float temp_max31865;    // PT100 temperature (°C)
    int32_t temp_max31855;  // Thermocouple temperature (°C)
} Sensing_Data_t;

/** Moving average buffer (5 samples) */
typedef struct {
    uint8_t data[5][MAX31865_DATA_SIZE];  // 5 samples of raw data
    uint8_t index;                         // Current write index (0-4)
} Sensing_Data_Array_t;
```

### DMA Complete Callback

```c
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        /* Deselect MAX31865 */
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
        
        /* Update buffer index (circular) */
        if (max31865_data.index < 4) {
            max31865_data.index++;
        } else {
            max31865_data.index = 0;
        }
        
        /* Store received data */
        memcpy(max31865_data.data[max31865_data.index], rx_buf_31865, MAX31865_DATA_SIZE);
        max31865_busy_flag = 0;
        
    } else if (hspi->Instance == SPI2) {
        /* Deselect MAX31855 */
        HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
        
        /* Update buffer index (circular) */
        if (max31855_data.index < 4) {
            max31855_data.index++;
        } else {
            max31855_data.index = 0;
        }
        
        /* Store received data */
        memcpy(max31855_data.data[max31855_data.index], rx_buf_31855, MAX31855_DATA_SIZE);
        max31855_busy_flag = 0;
    }
}
```

**Logic:**
1. Identify which SPI peripheral completed transfer
2. De-assert CS pin (end communication)
3. Update circular buffer index (0→1→2→3→4→0)
4. Copy DMA buffer to circular buffer for averaging
5. Clear busy flag (allow next read)

---

## Data Processing Logic

### Moving Average Implementation

The module uses a **5-sample moving average** to reduce noise:

```
Sample Buffer (Circular):
┌───┬───┬───┬───┬───┐
│ 0 │ 1 │ 2 │ 3 │ 4 │
└───┴───┴───┴───┴───┘
  ↑
Index (wraps to 0 after 4)

Processing:
Average = (Sample[0] + Sample[1] + Sample[2] + Sample[3] + Sample[4]) / 5
```

### Calibration

**Software offset correction** is applied after conversion:

```c
float max31865_calibration = 0.0f;    // PT100 offset
float max31855_calibration = 0.75f;   // Thermocouple offset

// Applied in processing:
sensor_data.temp_max31865 = raw_temp + max31865_calibration;
```

**Purpose:**
- Compensate for sensor tolerances
- Correct systematic errors
- Adjust for environmental factors

---

## Error Handling

### Fault Detection

Both sensors have built-in diagnostics:

#### MAX31865 Fault Flags

```c
#define MAX31865_HIGH_THD       0x80  // RTD High Threshold
#define MAX31865_LOW_THD        0x40  // RTD Low Threshold
#define MAX31865_REF_SC         0x20  // REFIN- < 0.85 × VBIAS (short)
#define MAX31865_REF_OC         0x10  // REFIN- > 0.85 × VBIAS (open)
#define MAX31865_SENSOR_OC      0x08  // RTD open circuit
#define MAX31865_OU_VOLTAGE     0x04  // Overvoltage/undervoltage fault
```

#### MAX31855 Fault Flags

```c
#define MAX31855_SENSOR_OC      0x01  // Thermocouple open circuit
#define MAX31855_SENSOR_SC_GND  0x02  // Short to ground
#define MAX31855_SENSOR_SC_VCC  0x04  // Short to VCC
```

### Error Callback Functions

```c
void max31865_ErrorCallback(void) {
    sensor_data.temp_max31865 = -999.0f;  // Invalid reading
    max31865_fault_flag = 1;
    max31865_error |= HAL_SPI_GetError(&hspi1);
    
    /* Check all samples for fault bits */
    uint8_t fault_bits = 0;
    for (int i = 0; i < 5; i++) {
        if (max31865_data.data[i][3] & 0x01) {
            fault_bits |= max31865_data.data[i][8];  // Fault status register
        }
    }
    
    /* Decode fault flags */
    if (fault_bits & 0x04) max31865_error |= MAX31865_OU_VOLTAGE;
    if (fault_bits & 0x08) max31865_error |= MAX31865_SENSOR_OC;
    if (fault_bits & 0x10) max31865_error |= MAX31865_REF_OC;
    // ... (additional checks)
    
    sprintf(max31865_lcd_buffer, "ERR:0X%X", (int)max31865_error);
}
```

**Logic:**
1. Set temperature to invalid value (-999)
2. Set fault flag to prevent further reads
3. Accumulate SPI errors
4. Check fault bits from all samples
5. Decode sensor-specific error codes
6. Format error message for LCD

---

## References

### Sensor Datasheets

- [MAX31865 Datasheet](max31865.pdf) - RTD-to-Digital Converter
  - Register map: Page 14-16
  - Fault detection: Page 17-18
  - PT100 application: Page 21-22
- [MAX31855 Datasheet](max31855.pdf) - Thermocouple-to-Digital Converter
  - Data format: Page 9-10
  - Fault detection: Page 11
  - K-type specifications: Page 3

### STM32 Documentation

- [STM32F1 HAL/LL Drivers](STM32f1_HAL_LL_Drivers.pdf) - HAL_SPI_TransmitReceive_DMA API

### Related Project Documentation

- [System Architecture](System_Architecture.md) - Overall system design
- [SPI Peripheral](SPI_peripheral.md) - SPI configuration for sensors
- [DMA Peripheral](DMA_peripheral.md) - DMA channel setup
- [TIM Peripheral](TIM_peripheral.md) - Sampling timer

---

**Document Version:** 1.0  
**Last Updated:** January 2026  
**Author:** Taha Pournik
