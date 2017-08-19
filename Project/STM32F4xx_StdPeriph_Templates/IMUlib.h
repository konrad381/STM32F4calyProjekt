//Nazwa: I2Clib.h
//Autor: Konrad Aleksiejuk
//Projekt: £azik marsjañski KNR
/*
 *
 */

#ifndef IMUlib_H
#define IMUlib_H

#include "stm32f4xx.h"

volatile uint8_t IMUrawData[14];

void IMUinit(void);
void I2Ctest(void);
void IMUsetReg(uint8_t regNum, uint8_t regValue);
void I2CstartDataAcquisition(void);
void I2C1_EV_IRQHandler(void);

typedef struct {
	volatile int16_t Accel[3];
	volatile int16_t Gyro[3];
	volatile int16_t Temp;
	volatile int16_t AccelScale;
	volatile int16_t GyroScale;
} ImuData_TypeDef;


ImuData_TypeDef IMUdata;

#define IMUaddress 0b1101000 // the slave address (example)

#endif
