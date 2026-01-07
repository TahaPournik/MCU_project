# LCD Module Documentation

**Project:** Temperature Monitoring System  
**Author:** Taha Pournik  
**Version:** 1.0

---

## Table of Contents

1. [Overview](#overview)
2. [Hardware Interface](#hardware-interface)
3. [4-Bit Mode Operation](#4-bit-mode-operation)
4. [Implementation Code](#implementation-code)
5. [API Functions](#api-functions)
6. [Initialization Sequence](#initialization-sequence)
7. [References](#references)

---

## Overview

The LCD module provides a simple driver for **16x2 character LCD displays** using the **LM016L controller**. The driver operates in **4-bit parallel mode** to minimize GPIO usage, requiring only 6 GPIO pins for full control.

### Module Features

- **Display Type:** 16x2 character LCD (HD44780 compatible)
- **Interface Mode:** 4-bit parallel
- **GPIO Pins Used:** 6 (RS, E, D4-D7)
- **Control Method:** Software bit-banging
- **Update Method:** Polling (HAL_Delay for timing)

### Pin Summary

| Function | STM32 Pin | Description |
|----------|-----------|-------------|
| **RS** | PC14 | Register Select (0=Command, 1=Data) |
| **E** | PC15 | Enable (falling edge latches data) |
| **D4** | PA0 | Data bit 4 (LSB in 4-bit mode) |
| **D5** | PA1 | Data bit 5 |
| **D6** | PA2 | Data bit 6 |
| **D7** | PA3 | Data bit 7 (MSB in 4-bit mode) |

---

## Hardware Interface

### LCD Pin Configuration

A typical 16x2 LCD has 16 pins. In 4-bit mode, we use:

| LCD Pin | Name | Connection | Description |
|---------|------|------------|-------------|
| 1 | VSS | GND | Ground |
| 2 | VDD | +5V | Power supply |
| 3 | VEE | +5V | Contrast adjustment |
| 4 | RS | PB4 | Register select |
| 5 | R/W | GND | Read/Write (GND = Write only) |
| 6 | E | PB5 | Enable signal |
| 7-10 | D0-D3 | Not connected | Lower data bits (unused in 4-bit mode) |
| 11 | D4 | PB6 | Data bit 4 |
| 12 | D5 | PB7 | Data bit 5 |
| 13 | D6 | PB8 | Data bit 6 |
| 14 | D7 | PB9 | Data bit 7 |

### Connection Diagram

```
STM32F103C8                    16x2 LCD (HD44780)
┌──────────┐               ┌──────────────┐
│          │               │              │
|       PA0├──────────────►│ D4           │
|       PA1├──────────────►│ D5           │
|       PA2├──────────────►│ D6           │
|       PA3├──────────────►│ D7           │
│          │               │              │
|      PC14├──────────────►│ RS           │
│      PC15├──────────────►│ E            │
│          │               │              │
│       GND├──────────────►│ R/W, VSS     │
|       +5V├──────────────►│ VDD          │
│          │               │              │
└──────────┘               └──────────────┘

```

---

## 4-Bit Mode Operation

### Why 4-Bit Mode?

In **8-bit mode**, the LCD requires 11 GPIO pins (RS, E, D0-D7, R/W can be tied to GND). In **4-bit mode**, only the upper 4 data lines (D4-D7) are used, reducing GPIO usage to just 6 pins.

### Data Transfer in 4-Bit Mode

Each byte is sent in **two nibbles** (4 bits each):

```
8-bit byte: 0xB3 = 0b1011 0011
                    │    │
                    │    └─ Lower nibble (0x3)
                    └────── Upper nibble (0xB)
```

**Transfer sequence:**
1. Send **upper nibble** (bits 7-4) on D7-D4
2. Pulse Enable pin (E) to latch
3. Send **lower nibble** (bits 3-0) on D7-D4
4. Pulse Enable pin (E) to latch

### Timing Diagram

```
RS   ──────────────────────────────────  (0=Command, 1=Data)

E    ‾‾‾‾\_____/‾‾‾‾\_____/‾‾‾‾  (Pulse to latch)
           ↑           ↑
        Latch H     Latch L

D7-D4  ───< High Nibble >───< Low Nibble >───

         Send 0xB3:
         Step 1: D7-D4 = 0xB (1011)
         Step 2: Pulse E
         Step 3: D7-D4 = 0x3 (0011)
         Step 4: Pulse E
```

---

## Implementation Code

### File Location

Source: `Core/Src/lcd.c`  
Header: `Core/Inc/lcd.h`

### Private Helper Functions

#### 1. Enable Pulse Generation

```c
/**
 * @brief  Generates a pulse on the EN pin to latch data.
 * @retval None
 */
static void LCD_EnablePulse(void) {
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(1);  // Hold enable high for 1ms
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);  // Wait before next operation
}
```

**Logic:** The HD44780 latches data/commands on the **falling edge** of the Enable pin. A minimum pulse width of ~450ns is required; we use 1ms for safety with software delays.

#### 2. Send 4-Bit Nibble

```c
/**
 * @brief  Sends 4 bits of data to the LCD.
 * @param  val Lower nibble contains the 4 bits to send
 * @retval None
 */
static void LCD_Send4Bit(uint8_t val) {
    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (val >> 0) & 0x01);
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (val >> 1) & 0x01);
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (val >> 2) & 0x01);
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (val >> 3) & 0x01);
    LCD_EnablePulse();
}
```

**Logic:** Extracts bits 0-3 from `val` and writes them to GPIO pins D4-D7, then pulses Enable to latch the data.

### Public API Functions

#### 1. Send Command

```c
/**
 * @brief  Sends a command to the LCD.
 * @param  cmd Command byte
 * @retval None
 */
void LCD_Command(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);  // RS=0 for command
    LCD_Send4Bit(cmd >> 4);      // Send upper nibble
    LCD_Send4Bit(cmd & 0x0F);    // Send lower nibble
}
```

**Logic:** 
- Set RS=0 (command mode)
- Send high nibble (bits 7-4)
- Send low nibble (bits 3-0)

#### 2. Send Data (Character)

```c
/**
 * @brief  Sends data to the LCD.
 * @param  data Data byte
 * @retval None
 */
void LCD_Data(uint8_t data) {
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);  // RS=1 for data
    LCD_Send4Bit(data >> 4);     // Send upper nibble
    LCD_Send4Bit(data & 0x0F);   // Send lower nibble
}
```

**Logic:**
- Set RS=1 (data mode)
- Send character in two 4-bit transfers

#### 3. LCD Initialization

```c
/**
 * @brief  Initializes the LCD in 4-bit mode.
 * @retval None
 */
void LCD_Init(void) {
    HAL_Delay(50);  // Wait for LCD to power up (>40ms required)
    
    /* Initialization sequence for 4-bit mode */
    LCD_Send4Bit(0x03);  // Function set (8-bit mode)
    HAL_Delay(5);
    LCD_Send4Bit(0x03);  // Function set (8-bit mode)
    HAL_Delay(1);
    LCD_Send4Bit(0x03);  // Function set (8-bit mode)
    HAL_Delay(1);
    LCD_Send4Bit(0x02);  // Set to 4-bit mode
    
    /* Configure LCD parameters */
    LCD_Command(LCD_CMD_FUNCTION_SET);      // 0x28: 4-bit, 2 lines, 5x8 font
    LCD_Command(LCD_CMD_DISPLAY_CONTROL);   // 0x0C: Display ON, Cursor OFF
    LCD_Command(LCD_CMD_CLEAR_DISPLAY);     // 0x01: Clear screen
    HAL_Delay(2);
    LCD_Command(LCD_CMD_ENTRY_MODE_SET);    // 0x06: Increment cursor, no shift
}
```

#### 4. Print String

```c
/**
 * @brief  Prints a string to the LCD.
 * @param  str String to be printed
 * @retval None
 */
void LCD_Print(char *str) {
    while (*str) {
        LCD_Data(*str++);
    }
}
```

**Logic:** Iterates through the string and sends each character using `LCD_Data()`.

#### 5. Set Cursor Position

```c
/**
 * @brief  Sets the cursor to a specific row and column.
 * @param  row LCD row (0 or 1)
 * @param  col LCD column (0 to 15)
 * @retval None
 */
void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0) ? col : (0x40 + col);
    LCD_Command(LCD_CMD_SET_DDRAM_ADDR | addr);
}
```

**Logic:**
- Row 0: DDRAM address 0x00-0x0F
- Row 1: DDRAM address 0x40-0x4F
- Command 0x80 | address sets DDRAM address

#### 6. Clear Display

```c
/**
 * @brief  Clears the LCD screen.
 * @retval None
 */
void LCD_Clear(void) {
    LCD_Command(LCD_CMD_CLEAR_DISPLAY);
    HAL_Delay(2);  // Clear command takes ~1.64ms
}
```

---

## API Functions

### Function Summary

| Function | Parameters | Description |
|----------|------------|-------------|
| `LCD_Init()` | None | Initialize LCD in 4-bit mode |
| `LCD_Command()` | `uint8_t cmd` | Send command to LCD |
| `LCD_Data()` | `uint8_t data` | Send character data to LCD |
| `LCD_Print()` | `char *str` | Print string at current cursor |
| `LCD_SetCursor()` | `uint8_t row, col` | Move cursor to (row, col) |
| `LCD_Clear()` | None | Clear screen and home cursor |

### Common LCD Commands (from lcd.h)

```c
#define LCD_CMD_CLEAR_DISPLAY      0x01  // Clear entire display
#define LCD_CMD_RETURN_HOME        0x02  // Return cursor to (0,0)
#define LCD_CMD_ENTRY_MODE_SET     0x06  // Increment cursor right
#define LCD_CMD_DISPLAY_CONTROL    0x0C  // Display ON, cursor OFF, blink OFF
#define LCD_CMD_FUNCTION_SET       0x28  // 4-bit mode, 2 lines, 5x8 font
#define LCD_CMD_SET_DDRAM_ADDR     0x80  // Set DDRAM address (OR with address)
```

### Usage Example (from main.c)

```c
/* Initialize LCD */
LCD_Init();

/* Display welcome message */
LCD_Clear();
LCD_SetCursor(0, 0);
LCD_Print("Temp Monitor");
LCD_SetCursor(1, 0);
LCD_Print("v1.0");
HAL_Delay(2000);

/* Main loop: Update temperature display */
while (1) {
    LCD_Clear();
    LCD_SetCursor(0, 0);
    sprintf(buffer, "AMB:%.2f C", current_ambient_temp);
    LCD_Print(buffer);
    
    LCD_SetCursor(1, 0);
    sprintf(buffer, "FURN:%d C", (int)current_furnace_temp);
    LCD_Print(buffer);
    
    HAL_Delay(1000);
}
```

---

## Initialization Sequence

The HD44780 requires a specific initialization sequence to enter 4-bit mode:

### Step-by-Step Process

```
Step 1: Power-up delay
    ├─ Wait 50ms for LCD internal circuits to stabilize
    
Step 2: Function set (8-bit interface)
    ├─ Send 0x03 (8-bit mode command)
    ├─ Wait 5ms
    ├─ Send 0x03 again
    ├─ Wait 1ms
    └─ Send 0x03 third time
    
Step 3: Switch to 4-bit mode
    ├─ Send 0x02 (4-bit mode switch)
    
Step 4: Configure display
    ├─ 0x28: Function set (4-bit, 2 lines, 5x8 font)
    ├─ 0x0C: Display control (ON, no cursor, no blink)
    ├─ 0x01: Clear display
    ├─ Wait 2ms (clear takes longer)
    └─ 0x06: Entry mode (increment, no shift)
```

### Why This Sequence?

The triple 0x03 command ensures the LCD enters a known state regardless of whether it was previously in 4-bit or 8-bit mode. This is a **reset by instruction** as recommended by the HD44780 datasheet.

---

## References

### LCD Controller Documentation

- **HD44780 Datasheet** - Standard character LCD controller
  - Initialization sequence: Page 46
  - Command table: Page 24-25
  - 4-bit interface timing: Page 58

### Related Project Documentation

- [System Architecture](System_Architecture.md) - Overall system design and LCD integration
- [GPIO Configuration](../Core/Inc/gpio.h) - Pin definitions for LCD interface

### Online Resources

- [HD44780 Tutorial](https://www.sparkfun.com/datasheets/LCD/HD44780.pdf)
- [4-bit vs 8-bit mode explanation](https://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller)

---

**Document Version:** 1.0  
**Last Updated:** January 2026  
**Author:** Taha Pournik
