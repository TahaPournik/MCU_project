#ifndef __LCD_H
#define __LCD_H

#include "stm32f1xx_hal.h"
#include "main.h"



// LCD Commands
#define LCD_CMD_CLEAR_DISPLAY   0x01
#define LCD_CMD_RETURN_HOME     0x02
#define LCD_CMD_ENTRY_MODE_SET  0x06
#define LCD_CMD_DISPLAY_CONTROL 0x0C
#define LCD_CMD_CURSOR_SHIFT    0x14
#define LCD_CMD_FUNCTION_SET    0x28 // 4-bit, 2-line, 5x8 dots
#define LCD_CMD_SET_CGRAM_ADDR  0x40
#define LCD_CMD_SET_DDRAM_ADDR  0x80

// Function Prototypes
void LCD_Init(void);
void LCD_Command(uint8_t cmd);
void LCD_Data(uint8_t data);
void LCD_Print(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Clear(void);

#endif /* __LCD_H */
