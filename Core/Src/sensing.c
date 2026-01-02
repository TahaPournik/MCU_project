#include "sensing.h"
#include "main.h"
#include "spi.h"
#include "stm32f103xb.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_adc.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_spi.h"
#include "lcd.h"
#include <stdint.h>
#include <math.h>

/* Volatile variables for DMA and readings */
volatile uint8_t ambient_raw_val[8];
volatile uint8_t furnace_raw_val[5];
volatile float current_ambient_temp = 0.0f;
volatile int32_t current_furnace_temp = 0;
volatile uint8_t furnace_error = 0;
volatile uint8_t ambient_error = 0;
volatile uint8_t status = 0;
volatile float thermo_calib = 1.0f;
volatile float pt100_calib = -0.137f;
volatile uint32_t last_sensor_tick = 0; // Watchdog for sensor chain
#define SENSOR_TIMEOUT_MS 500  // If no update in 500ms, restart chain
// status bits
#define STATUS_SPI_ERROR (1 << 0)
#define STATUS_SPI_BUSY (1 << 1) 
#define STATUS_SPI_TIMEOUT (1 << 2)
#define STATUS_AMBIENT_ERROR (1 << 3)
#define STATUS_FURNACE_ERROR (1 << 4)
#define STATUS_AMBIENT_READING (1 << 5)
#define STATUS_FURNACE_READING (1 << 6)



/**
  * @brief Initializes the sensing peripheral
  */



void Sensing_Init(void) {

  HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
  uint8_t init_data[2] = {0x80, 0b11000010,};
  HAL_SPI_Transmit(&hspi1, init_data, 2, 100);

}

// void furnace_dma_read(){
//   HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
//   HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_RESET);
//   HAL_SPI_Receive_DMA(&hspi1, (uint8_t*)furnace_raw_val, 4);
// }

void furnace_read(){
  // check if spi is busy
  if (status & STATUS_SPI_BUSY) return;
  
  status |= (STATUS_SPI_BUSY | STATUS_FURNACE_READING);
    
  // reset max31855 cs pin (PB12)
  HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_RESET);
    
  // read furnace (4 bytes) using DMA on SPI2
  if(HAL_SPI_Receive_DMA(&hspi2, (uint8_t*)furnace_raw_val, 4) != HAL_OK) {
     // If DMA fails to start, reset status
     status &= ~(STATUS_SPI_BUSY | STATUS_FURNACE_READING);
     HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
  }
}

void ambient_read(){
    if (status & STATUS_SPI_BUSY) return;
    status |= (STATUS_SPI_BUSY | STATUS_AMBIENT_READING);

    // reset max31865 cs pin (PA4)
    HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);

    // read ambient (8 bytes)
    // First byte is read address/command (0x00). 
    uint8_t data = 0x00;
    // We send command in blocking (fast enough) then read via DMA
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
    
    if(HAL_SPI_Receive_DMA(&hspi1, (uint8_t*)ambient_raw_val, 8) != HAL_OK) {
       status &= ~(STATUS_SPI_BUSY | STATUS_AMBIENT_READING);
       HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
    }
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
  // Case 1: MAX31865 (Ambient) on SPI1
  if (hspi->Instance == SPI1) {
      // Release CS
      HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
      status &= ~(STATUS_SPI_BUSY | STATUS_AMBIENT_READING);

      // --- Data Processing for MAX31865 ---
      // ambient_raw_val[0] = Config, [1]=RTD MSB, [2]=RTD LSB
      int16_t ambient_raw = ((ambient_raw_val[1] << 8) | ambient_raw_val[2]) >> 1;
      
      // Check Fault Bit (D0)
      if (ambient_raw_val[2] & 0x01) {
          status |= STATUS_AMBIENT_ERROR;
          ambient_error = ambient_raw_val[7]; // Fault Status Register
      } else {
          status &= ~STATUS_AMBIENT_ERROR;
          ambient_error = 0;
          
          // Simple Linear Conversion (Callendar-Van Dusen is better but costly)
          // R_RTD = (ADC * R_REF) / 32768
          // T = (R_RTD - R0) / (R0 * alpha)
          // Simplified: T = (ADC * 0.0317) - 259.7 (for PT100, 430Gb Rref)
          current_ambient_temp = ((ambient_raw * 0.0317f) - 259.7f + pt100_calib);
      }

      // Chain: Start Furnace Read
      furnace_read();
  }
  
  // Case 2: MAX31855 (Furnace) on SPI2
  else if (hspi->Instance == SPI2) {
      // Release CS
      HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
      status &= ~(STATUS_SPI_BUSY | STATUS_FURNACE_READING);

      // --- Data Processing for MAX31855 ---
      // 32-bit Frame: [31:18] Temp, [16] Fast, [15:4] Internal Temp
      uint32_t furnace_raw = ((uint32_t)furnace_raw_val[0] << 24) | 
                             ((uint32_t)furnace_raw_val[1] << 16) | 
                             ((uint32_t)furnace_raw_val[2] << 8)  | 
                             (uint32_t)furnace_raw_val[3];
      
      // Check for Errors (Bit 16 or Bits 2-0)
      if (furnace_raw & 0x10007) { // D16=Fault, D2=SCV, D1=SCG, D0=OC
          status |= STATUS_FURNACE_ERROR;
          furnace_error = (furnace_raw & 0x07); 
      } else {
          status &= ~STATUS_FURNACE_ERROR;
          furnace_error = 0;
          
          // Extract Thermocouple Temperature (14-bit signed, D31-D18)
          int32_t termo_val = (int32_t)furnace_raw >> 18; 
          
          // Extract Internal Reference Temperature (12-bit signed, D15-D4)
          int32_t intemp_val = (int32_t)((furnace_raw >> 4) & 0xFFF);
          if (intemp_val & 0x800) intemp_val |= 0xFFFFF000; // Sign extend

          // Calc T = Thermo * 0.25 - (Internal * alpha) + Offset
          // Note: Specific formula depends on usage, simplified here
          current_furnace_temp = (int32_t)roundf((termo_val * 0.25f) + thermo_calib);
          // (We usually don't subtract internal temp for K-Type raw value, 
          // MAX31855 outputs compensated temp in D31-D18)
      }

      // Chain: Start Ambient Read
      ambient_read();
  }
  
  // Update watchdog
  last_sensor_tick = HAL_GetTick();
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
  // Handle SPI1 Error (Ambient)
  if (hspi->Instance == SPI1) {
      HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
      status &= ~(STATUS_SPI_BUSY | STATUS_AMBIENT_READING);
      status |= STATUS_AMBIENT_ERROR;
      
      // Try to recover chain by skipping to next
      furnace_read();
  }
  // Handle SPI2 Error (Furnace)
  else if (hspi->Instance == SPI2) {
      HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
      status &= ~(STATUS_SPI_BUSY | STATUS_FURNACE_READING);
      status |= STATUS_FURNACE_ERROR;
      
      // Try to recover chain by skipping to next
      ambient_read();
  }
  
  last_sensor_tick = HAL_GetTick();
}