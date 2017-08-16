//Nazwa: BuzzerLib.h
//Autor: Konrad Aleksiejuk
//Projekt: £azik marsjañski KNR
/*
 *
 */

#ifndef BuzzerLib_H
#define BuzzerLib_H

#include "stm32f4xx.h"


void buzzerInit(void);
void buzzerOn(uint16_t FtimeOn,uint16_t FtimeOff, uint16_t Frepetition);




#endif
