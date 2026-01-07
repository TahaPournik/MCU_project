# Temperature Monitoring System with STM32F103C8

**Author:** Taha Pournik  
**Version:** 1.0  
**License:** GNU General Public License v3.0

## Overview

This project implements a dual-sensor temperature monitoring system using the STM32F103C8 microcontroller. The system reads ambient temperature via a PT100 RTD sensor (MAX31865) and furnace temperature via a K-type thermocouple (MAX31855), displaying both readings on a 16x2 LCD display in real-time.

## Features

- **Dual Temperature Sensing:**
  - Ambient temperature measurement using PT100 RTD with MAX31865
  - Furnace temperature measurement using K-type thermocouple with MAX31855
- **DMA-based SPI Communication:** Non-blocking, efficient data acquisition
- **Real-time Display:** 16x2 LCD showing both temperature readings
- **Error Handling:** Comprehensive fault detection and error reporting
- **Moving Average Filter:** 5-sample averaging for noise reduction
- **Hardware Fault Detection:** Open circuit, short circuit, and voltage fault detection

## Hardware Requirements

- **Microcontroller:** STM32F103C8T6 (Blue Pill)
- **Sensors:**
  - MAX31865 RTD-to-Digital Converter with PT100 sensor
  - MAX31855 Thermocouple-to-Digital Converter with K-type thermocouple
- **Display:** 16x2 Character LCD (HD44780 compatible) in 4-bit mode
- **Power Supply:** 5V DC
- **Connections:** See [System Architecture](docs/System_Architecture.md)

## Software Architecture

The project follows a modular architecture:

```
MCU_project/
├── Core/
│   ├── Inc/              # Header files
│   │   ├── main.h
│   │   ├── sensing.h     # Sensor module interface
│   │   ├── lcd.h         # LCD driver interface
│   │   ├── spi.h         # SPI peripheral config
│   │   ├── dma.h         # DMA peripheral config
│   │   ├── tim.h         # Timer peripheral config
│   │   └── gpio.h        # GPIO configuration
│   └── Src/              # Source files
│       ├── main.c        # Main application logic
│       ├── sensing.c     # Sensor data acquisition
│       ├── lcd.c         # LCD driver implementation
│       ├── spi.c         # SPI initialization
│       ├── dma.c         # DMA initialization
│       └── tim.c         # Timer with sampling logic
├── Drivers/              # STM32 HAL drivers
├── docs/                 # Documentation
└── simulation/           # Proteus simulation files
```

## Toolchain

This project uses the following development tools:

- **IDE:** Visual Studio Code
- **Build System:** CMake (v3.22+)
- **Compiler:** ARM GNU Toolchain (arm-none-eabi-gcc)
- **Debugger:** OpenOCD / ST-Link
- **HAL Library:** STM32Cube HAL for STM32F1 series
- **Configuration Tool:** STM32CubeMX (for peripheral initialization)
- **Simulation:** Proteus 9.0 SP2 Professional (for circuit simulation)

### Build Instructions

```bash
# Configure the project
cmake -B build -G "Ninja"

# Build the project
cmake --build build

# Flash to device (using ST-Link)
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/heater_module.elf verify reset exit"
```

## Documentation

Detailed documentation is available in the `docs/` directory:

- **[System Architecture](docs/System_Architecture.md)** - Overall system design and hardware connections
- **[DMA Peripheral](docs/DMA_peripheral.md)** - DMA configuration and usage
- **[SPI Peripheral](docs/SPI_peripheral.md)** - SPI communication setup
- **[TIM Peripheral](docs/TIM_peripheral.md)** - Timer configuration for sampling
- **[LCD Module](docs/lcd_module.md)** - LCD driver implementation
- **[Sensing Module](docs/sensing_module.md)** - Sensor data acquisition logic

## Reference Materials

Available in the `docs/` directory:

- [STM32F103C8 Datasheet](docs/STM32f103c8.pdf)
- [STM32F1 Reference Manual](docs/Reference_Manual.pdf)
- [STM32F1 Programming Manual](docs/Programming_Manual.pdf)
- [STM32F1 HAL/LL Drivers Documentation](docs/STM32f1_HAL_LL_Drivers.pdf)
- [MAX31865 Datasheet](docs/max31865.pdf)
- [MAX31855 Datasheet](docs/max31855.pdf)

## Simulation

The project includes a complete Proteus simulation:

- **File:** [simulation/heater_module.pdsprj](simulation/heater_module.pdsprj)
- **Circuit Diagram:** [simulation/heater_module.svg](simulation/heater_module.svg)

Open the `.pdsprj` file in Proteus to run the simulation. The simulation includes virtual sensors and displays to verify the system operation.

## Project Structure Details

### Main Components

1. **Sensing Module** (`sensing.c/h`):
   - Manages MAX31865 and MAX31855 sensor communication
   - Implements DMA-based SPI transfers
   - Performs data processing and calibration
   - Handles error detection and fault reporting

2. **LCD Module** (`lcd.c/h`):
   - 4-bit parallel interface driver for LM016L-compatible displays
   - Provides simple API for text display

3. **Timer Module** (`tim.c`):
   - Generates periodic interrupts (500ms) for sensor sampling
   - Sequences sensor readings to prevent bus contention

4. **SPI/DMA Configuration** (`spi.c`, `dma.c`):
   - Configures two SPI peripherals for sensor communication
   - Sets up DMA channels for non-blocking data transfer

## Operation Principle

1. **Initialization Phase:**
   - Configure all peripherals (GPIO, SPI, DMA, TIM)
   - Initialize sensors and LCD
   - Start periodic timer

2. **Sampling Phase (every 500ms):**
   - Timer interrupt triggers sensor reading sequence
   - Alternates between MAX31855 and MAX31865 readings
   - Collects 5 samples for each sensor

3. **Processing Phase:**
   - Calculate moving average from collected samples
   - Apply calibration offsets
   - Convert raw ADC values to temperature
   - Detect and report faults

4. **Display Phase (every 1 seconds):**
   - Update LCD with current temperature readings
   - Format: Line 1: "AMB:25.50" (ambient), Line 2: "FURN:450 C" (furnace)

## Calibration

Temperature offsets can be adjusted in `sensing.c`:

```c
float max31865_calibration = 0.0f;   // PT100 offset in °C
float max31855_calibration = 0.75f;  // Thermocouple offset in °C
```

## Error Handling

The system includes comprehensive error detection:

- **MAX31865 Faults:** Over/under voltage, RTD open/short circuit, reference faults
- **MAX31855 Faults:** Thermocouple open circuit, short to GND/VCC
- **SPI Communication Errors:** Timeout, overrun, mode faults

Errors are displayed on the LCD as "ERR:0xXX" with diagnostic codes.

## License

This project is licensed under the GNU General Public License v3.0. See the full license text below:

```
Copyright (C) 2026 Taha Pournik

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

## Contributing

This is a university course project. While it's open source, it's primarily for educational purposes.

## Contact

**Author:** Taha Pournik  
**Project:** MCU Course Project - Temperature Monitoring System

---

*Last updated: January 2026*
