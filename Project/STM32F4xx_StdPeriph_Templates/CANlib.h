//Nazwa: CANlib.h
//Autor: Konrad Aleksiejuk
//Projekt: �azik marsja�ski KNR
/*
 * INSTRUKCJA OBS�UGI BIBLIOTEKI
 * 1)na poczatku programu musi zanlezc si� #include "CANlib.h"
 * 2)w funkcji main (poza petla glowna) musi znalezc sie CAN_Config();
 * W razie potrzeby wykorzystanie przerwa� nalezy wprowadzic odpowiednie zmiany w pliku CANlib.c
 *
 * Adresowanie:
 *  Wiadomosc do wszystkich				0x00
 *  Wiadomosc do sterowanika prawego 	0x123
 *  Wiadomosc do sterowanika lewego 	0x124
 *  Adres sterownika glownego			0x125
 */

#ifndef CANlib_H
#define CANlib_H

#include "stm32f4xx.h"
#include "UARTlib.h"

//definiowanie zmiennych zawieraj�cych wiadmosci
CanTxMsg txMessage; //wiadomosc do wyslania
CanRxMsg rxMessage; //wiadomosc odebrana

//definiowanie typ�w wyliczeniowych dla funkcji
//kierunek obrot�w silnika
typedef enum {
	PRZOD = 0, TYL = 1
} Silnik_kierunk;

//strona po kt�rej zanjduj� si� silniki
typedef enum {
	PRAWA = -1, OBA = 0, LEWA = 1
} Silniki_strona;

//stan silnika w��czyc START, wy��czyc STOP
typedef enum {
	STOP = 0, START = 1
} Silnik_enable;

//definiowanie funkcji
void initCan(void);   //funkcja inicjalizuj�ca
void CAN1_RX0_IRQHandler(void);   //przerwanie po odebraniu wiadomo�ci
void sendSpeed(Silniki_strona strona, int predkosc1, int predkosc2,
		int predkosc3);
void sendStop(Silnik_enable zezwolenie);
void sendPid(uint8_t P, uint8_t Ilow, uint8_t Ihigh, uint8_t K);
void sendUartParam(void);
void sendUartStartStop(void);

#endif
