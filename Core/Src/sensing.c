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
#include <stdio.h>
#include <string.h>
#include "stm32f1xx_hal.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/** 
 * @brief Processed sensor values 
 * Initialized with -999.0f to indicate no valid reading yet.
 */
Sensing_Data_t sensor_data = {-999.0f, -999};

/** DMA Buffers for MAX31865 */
uint8_t rx_buf_31865[MAX31865_DATA_SIZE]; 
uint8_t tx_buf_31865[MAX31865_DATA_SIZE] = {0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
uint8_t config_data_31865[MAX31865_DATA_SIZE] = {0x80, 0xc2, 0xff, 0xff, 0x4c, 0x6a, 0x40, 0x00, 0x00};

/** Moving average buffers for sample accumulation */
Sensing_Data_Array_t max31865_data = {0};
Sensing_Data_Array_t max31855_data = {0};

/** DMA Buffer for MAX31855 (Binary read-only sensor) */
uint8_t rx_buf_31855[MAX31855_DATA_SIZE];

/** Sensor state flags */
uint8_t max31865_busy_flag = 0;
uint8_t max31865_fault_flag = 0;
uint8_t max31855_busy_flag = 0;
uint8_t max31855_fault_flag = 0;

/** Accumulated error registers */
uint32_t max31865_error = 0;
uint32_t max31855_error = 0;

/** LCD Display buffers */
char max31865_lcd_buffer[16];
char max31855_lcd_buffer[16];

/** Calibration offsets (applied after processing) */
float max31865_calibration = 0.0f;
float max31855_calibration = 0.75f;

/* Private function prototypes -----------------------------------------------*/
static void config_max31865(void);

/* Public Functions ----------------------------------------------------------*/

/**
 * @brief  Initializes the sensing module and configures the sensors.
 * @retval None
 */
void sensing_init(void) {
    /* Initialize CS pins (Active Low, set high to deselect) */
    HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);

    /* Auto-configure MAX31865 registers via DMA */
    config_max31865();
}

/**
 * @brief  Starts a non-blocking DMA read for the MAX31865 sensor.
 * @retval None
 */
void max31865_read(void) {
    if (max31865_busy_flag == 0) {
        max31865_busy_flag = 1;
        /* Pull CS low to select the chip */
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
        /* Start DMA transfer (bi-directional) */
        HAL_SPI_TransmitReceive_DMA(&hspi1, tx_buf_31865, rx_buf_31865, MAX31865_DATA_SIZE);
    }
}

/**
 * @brief  Starts a non-blocking DMA read for the MAX31855 sensor.
 * @retval None
 */
void max31855_read(void) {
    if (max31855_busy_flag == 0) {
        max31855_busy_flag = 1;
        /* Pull CS low to select the chip */
        HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_RESET);
        /* Start DMA receive (MAX31855 requires clock, so we transmit dummy bytes) */
        static uint8_t tx_dummy[4] = {0x00, 0x00, 0x00, 0x00};
        HAL_SPI_TransmitReceive_DMA(&hspi2, tx_dummy, rx_buf_31855, MAX31855_DATA_SIZE);
    }
}

/**
 * @brief  Callback function triggered when an SPI DMA transfer is complete.
 * @param  hspi SPI handle
 * @retval None
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI1) {
        /* Deselect MAX31865 */
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_SET);
        
        /* Update buffer index and store data for averaging */
        if (max31865_data.index < 4) {
            max31865_data.index++;
        } else {
            max31865_data.index = 0;
        }
        memcpy(max31865_data.data[max31865_data.index], rx_buf_31865, MAX31865_DATA_SIZE);
        max31865_busy_flag = 0;
        
    } else if (hspi->Instance == SPI2) {
        /* Deselect MAX31855 */
        HAL_GPIO_WritePin(MAX31855_CS_GPIO_Port, MAX31855_CS_Pin, GPIO_PIN_SET);
        
        /* Update buffer index and store data for averaging */
        if (max31855_data.index < 4) {
            max31855_data.index++;
        } else {
            max31855_data.index = 0;
        }
        memcpy(max31855_data.data[max31855_data.index], rx_buf_31855, MAX31855_DATA_SIZE);
        max31855_busy_flag = 0;
    }
}

/**
 * @brief  Processes the raw data received from the MAX31865 sensor (Averaging).
 * @retval None
 */
void process_max31865_data(void) {
    int32_t raw_accumulated = 0;
    for (int i = 0; i < 5; i++) {
        /* Check for fault bit or missing data (all zeros) */
        if ((max31865_data.data[i][3] & 0x01) || (max31865_data.data[i][3] == 0x00)) {
            max31865_ErrorCallback();
            return;
        }
        /* Assemble 15-bit raw ADC value */
        raw_accumulated += ((max31865_data.data[i][2] << 7) | (max31865_data.data[i][3] >> 1));
    }
    int32_t raw_average = raw_accumulated / 5;
    /* Convert ADC value to Temperature (Celsius) */
    sensor_data.temp_max31865 = ((float)raw_average / 32.0f) - 256.0f + max31865_calibration;
    sprintf(max31865_lcd_buffer, "AMB:%.2f", sensor_data.temp_max31865);
}

