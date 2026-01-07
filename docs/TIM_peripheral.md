# TIM Peripheral Configuration

**Project:** Temperature Monitoring System  
**Author:** Taha Pournik  
**Version:** 1.0

---

## Table of Contents

1. [Overview](#overview)
2. [Timer Architecture](#timer-architecture)
3. [TIM1 Configuration](#tim1-configuration)
4. [Timing Calculations](#timing-calculations)
5. [Implementation Code](#implementation-code)
6. [Sampling Logic](#sampling-logic)
7. [References](#references)

---

## Overview

The Timer peripheral (TIM1) is configured to generate periodic interrupts every **500 milliseconds**, serving as the heartbeat for the sensor sampling system. This timer orchestrates the sequential reading of both temperature sensors, ensuring proper timing and preventing SPI bus contention.

### Timer Function Summary

| Parameter | Value |
|-----------|-------|
| **Timer Used** | TIM1 (Advanced Control Timer) |
| **Interrupt Period** | 500 ms (2 Hz) |
| **CPU Clock** | 72 MHz |
| **Clock Division** | DIV4 (÷4) |
| **Prescaler** | 8999 (divides by 9000) |
| **Auto-Reload** | 999 (counts to 1000) |
| **Interrupt Priority** | 0 (Highest) |

---

## Timer Architecture

### TIM1 Block Diagram

```
        72 MHz System Clock
               │
               ▼
        ┌──────────────┐
        │Clock Division│
        │     ÷ 4      │
        └──────┬───────┘
               │
          18 MHz Clock
               │
               ▼
        ┌──────────────┐
        │  Prescaler   │
        │   ÷ 9000     │
        └──────┬───────┘
               │
           2 kHz Clock
               │
               ▼
        ┌──────────────┐
        │   Counter    │
        │  0 → 999     │
        └──────┬───────┘
               │
        Counter Overflow
               │
               ▼
        ┌──────────────┐
        │  Interrupt   │
        │   (500 ms)   │
        └──────┬───────┘
               │
               ▼
    HAL_TIM_PeriodElapsedCallback()
```

### Timing Sequence

```
Time: 0ms    500ms   1000ms  1500ms  2000ms  2500ms  3000ms  3500ms  4000ms  4500ms  5000ms
      │      │       │       │       │       │       │       │       │       │       │
      ▼      ▼       ▼       ▼       ▼       ▼       ▼       ▼       ▼       ▼       ▼
TIM1: INT    INT     INT     INT     INT     INT     INT     INT     INT     INT     INT
      │      │       │       │       │       │       │       │       │       │       │
Step: 0      1       2       3       4       5       6       7       8       9       10
      │      │       │       │       │       │       │       │       │       │       │
Read: 31865  31855   31865   31855   31865   31855   31865   31855   31865   31855   Process

      ◄─────────────────── Collect 5 Samples Each (5 seconds) ────────────────►
```

---

## TIM1 Configuration

### Timer Configuration Structure

TIM1 is configured as a basic up-counter with the following parameters:

```c
htim1.Instance = TIM1;
htim1.Init.Prescaler = 8999;                        // (PSC + 1) = 9000
htim1.Init.CounterMode = TIM_COUNTERMODE_UP;        // Count from 0 to ARR
htim1.Init.Period = 999;                            // Auto-reload value (ARR)
htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;  // Clock Division 4
htim1.Init.RepetitionCounter = 0;                   // No repetition
htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
```

### Parameter Explanation

| Parameter | Value | Description |
|-----------|-------|-------------|
| **Clock Division** | DIV4 | Divides input clock by 4 (for dead-time and filter) |
| **Prescaler** | 8999 | Divides timer clock by 9000 |
| **Counter Mode** | Up-counting | Counter increments from 0 |
| **Period (ARR)** | 999 | Counter reloads after reaching 999 (1000 counts) |
| **Clock Division Effect** | ÷4 | Reduces effective timer clock to 18 MHz |
| **Repetition Counter** | 0 | Interrupt on every overflow |

### Interrupt Configuration

```c
/* TIM1 interrupt Init */
HAL_NVIC_SetPriority(TIM1_UP_IRQn, 0, 0);  // Highest priority
HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);          // Enable interrupt
```

**Priority Settings:**
- **Preemption Priority:** 0 (highest)
- **Sub Priority:** 0
- This ensures timer interrupts are not delayed by other interrupts

---

## Timing Calculations

### Formula

The timer interrupt period is calculated as:

```
T_interrupt = (Prescaler + 1) × (Period + 1) / F_timer
```

Where:
- **F_timer** = Timer clock frequency (72 MHz for TIM1 on APB2)
- **Prescaler** = 8999
- **Period (ARR)** = 999

### Calculation Steps

1. **Timer Clock Frequency:**
   ```
   TIM1 is on APB2 → F_timer = 72 MHz
   ```

2. **Clock Division:**
   ```
   F_divided = 72 MHz / 4 = 18 MHz
   ```

3. **Prescaler Output:**
   ```
   F_prescaled = 18 MHz / 9000 = 2 kHz
   ```

4. **Counter Period:**
   ```
   Counter counts from 0 to 999 → 1000 counts
   ```

5. **Interrupt Period:**
   ```
   T_interrupt = 1000 / 2000 Hz = 0.5 seconds = 500 ms
   ```

6. **Interrupt Frequency:**
   ```
   F_interrupt = 1 / 0.5 s = 2 Hz
   ```

### Verification

| Stage | Frequency | Period |
|-------|-----------|--------|
| Timer Clock (APB2) | 72 MHz | 13.9 ns |
| After Clock Division | 18 MHz | 55.6 ns |
| After Prescaler | 2 kHz | 500 μs |
| After Counter | 2 Hz | **500 ms** ✓ |

---

## Implementation Code

### File Location

Source: `Core/Src/tim.c`

### Timer Initialization

```c
/* TIM1 init function */
void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* Timer base configuration */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 8999;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
    Error_Handler();
  }
  
  /* Configure internal clock source */
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
    Error_Handler();
  }
  
  /* Configure master mode (not used in this project) */
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
    Error_Handler();
  }
}
```

### Timer Interrupt Callback

```c
/**
 * @brief Counter to sequence sensor readings.
 * Even steps: Read MAX31855
 * Odd steps: Read MAX31865
 * Step 9+: Process and reset
 */
static uint8_t sampling_step = 0;

/**
 * @brief  Period elapsed callback in non-blocking mode
 * @param  htim TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* Toggle status LED for visual feedback */
  HAL_GPIO_TogglePin(LED_POWER_GPIO_Port, LED_POWER_Pin);
  
  if (htim->Instance == TIM1) 
  {
    /* Sequencing sensor readings to avoid SPI bus contention */
    if (sampling_step <= 9) 
    {
      if (sampling_step % 2) 
      {
        /* Odd steps: Request MAX31865 reading if ready */
        sampling_step++;
        if (!max31865_busy_flag && !max31865_fault_flag) 
        {
          max31865_read();
        }
      } 
      else 
      {
        /* Even steps: Request MAX31855 reading if ready */
        sampling_step++;
        if (!max31855_busy_flag && !max31855_fault_flag) 
        {
          max31855_read();
        }
      }
    } 
    else
    {
      /* Final step: Process all accumulated samples */
      process_max31865_data();
      process_max31855_data();
      sampling_step = 0;
    }
  }
}
```

---

## Sampling Logic

### Step-by-Step Sequence

The timer callback implements a **10-step sampling cycle**:

| Step | Time (s) | Action | Sensor |
|------|-----------|--------|--------|
| 0 | 0.0 | Read | MAX31865 (PT100)  |
| 1 | 0.5 | Read | MAX31855 (Thermocouple) |
| 2 | 1.0 | Read | MAX31865 |
| 3 | 1.5 | Read | MAX31855 |
| 4 | 2.0 | Read | MAX31865 |
| 5 | 2.5 | Read | MAX31855 |
| 6 | 3.0 | Read | MAX31865 |
| 7 | 3.5 | Read | MAX31855 |
| 8 | 4.0 | Read | MAX31865 |
| 9 | 4.5 | Read | MAX31855 |
| 10 | 5.0 | **Process Data** | Both sensors |

### Flow Diagram

```
     TIM1 Interrupt (500ms)
            │
            ▼
     ┌──────────────┐
     │ Toggle LED   │
     └──────┬───────┘
            │
            ▼
     ┌──────────────┐
     │  sampling_   │
     │  step <= 9?  │
     └──────┬───────┘
            │
      ┌─────┴─────┐
      │           │
     YES         NO
      │           │
      ▼           ▼
   ┌──────┐   ┌───────────────┐
   │ Even?│   │ Process Data: │
   └──┬───┘   │ - Average     │
      │       │ - Calibrate   │
   ┌──┴──┐    │ - Convert     │
   │     │    │ Reset step=0  │
  YES   NO    └───────────────┘
   │     │
   ▼     ▼
 Read  Read
31855 31865
   │     │
   └──┬──┘
      │
 sampling_step++
```

### Logic Explanation

1. **Even Steps (0, 2, 4, 6, 8):**
   - Check if MAX31865 is ready (not busy, no fault)
   - If ready, initiate DMA read via `max31865_read()`
   - Increment step counter

2. **Odd Steps (1, 3, 5, 7, 9):**
   - Check if MAX31855 is ready
   - If ready, initiate DMA read via `max31855_read()`
   - Increment step counter

3. **Step 10 (After 5 seconds):**
   - Call `process_max31865_data()` → Averages 5 samples (collected over 5 seconds)
   - Call `process_max31855_data()` → Averages 5 samples (collected over 5 seconds)
   - Reset `sampling_step = 0` to restart cycle

### Benefits of This Approach

1. **Prevents Bus Contention:**
   - Only one SPI transfer happens at a time
   - Both sensors share DMA1 controller

2. **Moving Average Filter (5-second window):**
   - Collects 5 samples per sensor over 5 seconds
   - Reduces noise and provides stable readings

3. **Fault Tolerance:**
   - If a sensor is busy or faulted, skip that reading
   - System continues operating with other sensor

4. **Predictable Timing:**
   - Hardware timer ensures exact 500ms intervals
   - Not affected by CPU load or delays

---

## References

### STM32 Documentation

- [STM32F1 Reference Manual](Reference_Manual.pdf) - Chapter 14: Advanced-control timers (TIM1)
- [STM32F1 HAL/LL Drivers](STM32f1_HAL_LL_Drivers.pdf) - HAL_TIM API reference
- [STM32F103C8 Datasheet](STM32f103c8.pdf) - Timer specifications

### Related Project Documentation

- [System Architecture](System_Architecture.md) - Overall system design
- [DMA Peripheral](DMA_peripheral.md) - DMA usage for sensor reading
- [Sensing Module](sensing_module.md) - Sensor reading and processing logic

---

**Document Version:** 1.0  
**Last Updated:** January 2026  
**Author:** Taha Pournik
