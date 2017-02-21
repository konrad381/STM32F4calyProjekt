#include "ADClib.h"

//===============================================================================
/**
 * @brief  Inicjalizuje przetwornik ADC w trybie analogwatchdog
 * @note   aktywuje przerwanie ADC_IRQHandler w momencie przekroczenia
 * 		warto�ci progowych napi�c wejsciowych na pinie PA0
 * @retval None
 */
void initAdc() {
	ADC_InitTypeDef adc;
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef gpio;
	batteryError = 0;
	batteryAlert = 0;
	batteryAlertTime = 0;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_0;
	gpio.GPIO_Mode = GPIO_Mode_AN;
	gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &gpio);

	ADC_StructInit(&adc);
	adc.ADC_DataAlign = ADC_DataAlign_Right;
	adc.ADC_Resolution = ADC_Resolution_12b;
	adc.ADC_ContinuousConvMode = ENABLE;
	adc.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	adc.ADC_NbrOfConversion = 1;
	adc.ADC_ScanConvMode = DISABLE;
	ADC_Init(ADC1, &adc);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_480Cycles);

	ADC_ContinuousModeCmd(ADC1, ENABLE);
	ADC_ITConfig(ADC1, ADC_IT_AWD, ENABLE);

	ADC_AnalogWatchdogSingleChannelConfig(ADC1, ADC_Channel_0);
	//---------------------------------------------------------------
	//Ustawienie poziomu napi�cia powoduj�cego przerwanie.
	//1861 = 9V
	//2172 = 10.5V
	ADC_AnalogWatchdogThresholdsConfig(ADC1, 4000, wartoscProgowaADC);
	//--------------------------------------------------------------
	ADC_AnalogWatchdogCmd(ADC1, ADC_AnalogWatchdog_SingleRegEnable);

	ADC_Cmd(ADC1, ENABLE);
	ADC_SoftwareStartConv(ADC1);
}

//===============================================================================
/**
 * @brief  Obs�uga przewania od analogwatchdog
 * @note   W momencie uruchomienia przerwania po raz pierwszy za pomoc� systick'a
 * 		odliczany jest zadany czas przez kt�ry przerwanie jest niekatywne, po
 * 		up�ywie tego czasu przerwanie jest ponownie aktywowane je�li w okre�lonym
 * 		(przez systick) czasie od ponownej aktywacji przerwania wyst�pi przerwanie
 * 		ADC pojazd zostanie zatrzymany oraz pojawi si� informacja o zbyt niskim
 * 		napi�ciu na baterii (miganie czerwonej diody). Ca�a ta procedura zapobiega
 * 		blokowaniu pojazdu przy rozruchu gdy silniki pobieraj� duzy pr�d i napi�cie
 * 		mierzone jest nizesze od rzeczywistego napi�cia na baterii.
 * @retval None
 */
void ADC_IRQHandler(void) {
	if (ADC_GetITStatus(ADC1, ADC_IT_AWD)) {
		ADC_ClearITPendingBit(ADC1, ADC_IT_AWD);
		ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE);
		if (batteryAlert == 1) {
			batteryError = 1;
			sendStop(STOP);
		}
		batteryAlert = 1;
	}
}

//===============================================================================
/**
 * @brief 	Sprawdzanie stanu baterii wed�ug schamtu opisanego przy przerwaniu
 * 			funkcja wewnatrz systick'a
 * @note   opis dzia�ania przy ADC_IRQHandler
 * @retval None
 */
void AdcBatteryStatusCheck(void) {
	if (batteryAlert > 0) {
		batteryAlertTime++;
		if (batteryAlertTime == betteryAlertPeriod) {
			ADC_ClearITPendingBit(ADC1, ADC_IT_AWD);
			ADC_ITConfig(ADC1, ADC_IT_AWD, ENABLE);
		}
		if (batteryAlertTime > (betteryAlertPeriod+20)) {
			batteryAlert = 0;
			batteryAlertTime = 0;
		}
	}
}

//===============================================================================
/**
 * @brief  Wysy�a informacj� o stanie baterii za pomoc� UART2 do jednostki steruj�cej
 * 			funkcja wewnatrz systick'a
 * @note   Struktura ramki danych: #+B+low(wynik pomiaru ADC)+high(wartosc pomiaru ADC)
 * 			wynik pomiary to 12 bit od 0 do 3.3V. Funkcja jest umieszczona wewn�trz systick'a
 * 			i wysy�a warto�c napi�cia z okreslonym interwalem.
 * @retval None
 */
void AdcBatteryStatusSend(void) {
	static uint16_t licznik = 0;
	licznik++;
	if (licznik == batteryValuePeriod) {
		licznik = 0;
		sendBuffor[0] = '#';
		sendBuffor[1] = 'B';
		sendBuffor[2] = 0xFF & ADC_GetConversionValue(ADC1);
		sendBuffor[3] = (0xFF00 & ADC_GetConversionValue(ADC1)) >> 8;
		UART2wyslij(4);
	}
}
