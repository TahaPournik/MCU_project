# Technical Specification: Industrial Heater Control Module (v2.0)
**Revision:** 2.0  
**Date:** 2026-01-01  
**Platform:** STM32F103C8T6 (Arm® Cortex®-M3)

---

## 1. Functional Overview
The Smart Heater Module is a high-reliability industrial controller designed for precise thermal management and safety-critical combustion control. The system architecture leverages DMA (Direct Memory Access) and Interrupt-driven logic to ensure safe, real-time operation without CPU blocking.

---

## 2. System Hardware Components

### 2.1 Temperature Sensing ICs
*   **MAX31865 (RTD-to-Digital Converter):**
    *   Interfaces with a high-precision PT100 probe.
    *   Configured in **DMA Mode** for continuous resistance-to-temperature conversion.
*   **MAX31855 (Thermocouple-to-Digital):**
    *   Interfaces with a K-Type Thermocouple for furnace monitoring.
    *   Configured in **DMA Mode** (Read-Only) with built-in cold-junction compensation and fault detection (Open/Short circuit).

### 2.2 Communication & Safety
*   **RS485 Transceiver:**
    *   Implements **Modbus RTU** protocol.
    *   Utilizes **DMA with Idle Line Detection** to process packets only upon frame completion.
*   **V/A Monitor Relay:**
    *   Hardware feedback loop for heater fan status.
    *   Connected via **EXTI (External Interrupt)** for sub-microsecond fail-safe response.

---

## 3. Peripheral Map & Wire Interconnects

| Pin | Identifier | Primary Function | Drive Mode | Purpose |
|-----|------------|------------------|------------|---------|
| **PA1** | RS485_DE | GPIO_Output | High-Speed PP | Direction Control (DE/RE) |
| **PA2** | RS485_TX | USART2_TX | AF_PP | Modbus Data Transmit |
| **PA3** | RS485_RX | USART2_RX | Input (Pull-up) | Modbus Data Receive |
| **PA4** | PT100_CS | GPIO_Output | Push-Pull | MAX31865 Slave Select |
| **PA5** | SCK1 | SPI1_SCK | AF_PP | PT100/Sensor Bus Clock |
| **PA6** | MISO1 | SPI1_MISO | Input | PT100 Data Input |
| **PA7** | MOSI1 | SPI1_MOSI | AF_PP | PT100 Data Output |
| **PA9** | MENU_BTN | EXTI9 | IT Falling | System Menu Access |
| **PA10**| ENTER_BTN | EXTI10 | IT Falling | Command Confirmation |
| **PB0** | VC_MONITOR| EXTI0 | **IT Falling** | **Fan Fault Monitor (Safety)** |
| **PB1** | GAS_RELAY | GPIO_Output | Push-Pull | Fuel Valve Control |
| **PB2** | FAN_RELAY | GPIO_Output | Push-Pull | Air Circulator Control |
| **PB12**| TC_CS | GPIO_Output | Push-Pull | MAX31855 Slave Select |
| **PB13**| SCK2 | SPI2_SCK | AF_PP | Thermocouple Bus Clock |
| **PB14**| MISO2 | SPI2_MISO | Input | Thermocouple Data Input |

---

## 4. Software Logic & Interrupt Architecture

### 4.1 Asynchronous Sensor Acquisition (DMA Chaining)
To ensure the SPI bus is utilized efficiently without CPU overhead, the sensors operate in a "Ping-Pong" circular sequence.
1.  **Trigger:** Timer or Main Loop starts the first DMA transfer.
2.  **Handoff:** Upon `DMA_RX_Complete`, the Interrupt Handler processes the data and immediately initiates the transfer for the next sensor.

`[INSERT CODE SAMPLE: DMA RX CALLBACK CHAINING]`

### 4.2 Safety-Critical Fan Monitoring (EXTI)
The `VC_MONITOR` is mapped to the highest priority interrupt (NVIC Priority 0).
*   **Logic:** Any hardware-detected failure in the fan voltage or current triggers a hard-reset of the Gas Valve and Ignition relays within nanoseconds.

`[INSERT CODE SAMPLE: EXTI SAFETY CALLBACK]`

### 4.3 Deterministic Modbus RTU (DMA + Idle IT)
The system uses Hardware Idle Line detection to minimize interrupt frequency.
*   The DMA fills a global buffer in the background.
*   The CPU is only interrupted when the line is silent for 3.5 character times (end of Modbus frame).

`[INSERT CODE SAMPLE: UART IDLE LINE DMA SETUP]`

### 4.4 Non-Blocking User Interface
*   **LCD (16x2):** Operated via a **Polling State Machine**. This method eliminates the need for `HAL_Delay()`, allowing the MCU to process safety logic even during display refreshes.
*   **Keypad:** Event-driven via EXTI. Non-menu buttons (Up/Down) are only polled when the menu state is active to conserve resources.

`[INSERT CODE SAMPLE: NON-BLOCKING LCD STATE MACHINE]`

---

## 5. NVIC Priority Matrix

| IRQ Source | Preemption Priority | Sub-priority | Logic Role |
|------------|---------------------|--------------|------------|
| **EXTI0 (VC Monitor)** | **0** | **0** | **Emergency Shutdown (Fan Fail)** |
| DMA Channels (SPI/UART) | 1 | 0 | High-Speed Data Transfer |
| Timer Interrupt (Relay IT) | 2 | 0 | Deterministic Relay Control |
| UART Global IT | 3 | 0 | Modbus Packet Processing |
| EXTI 9-10 (Buttons) | 4 | 0 | User Interface Interaction |

---

## 6. Engineering Requirements & Protection
1.  **Isolation:** Suggest Opto-isolation for all Relay control pins and Modbus RS485 lines.
2.  **Filtering:** 100nF decoupling capacitors must be placed < 5mm from MAX ICs.
3.  **Fail-Safe:** Internal Relay flags must be cleared on any System Reset or Watchdog event.

---
**Document Status:** Final Technical Proposal  
**Approved by:** Engineering Lead  