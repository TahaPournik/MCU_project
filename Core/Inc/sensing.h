/**
 ******************************************************************************
 * @file           : sensing.h
 * @brief          : Header file for sensor data acquisition (MAX31865 & MAX31855)
 ******************************************************************************
 */

#ifndef __SENSING_H
#define __SENSING_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <math.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Exported defines ----------------------------------------------------------*/

/* Data Buffer Sizes */
#define MAX31865_DATA_SIZE 9
#define MAX31855_DATA_SIZE 4

/* MAX31865 Configuration Options */
#define FILTER_50HZ      0x00
#define FILTER_60HZ      0x01
#define FAULT_CLEAR      0x02
#define PT100_TWO_WIRE   0x10
#define PT100_FOUR_WIRE  0x10
#define PT100_THREE_WIRE 0x00

/* MAX31865 Fault Error Bits */
#define MAX31865_OU_VOLTAGE 0x04 << 8   /**< Over/Under voltage fault */
#define MAX31865_SENSOR_OC  0x08 << 8   /**< Sensor Open Circuit */
#define MAX31865_REF_OC     0x10 << 8   /**< Reference Open Circuit */
#define MAX31865_REF_SC     0x20 << 8   /**< Reference Short Circuit */
#define MAX31865_LOW_THD    0x40 << 8   /**< Low Threshold fault */
#define MAX31865_HIGH_THD   0x80 << 8   /**< High Threshold fault */

/* MAX31855 Fault Error Bits */
#define MAX31855_SENSOR_OC      0x01 << 8  /**< Sensor Open Circuit */
#define MAX31855_SENSOR_SC_GND  0x02 << 8  /**< Sensor Short Circuit to GND */
#define MAX31855_SENSOR_SC_VCC  0x04 << 8  /**< Sensor Short Circuit to VCC */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Structure to hold processed sensor data
 */
typedef struct {
    float temp_max31865;    /**< Temperature from MAX31865 (RTD) in Celsius */
    int32_t temp_max31855;  /**< Temperature from MAX31855 (Thermocouple) in Celsius */
} Sensing_Data_t;

/**
 * @brief Structure for raw sensor data buffering (averaging)
 */
typedef struct {
    uint8_t index;              /**< Current buffer index */
    uint8_t data[5][9];         /**< Data buffer for 5 samples */
} Sensing_Data_Array_t;

/* Exported constants/variables ----------------------------------------------*/
extern Sensing_Data_t sensor_data;
extern float max31865_calibration;
extern float max31855_calibration;

extern uint8_t config_data_31865[MAX31865_DATA_SIZE];
extern uint8_t rx_buf_31865[MAX31865_DATA_SIZE];
extern uint8_t tx_buf_31865[MAX31865_DATA_SIZE]; 
extern uint8_t rx_buf_31855[MAX31855_DATA_SIZE];

extern uint8_t max31865_fault_flag;
extern uint8_t max31855_fault_flag;
extern uint8_t max31865_busy_flag;
extern uint8_t max31855_busy_flag;

extern char max31865_lcd_buffer[16];
extern char max31855_lcd_buffer[16];

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief  Initializes the sensing module.
 * @retval None
 */
void sensing_init(void);

/**
 * @brief  Starts non-blocking reading for MAX31865.
 * @retval None
 */
void max31865_read(void);

/**
 * @brief  Starts non-blocking reading for MAX31855.
 * @retval None
 */
void max31855_read(void);

/**
 * @brief  Processes accumulated data for MAX31865.
 * @retval None
 */
void process_max31865_data(void);

/**
 * @brief  Processes accumulated data for MAX31855.
 * @retval None
 */
void process_max31855_data(void);

/**
 * @brief  Error callback for MAX31865.
 * @retval None
 */
void max31865_ErrorCallback(void);

/**
 * @brief  Error callback for MAX31855.
 * @retval None
 */
void max31855_ErrorCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* __SENSING_H */

