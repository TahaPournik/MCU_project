/**
 ******************************************************************************
 * @file           : sensing.h
 * @brief          : Header file for sensor data acquisition
 ******************************************************************************
 */

#ifndef __SENSING_H
#define __SENSING_H

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
    float temp_max31865;    /**< Temperature from MAX31865 (RTD) */
    int temp_max31855;      /**< Temperature from MAX31855 (Thermocouple) */
} Sensing_Data_t;

/* Exported constants --------------------------------------------------------*/

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

/**
 * @brief Getter for sensor data.
 * @return Sensing_Data_t pointer
 */
Sensing_Data_t* Sensing_Get_Data(void);

#ifdef __cplusplus
}
#endif

#endif /* __SENSING_H */