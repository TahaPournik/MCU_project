/**
 ******************************************************************************
 * @file           : sensing.c
 * @brief          : Source file for sensor data acquisition (MAX31865 & MAX31855)
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "sensing.h"
#include "spi.h"
#include "gpio.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define MAX31865_DATA_SIZE 9
#define MAX31855_DATA_SIZE 4  // MAX31855 is usually 32-bit (4 bytes)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static Sensing_Data_t sensor_data = {0};

// DMA Buffers for MAX31865
static uint8_t rx_buf_31865[MAX31865_DATA_SIZE];
static uint8_t tx_buf_31865[MAX31865_DATA_SIZE] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t config_data_31865[8] = {0x80, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00};

// DMA Buffer for MAX31855
static uint8_t rx_buf_31855[MAX31855_DATA_SIZE + 1]; // Extra byte for safety

/* Private function prototypes -----------------------------------------------*/
static void config_max31865(void);

/* Public Functions ----------------------------------------------------------*/

/**
 * @brief Initializes the sensing module and configures the sensors.
 */
void sensing_init(void) {
  //initialize SPI  
  MX_SPI1_Init();
  MX_SPI2_Init();
  //configure max31865
  config_max31865();
}


/**
 * @brief Starts a non-blocking DMA read for the MAX31865 sensor.
 */
void max31865_read(void) {
    // Pull CS low to select the chip
    HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
    
    // Start DMA transfer
    HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buf_31865, rx_buf_31865, MAX31865_DATA_SIZE);
}

/**
 * @brief Starts a non-blocking DMA read for the MAX31855 sensor.
 */
void max31855_read(void) {
    // Pull CS low to select the chip
    HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_RESET);
    
    // Start DMA receive (MAX31855 is read-only)
    HAL_SPI_Receive_DMA(&hspi2, rx_buf_31855, 4);
}

/**
 * @brief Callback function triggered when an SPI DMA transfer is complete.
 * @param hspi SPI handle
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        // Deselect MAX31865
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
        process_max31865_data();
    }
}

/**
 * @brief Callback function triggered when an SPI DMA receive is complete.
 * @param hspi SPI handle
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        // Deselect MAX31855
        HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
        process_max31855_data();
    }
}

/**
 * @brief Processes the raw data received from the MAX31865 sensor.
 */
void process_max31865_data(void) {
    // Placeholder for temperature conversion logic
    // sensor_data.temp_max31865 = ...
}

/**
 * @brief Processes the raw data received from the MAX31855 sensor.
 */
void process_max31855_data(void) {
    // Placeholder for temperature conversion logic
    // sensor_data.temp_max31855 = ...
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
    // Handle MAX31855 error
}

/* Private Functions ---------------------------------------------------------*/

/**
 * @brief Configures the MAX31865 chip settings.
 */
static void config_max31865(void) {
    HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, config_data_31865, 8, 100);
    HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
}
