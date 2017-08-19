//Nazwa: I2Clib.h
//Autor: Konrad Aleksiejuk
//Projekt: £azik marsjañski KNR
/*
 *
 */

#ifndef MAGlib_H
#define MAGlib_H

#include "stm32f4xx.h"

volatile uint8_t MAGrawData[6];
volatile int16_t MagnetometrData[3];

void MAGinit(void);
void I2C3test(void);
void MAGsetReg(uint8_t regNum, uint8_t regValue);
void I2C3startDataAcquisition(void);
void I2C3_EV_IRQHandler(void);


#define MAGaddress 0b0011101 // the slave address (example)

#endif
