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
#define MAX31865_DATA_SIZE 9
#define MAX31855_DATA_SIZE 4

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

/*private define*/
//max31865_configs
#define FILTER_50HZ 0X00
#define FILTER_60HZ 0X01
#define FAULT_CLEAR 0X02
#define PT100_TWO_WIRE 0X10
#define PT100_FOUR_WIRE 0X10
#define PT100_THREE_WIRE 0X00
//MAX31865_FAULTS
#define MAX31865_OU_VOLTAGE 0X04 << 8
#define MAX31865_SENOR_OC 0X08 << 8
#define MAX31865_REF_OC 0X10 << 8
#define MAX31865_REF_SC 0X20 << 8
#define MAX31865_LOW_THD 0X40 << 8
#define MAX31865_HIGH_THD 0X80 << 8
//MAX31855_FAULTS
#define MAX31855_SENSOR_OC 0X01 << 8
#define MAX31855_SENSOR_SC_GND 0X02 << 8
#define MAX31855_SENSOR_SC_VCC 0X04 << 8

#ifdef __cplusplus
}
#endif

#endif /* __SENSING_H */
