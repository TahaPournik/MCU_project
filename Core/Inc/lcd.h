/**
 ******************************************************************************
 * @file           : lcd.h
 * @brief          : Header file for 16x2 LCD (4-bit mode) driver
 ******************************************************************************
 */

#ifndef __LCD_H
#define __LCD_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "main.h"

/* Exported defines ----------------------------------------------------------*/
#define LCD_CMD_CLEAR_DISPLAY   0x01
#define LCD_CMD_RETURN_HOME     0x02
#define LCD_CMD_ENTRY_MODE_SET  0x06
#define LCD_CMD_DISPLAY_CONTROL 0x0C
#define LCD_CMD_CURSOR_SHIFT    0x14
#define LCD_CMD_FUNCTION_SET    0x28 /**< 4-bit, 2-line, 5x8 dots */
#define LCD_CMD_SET_CGRAM_ADDR  0x40
#define LCD_CMD_SET_DDRAM_ADDR  0x80

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief  Initializes the LCD in 4-bit mode.
 * @retval None
 */
void LCD_Init(void);

/**
 * @brief  Sends a command to the LCD.
 * @param  cmd Command to send
 * @retval None
 */
void LCD_Command(uint8_t cmd);

/**
 * @brief  Sends character data to the LCD.
 * @param  data Character data to display
 * @retval None
 */
void LCD_Data(uint8_t data);

/**
 * @brief  Prints a string on the LCD.
 * @param  str Null-terminated string
 * @retval None
 */
void LCD_Print(char *str);

/**
 * @brief  Sets the cursor position.
 * @param  row LCD row (0 or 1)
 * @param  col LCD column (0 to 15)
 * @retval None
 */
void LCD_SetCursor(uint8_t row, uint8_t col);

/**
 * @brief  Clears the LCD display.
 * @retval None
 */
void LCD_Clear(void);

#endif /* __LCD_H */
