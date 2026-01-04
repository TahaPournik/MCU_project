/**
 ******************************************************************************
 * @file           : sensing.h
 * @brief          : Header file for sensor data acquisition
 ******************************************************************************
 */

#ifndef __SENSING_H
#define __SENSING_H

#include <math.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Structure to hold sensor data
 */
typedef struct {
    float_t temp_max31865;    /**< Temperature from MAX31865 (RTD) */
    int32_t temp_max31855;      /**< Temperature from MAX31855 (Thermocouple) */
} Sensing_Data_t;

#define MAX31865_DATA_SIZE 8
#define MAX31855_DATA_SIZE 4

/* Exported constants --------------------------------------------------------*/
extern volatile Sensing_Data_t sensor_data;
extern volatile float max31865_callibration;
extern volatile float max31855_callibration;
extern  uint8_t config_data_31865[9];
extern  uint8_t rx_buf_31865[MAX31865_DATA_SIZE + 1]; // Extra byte for safety
extern  uint8_t tx_buf_31865[MAX31865_DATA_SIZE + 1]; 

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the sensing module.
 */
void sensing_init(void);

/**
 * @brief Starts reading process for MAX31865.
 */
void max31865_read(void);

/**
 * @brief Starts reading process for MAX31855.
 */
void max31855_read(void);

/**
 * @brief Processes data for MAX31865.
 */
void process_max31865_data(void);

/**
 * @brief Processes data for MAX31855.
 */
void process_max31855_data(void);

/**
 * @brief Error handler for MAX31865.
 */
void max31865_ErrorCallback(void);

/**
 * @brief Error handler for MAX31855.
 */
void max31855_ErrorCallback(void);



#ifdef __cplusplus
}
#endif

#endif /* __SENSING_H */