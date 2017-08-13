//Nazwa: I2Clib.h
//Autor: Konrad Aleksiejuk
//Projekt: £azik marsjañski KNR
/*
 *
 */

#ifndef I2Clib_H
#define I2Clib_H

#include "stm32f4xx.h"

s16 test[6];

void initI2C(void);
void I2Ctest(void);
#define myAddress 0b1101000 // the slave address (example)
//void init_I2C1(void);
//void I2C_start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction);

#endif
