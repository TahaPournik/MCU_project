#include "lcd.h"

static void LCD_EnablePulse(void) {
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(LCD_EN_GPIO_Port, LCD_EN_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
}

static void LCD_Send4Bit(uint8_t val) {
    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, (val >> 0) & 0x01);
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, (val >> 1) & 0x01);
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, (val >> 2) & 0x01);
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, (val >> 3) & 0x01);
    LCD_EnablePulse();
}

void LCD_Command(uint8_t cmd) {
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_RESET);
    LCD_Send4Bit(cmd >> 4);
    LCD_Send4Bit(cmd & 0x0F);
}

void LCD_Data(uint8_t data) {
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, GPIO_PIN_SET);
    LCD_Send4Bit(data >> 4);
    LCD_Send4Bit(data & 0x0F);
}

void LCD_Init(void) {
    HAL_Delay(50); // Wait for LCD to power up
    
    // Initializing 4-bit mode
    LCD_Send4Bit(0x03);
    HAL_Delay(5);
    LCD_Send4Bit(0x03);
    HAL_Delay(1);
    LCD_Send4Bit(0x03);
    HAL_Delay(1);
    LCD_Send4Bit(0x02); // Set to 4-bit mode
    
    LCD_Command(LCD_CMD_FUNCTION_SET);
    LCD_Command(LCD_CMD_DISPLAY_CONTROL);
    LCD_Command(LCD_CMD_CLEAR_DISPLAY);
    HAL_Delay(2);
    LCD_Command(LCD_CMD_ENTRY_MODE_SET);
}

void LCD_Print(char *str) {
    while (*str) {
        LCD_Data(*str++);
    }
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0) ? col : (0x40 + col);
    LCD_Command(LCD_CMD_SET_DDRAM_ADDR | addr);
}

void LCD_Clear(void) {
    LCD_Command(LCD_CMD_CLEAR_DISPLAY);
    HAL_Delay(2);
}