/**
 * @brief  Processes the raw data received from the MAX31855 sensor (Averaging).
 * @retval None
 */
void process_max31855_data(void) {
    int32_t temp_sum = 0;
    int16_t internal_temp = 0;
    int16_t thermocouple_temp = 0;

    for (int i = 0; i < 5; i++) {
        /* Check fault bit (Bit 16 in the 32-bit stream, but index 1 is D23-D16) */
        if (max31855_data.data[i][1] & 0x01) {
            max31855_ErrorCallback();
            return;
        }
        
        /* Process Internal Temperature (12-bit signed, steps of 0.0625 C) */
        if (max31855_data.data[i][2] & 0x80) { /* Negative internal temp */
            internal_temp = ((max31855_data.data[i][2] << 2) | (max31855_data.data[i][3] >> 6)) | 0xC000;
        } else {
            internal_temp = (max31855_data.data[i][2] << 2) | (max31855_data.data[i][3] >> 6);
        }
        
        /* Process Thermocouple Temperature (14-bit signed, steps of 0.25 C) */
        if (max31855_data.data[i][0] & 0x80) { /* Negative thermocouple temp */
            thermocouple_temp = ((max31855_data.data[i][0] << 6) | (max31855_data.data[i][1] >> 2)) | 0xC000;
        } else {
            thermocouple_temp = (max31855_data.data[i][0] << 6) | (max31855_data.data[i][1] >> 2);
        }
        
        temp_sum += (thermocouple_temp - internal_temp);
    }
    
    /* Calculate average and apply calibration */
    sensor_data.temp_max31855 = (int32_t)round(((float)temp_sum / 5.0f) * 0.25f + max31855_calibration);
    sprintf(max31855_lcd_buffer, "FURN:%d C", (int)sensor_data.temp_max31855);
}

/**
 * @brief  Callback for SPI errors.
 * @param  hspi SPI handle
 * @retval None
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
 * @brief  Error handler for MAX31865 communication issues and diagnostic faults.
 * @retval None
 */
void max31865_ErrorCallback(void) {
    sensor_data.temp_max31865 = -999.0f;
    max31865_fault_flag = 1;
    max31865_error |= HAL_SPI_GetError(&hspi1);
    
    uint8_t fault_bits = 0;
    for (int i = 0; i < 5; i++) {
        if (max31865_data.data[i][3] & 0x01) {
            /* Accumulate fault flags from Fault Status Register (typically index 8 in extended read) */
            fault_bits |= max31865_data.data[i][8];
        }
        if (max31865_data.data[i][3] == 0) {
            max31865_error |= MAX31865_SENSOR_OC;
        }
    }

    if (fault_bits & 0x04) max31865_error |= MAX31865_OU_VOLTAGE;
    if (fault_bits & 0x08) max31865_error |= MAX31865_SENSOR_OC;
    if (fault_bits & 0x10) max31865_error |= MAX31865_REF_OC;
    if (fault_bits & 0x20) max31865_error |= MAX31865_REF_SC;
    if (fault_bits & 0x40) max31865_error |= MAX31865_LOW_THD;
    if (fault_bits & 0x80) max31865_error |= MAX31865_HIGH_THD;
    
    sprintf(max31865_lcd_buffer, "ERR:0X%X", (int)max31865_error);
}

/**
 * @brief  Error handler for MAX31855 communication issues and diagnostic faults.
 * @retval None
 */
void max31855_ErrorCallback(void) {
    sensor_data.temp_max31855 = -999;
    max31855_fault_flag = 1;
    max31855_error |= HAL_SPI_GetError(&hspi2);
    
    uint8_t fault_bits = 0;
    for (int i = 0; i < 5; i++) {
        if (max31855_data.data[i][1] & 0x01) {
            /* Fault bits are in the last byte of the 32-bit stream */
            fault_bits |= max31855_data.data[i][3];
        }
    }

    if (fault_bits & 0x01) max31855_error |= MAX31855_SENSOR_OC;
    if (fault_bits & 0x02) max31855_error |= MAX31855_SENSOR_SC_GND;
    if (fault_bits & 0x04) max31855_error |= MAX31855_SENSOR_SC_VCC;
    
    sprintf(max31855_lcd_buffer, "ERR:0X%X", (int)max31855_error);
}

/**
 * @brief  Writes configuration registers to MAX31865.
 * @retval None
 */
static void config_max31865(void) {
    if (max31865_busy_flag == 0) {
        rx_buf_31865[3] = 0xfe;
        max31865_busy_flag = 1;
        HAL_GPIO_WritePin(MAX31865_CS_GPIO_Port, MAX31865_CS_Pin, GPIO_PIN_RESET);
        static uint8_t dummy_rx[MAX31865_DATA_SIZE];
        HAL_SPI_TransmitReceive_DMA(&hspi1, config_data_31865, dummy_rx, MAX31865_DATA_SIZE);   
    }
}

