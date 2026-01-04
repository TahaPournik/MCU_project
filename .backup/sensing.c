/**
 ******************************************************************************
 * @file           : sensing.c
 * @brief          : Source file for sensor data acquisition (MAX31865 & MAX31855)
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "sensing.h"
#include "lcd.h"
#include "main.h"
#include "spi.h"
#include "gpio.h"
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_def.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_spi.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define MAX31865_DATA_SIZE 8
#define MAX31855_DATA_SIZE 4  // MAX31855 is usually 32-bit (4 bytes)

/* Private variables ---------------------------------------------------------*/
volatile Sensing_Data_t sensor_data = {0.0f, -999};

// DMA Buffers for MAX31865
 uint8_t rx_buf_31865[9]; // Extra byte for safety
 uint8_t tx_buf_31865[9] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
 uint8_t config_data_31865[9] = {0x80, 0xc1, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00};

// DMA Buffer for MAX31855
volatile uint8_t rx_buf_31855[MAX31855_DATA_SIZE + 1]; // Extra byte for safety

// Callibration variables
volatile float max31865_callibration = 0.0f;
volatile float max31855_callibration = 0.75f;

/* Private function prototypes -----------------------------------------------*/
static void config_max31865(void);

/* Public Functions ----------------------------------------------------------*/



/**
 * @brief Initializes the sensing module and configures the sensors.
 */
void sensing_init(void) {
  MX_SPI1_Init();
  MX_SPI2_Init();
  
  HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);

  //configure max31865
  HAL_Delay(200);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_Delay(200);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
  HAL_Delay(200);
  HAL_SPI_TransmitReceive(&hspi1, tx_buf_31865, rx_buf_31865, 9, 100);
  HAL_Delay(200);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_Delay(1000);
  
   // wait for the sensor to be ready
}

/**
 * @brief Starts a non-blocking DMA read for the MAX31865 sensor.
 */
void max31865_read(void) {
    // Pull CS low to select the chip
    HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
    HAL_Delay(200);
    // Start DMA transfer
    HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buf_31865, rx_buf_31865, 9);
}

/**
 * @brief Starts a non-blocking DMA read for the MAX31855 sensor.
 */
void max31855_read(void) {
    // Clear any potential Overrun flag from previous noisy states
    __HAL_SPI_CLEAR_OVRFLAG(&hspi2);
    
    // Pull CS low to select the chip
    HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_RESET);
    
    // Start DMA receive (MAX31855 is read-only)
    uint8_t tx_buf_31855[4] = {0x00, 0x00, 0x00, 0x00};
    HAL_SPI_TransmitReceive_DMA(&hspi2, tx_buf_31855, rx_buf_31855, MAX31855_DATA_SIZE);

    // CS pin will be set high in HAL_SPI_RxCpltCallback

}

/**
 * @brief Callback function triggered when an SPI DMA transfer is complete.
 * @param hspi SPI handle
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        // Deselect MAX31865
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
        // process_max31865_data();
    }else if (hspi->Instance == SPI2) {
        // Deselect MAX31855
        HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
        process_max31855_data();
    }
}

/**
 * @brief Processes the raw data received from the MAX31865 sensor.
 */
void process_max31865_data(void) {
    if (rx_buf_31865[2] & 0x01) {
        max31865_ErrorCallback();
        return;
    }
    
    int32_t raw_data =  (rx_buf_31865[2] << 8) | (rx_buf_31865[3] );
    sensor_data.temp_max31865 =  (float)rx_buf_31865[1] ;
        // sensor_data.temp_max31865 =  (float)raw_data * 0.0317f - 259.7f;
}

/**
 * @brief Processes the raw data received from the MAX31855 sensor.
 */
void process_max31855_data(void) {
    // if (rx_buf_31855[2] & 0x01) {
    //     max31855_ErrorCallback();
    //     return;
    // }
    int32_t internal_temp = 0;
    int32_t thermocouple_temp = 0;
    if ((rx_buf_31855[2] & 0x80) >> 7) {
        internal_temp = -((rx_buf_31855[2] & 0b01111111) << 4) | (rx_buf_31855[3] >> 4);
    }else {
        internal_temp = ((rx_buf_31855[2] & 0b01111111) << 4) | (rx_buf_31855[3] >> 4);
    }
    if ((rx_buf_31855[0] & 0x80) >> 7) {
        thermocouple_temp = -((rx_buf_31855[0] & 0b01111111) << 6) | (rx_buf_31855[1] >> 2);
    }else {
        thermocouple_temp = ((rx_buf_31855[0] & 0b01111111) << 6) | (rx_buf_31855[1] >> 2);
    }
    
    // int16_t thermocouple_temp = ((rx_buf_31855[0] << 6) & 0x7f) | (rx_buf_31855[1] >> 2);
    sensor_data.temp_max31855 = (int32_t)roundf((thermocouple_temp * 0.25f) - (internal_temp * 0.0625f) + max31855_callibration);
    // sensor_data.temp_max31855 = external_temp;
}

/**
 * @brief Callback for SPI errors.
 * @param hspi SPI handle
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
        max31865_ErrorCallback();
    } else if (hspi->Instance == SPI2) {
        HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
        max31855_ErrorCallback();
    }
}

/**
 * @brief Error handler for MAX31865 communication issues.
 */
void max31865_ErrorCallback(void) {
    // Handle MAX31865 error
}

/**
 * @brief Error handler for MAX31855 communication issues.
 */
void max31855_ErrorCallback(void) {

}

/* Private Functions ---------------------------------------------------------*/

/**
 * @brief Configures the MAX31865 chip settings.
 */
static void config_max31865(void) {
    // uint8_t trash[9];
    HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive_DMA(&hspi1, config_data_31865, rx_buf_31865, 9);
    HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
}
