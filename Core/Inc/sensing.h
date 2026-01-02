#ifndef __SENSING_H
#define __SENSING_H

#include "main.h"
#include <math.h>

/* Global Temperature Variables */

extern volatile uint8_t ambient_raw_val[8];
extern volatile uint8_t furnace_raw_val[5];
extern volatile float current_ambient_temp;
extern volatile int32_t current_furnace_temp;
extern volatile uint8_t furnace_error;
extern volatile uint8_t ambient_error;
extern volatile uint8_t status;
extern volatile float thermo_calib;
extern volatile float pt100_calib;

/* Function Prototypes */
void Sensing_Init(void);
void ambient_read(void);
void furnace_read(void);



#endif /* __SENSING_H */
