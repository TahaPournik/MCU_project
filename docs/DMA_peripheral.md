# DMA Peripheral Configuration

**Project:** Temperature Monitoring System  
**Author:** Taha Pournik  
**Version:** 1.0

---

## Table of Contents

1. [Overview](#overview)
2. [DMA Architecture](#dma-architecture)
3. [Channel Configuration](#channel-configuration)
4. [Implementation](#implementation)
5. [Timing Diagrams](#timing-diagrams)
6. [Code Explanation](#code-explanation)
7. [References](#references)

---

## Overview

Direct Memory Access (DMA) is used extensively in this project to enable efficient, CPU-independent data transfers between memory and SPI peripherals. This approach allows the CPU to perform other tasks while sensor data is being transferred, improving overall system responsiveness.

### Benefits of DMA in This Project

- **Non-blocking Operation:** CPU can continue execution during SPI transfers
- **Reduced Interrupt Overhead:** Only one interrupt per complete transfer
- **Consistent Timing:** Hardware-managed transfers ensure predictable performance
- **Energy Efficiency:** CPU can enter low-power states during transfers

### DMA Controller: DMA1

The STM32F103C8 has one DMA controller (DMA1) with 7 channels. This project uses 4 channels:

| Channel | Function        | Direction        | Priority |
|---------|-----------------|------------------|----------|
| DMA1_CH2| SPI1 RX (MAX31865) | Peripheral → Memory | Low      |
| DMA1_CH3| SPI1 TX (MAX31865) | Memory → Peripheral | Low      |
| DMA1_CH4| SPI2 RX (MAX31855) | Peripheral → Memory | Low      |
| DMA1_CH5| SPI2 TX (MAX31855) | Memory → Peripheral | Highest  |

---

## DMA Architecture

### DMA1 Block Diagram

```
┌──────────────────────────────────────────────────────┐
│                    DMA1 Controller                   │
│                                                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│  │ Channel2 │  │ Channel3 │  │ Channel4 │ ...        │
│  │ SPI1_RX  │  │ SPI1_TX  │  │ SPI2_RX  │            │
│  └────┬─────┘  └─────┬────┘  └────┬─────┘            │
│       │              │             │                 │
└───────┼──────────────┼─────────────┼─────────────────┘
        │              │             │
    ┌───▼───┐      ┌───▼───┐     ┌───▼───┐
    │  RAM  │      │  RAM  │     │  RAM  │
    │ (RX)  │      │ (TX)  │     │ (RX)  │
    └───────┘      └───────┘     └───────┘
        ▲              │             ▲
        │              ▼             │
    ┌───┴──────────────┴─────────┬───┴───┐
    │         SPI1_DR            │SPI2_DR│
    └────────────────────────────┴───────┘
```

### Transfer Mechanism

**Full-Duplex SPI with DMA:**

1. **TX DMA** writes data from memory to SPI_DR (Data Register)
2. **SPI peripheral** shifts data out on MOSI and receives on MISO
3. **RX DMA** reads received data from SPI_DR to memory
4. **Interrupt** fires when both TX and RX complete

---

## Channel Configuration

### SPI1 - MAX31865 (PT100 Sensor)

#### DMA1 Channel 2 (RX)

**Purpose:** Receive sensor data from MAX31865

**Configuration:**
- **Direction:** Peripheral to Memory
- **Peripheral:** SPI1_DR register
- **Memory:** `rx_buf_31865[9]`
- **Peripheral Increment:** Disabled (always read from SPI1_DR)
- **Memory Increment:** Enabled (fill buffer sequentially)
- **Data Alignment:** Byte (8-bit)
- **Transfer Mode:** Normal (single transfer)
- **Priority:** Low
- **Interrupt:** Enabled (on transfer complete)

#### DMA1 Channel 3 (TX)

**Purpose:** Send register addresses to MAX31865

**Configuration:**
- **Direction:** Memory to Peripheral
- **Peripheral:** SPI1_DR register
- **Memory:** `tx_buf_31865[9]`
- **Peripheral Increment:** Disabled
- **Memory Increment:** Enabled
- **Data Alignment:** Byte (8-bit)
- **Transfer Mode:** Normal
- **Priority:** Low
- **Interrupt:** Enabled (on transfer complete)

### SPI2 - MAX31855 (Thermocouple)

#### DMA1 Channel 4 (RX)

**Purpose:** Receive thermocouple data

**Configuration:**
- **Direction:** Peripheral to Memory
- **Peripheral:** SPI2_DR register
- **Memory:** `rx_buf_31855[4]`
- **Peripheral Increment:** Disabled
- **Memory Increment:** Enabled
- **Data Alignment:** Byte (8-bit)
- **Transfer Mode:** Normal
- **Priority:** Low
- **Interrupt:** Enabled

#### DMA1 Channel 5 (TX)

**Purpose:** Send dummy bytes to generate SPI clock

**Configuration:**
- **Direction:** Memory to Peripheral
- **Peripheral:** SPI2_DR register
- **Memory:** `tx_dummy[4]` (static array of 0x00)
- **Peripheral Increment:** Disabled
- **Memory Increment:** Enabled
- **Data Alignment:** Byte (8-bit)
- **Transfer Mode:** Normal
- **Priority:** Highest (to ensure clock generation)
- **Interrupt:** Enabled

---

## Implementation

### Code Location

File: `Core/Src/dma.c`

### DMA Initialization Code

```c
/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 9, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 13, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
  
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 9, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
}
```

### Logic Explanation

1. **Clock Enable:** First, the DMA1 peripheral clock is enabled using the HAL macro
2. **Interrupt Priorities:**
   - Channel 5 (SPI2 TX): Highest priority (0) - ensures clock generation
   - Channels 2 & 4 (RX): Medium priority (9) - data reception
   - Channel 3 (SPI1 TX): Lowest priority (13) - data transmission
3. **NVIC Configuration:** Each DMA channel interrupt is registered with the Nested Vectored Interrupt Controller

**Note:** Individual channel configurations (source, destination, size, etc.) are set up in `spi.c` during `HAL_SPI_MspInit()`.

### DMA Channel Setup in SPI Module

File: `Core/Src/spi.c`

Example for SPI1 RX (full code in [SPI Peripheral Documentation](SPI_peripheral.md)):

```c
/* SPI1_RX Init */
hdma_spi1_rx.Instance = DMA1_Channel2;
hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
hdma_spi1_rx.Init.Mode = DMA_NORMAL;
hdma_spi1_rx.Init.Priority = DMA_PRIORITY_LOW;
if (HAL_DMA_Init(&hdma_spi1_rx) != HAL_OK) {
    Error_Handler();
}

__HAL_LINKDMA(spiHandle, hdmarx, hdma_spi1_rx);
```

**Key Points:**
- `Instance`: Specifies which DMA channel to use
- `Direction`: Peripheral→Memory for RX, Memory→Peripheral for TX
- `PeriphInc`: Always disabled (SPI_DR address is fixed)
- `MemInc`: Enabled to fill/read buffer sequentially
- `Mode`: NORMAL (one-shot transfer, not circular)
- `__HAL_LINKDMA`: Links DMA handle to SPI handle

---

## Timing Diagrams

### SPI Full-Duplex Transfer with DMA

```
CPU Starts Transfer
    │
    ▼
┌───────────────┐
│ CS Pin LOW    │
└───────┬───────┘
        │
        ▼
┌────────────────────────────────────────────────┐
│  HAL_SPI_TransmitReceive_DMA()                 │
│  - Sets up TX DMA from tx_buffer               │
│  - Sets up RX DMA to rx_buffer                 │
│  - Enables SPI peripheral                      │
└────────┬───────────────────────────────────────┘
         │
         ▼
    DMA Active
         │
    ┌────┴─────┐
    │  CPU is  │
    │   FREE   │ ← Can do other work
    └────┬─────┘
         │
         │ (Hardware transfers data)
         │
         ▼
┌────────────────────┐
│ Transfer Complete  │
│ Interrupt Fires    │
└────────┬───────────┘
         │
         ▼
┌───────────────────────────┐
│ HAL_SPI_TxRxCpltCallback  │
│ - CS Pin HIGH             │
│ - Store data              │
│ - Clear busy flag         │
└───────────────────────────┘
```

### Byte-Level Transfer Timing

```
  SCK  ___/‾‾‾\___/‾‾‾\___/‾‾‾\___/‾‾‾\___
       
  MOSI ─┬───────┬───────┬───────┬───────
        │ Bit 7 │ Bit 6 │ Bit 5 │ Bit 4 │ ...
        └───────┴───────┴───────┴───────

  MISO ─┬───────┬───────┬───────┬───────
        │ Bit 7 │ Bit 6 │ Bit 5 │ Bit 4 │ ...
        └───────┴───────┴───────┴───────
        
        ▲                               ▲
    DMA writes                      DMA reads
    TX_Buffer[n]                    to RX_Buffer[n]
    to SPI_DR                       from SPI_DR
```

**Simultaneous Operation:**
- TX DMA continuously feeds SPI_DR with outgoing data
- RX DMA continuously reads incoming data from SPI_DR
- SPI hardware shifts data in/out on each clock pulse

---

## Code Explanation

### Initialization Sequence

1. **Call from main():**
   ```c
   MX_DMA_Init();  // Enable DMA clock and interrupts
   MX_SPI1_Init(); // Configure SPI1 and link DMA channels
   MX_SPI2_Init(); // Configure SPI2 and link DMA channels
   ```

2. **DMA remains idle until explicitly started**

3. **Starting a transfer (example from sensing.c):**
   ```c
   void max31865_read(void) {
       if (max31865_busy_flag == 0) {
           max31865_busy_flag = 1;
           HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
           HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buf_31865, rx_buf_31865, MAX31865_DATA_SIZE);
       }
   }
   ```

4. **DMA transfer completes asynchronously:**
   - Hardware performs transfer
   - Interrupt fires upon completion
   - `HAL_SPI_TxRxCpltCallback()` executes (in `sensing.c`)

### Interrupt Handling

DMA interrupts are handled internally by the HAL:

```
DMA Interrupt → HAL_DMA_IRQHandler() → HAL_SPI_TxRxCpltCallback()
```

The user-defined callback in `sensing.c`:

```c
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
        // Store data and update index
        max31865_busy_flag = 0;
    } else if (hspi->Instance == SPI2) {
        HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
        // Store data and update index
        max31855_busy_flag = 0;
    }
}
```

**Logic:**
- De-assert CS pin (end of transfer)
- Copy received data to averaging buffer
- Clear busy flag (allow next transfer)

---

## References

### STM32 Documentation

- [STM32F1 Reference Manual](Reference_Manual.pdf) - Chapter 13: DMA Controller
- [STM32F1 Programming Manual](Programming_Manual.pdf) - DMA usage guidelines
- [STM32F1 HAL/LL Drivers](STM32f1_HAL_LL_Drivers.pdf) - HAL_DMA API reference

### Related Project Documentation

- [System Architecture](System_Architecture.md) - Overall system design
- [SPI Peripheral](SPI_peripheral.md) - SPI configuration and DMA linkage
- [Sensing Module](sensing_module.md) - High-level DMA usage in sensor reading

---

**Document Version:** 1.0  
**Last Updated:** January 2026  
**Author:** Taha Pournik
