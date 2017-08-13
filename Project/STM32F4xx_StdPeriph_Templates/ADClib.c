#include "ADClib.h"

//===============================================================================
/**
 * @brief  Inicjalizuje przetwornik ADC w trybie analogwatchdog
 * @note   aktywuje przerwanie ADC_IRQHandler w momencie przekroczenia
 * 		wartoœci progowych napiêc wejsciowych na pinie PA0
 * @retval None
 */
void initAdcWatchdog() {
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
	//Ustawienie poziomu napiêcia powoduj¹cego przerwanie.
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
 * @brief  Obs³uga przewania od analogwatchdog
 * @note   W momencie uruchomienia przerwania po raz pierwszy za pomoc¹ systick'a
 * 		odliczany jest zadany czas przez który przerwanie jest niekatywne, po
 * 		up³ywie tego czasu przerwanie jest ponownie aktywowane jeœli w okreœlonym
 * 		(przez systick) czasie od ponownej aktywacji przerwania wyst¹pi przerwanie
 * 		ADC pojazd zostanie zatrzymany oraz pojawi siê informacja o zbyt niskim
 * 		napiêciu na baterii (miganie czerwonej diody). Ca³a ta procedura zapobiega
 * 		blokowaniu pojazdu przy rozruchu gdy silniki pobieraj¹ duzy pr¹d i napiêcie
 * 		mierzone jest nizesze od rzeczywistego napiêcia na baterii.
 * @retval None
 */
void ADC_IRQHandler(void) {
	if (ADC_GetITStatus(ADC1, ADC_IT_AWD)) {
		ADC_ClearITPendingBit(ADC1, ADC_IT_AWD);
		ADC_ITConfig(ADC1, ADC_IT_AWD, DISABLE);
		if (batteryAlert == 1) {
			batteryError = 1;
			//sendStop(STOP);
		}
		batteryAlert = 1;
	}
}

//===============================================================================
/**
 * @brief 	Sprawdzanie stanu baterii wed³ug schamtu opisanego przy przerwaniu
 * 			funkcja wewnatrz systick'a
 * @note   opis dzia³ania przy ADC_IRQHandler
 * @retval None
 */
void AdcBatteryStatusCheck(void) {
	if (batteryAlert > 0) {
		batteryAlertTime++;
		if (batteryAlertTime == betteryAlertPeriod) {
			ADC_ClearITPendingBit(ADC1, ADC_IT_AWD);
			ADC_ITConfig(ADC1, ADC_IT_AWD, ENABLE);
		}
		if (batteryAlertTime > (betteryAlertPeriod + 20)) {
			batteryAlert = 0;
			batteryAlertTime = 0;
		}
	}
}

//===============================================================================
/**
 * @brief  Wysy³a informacjê o stanie baterii za pomoc¹ UART2 do jednostki steruj¹cej
 * 			funkcja wewnatrz systick'a
 * @note   Struktura ramki danych: #+B+low(wynik pomiaru ADC)+high(wartosc pomiaru ADC)
 * 			wynik pomiary to 12 bit od 0 do 3.3V. Funkcja jest umieszczona wewn¹trz systick'a
 * 			i wysy³a wartoœc napiêcia z okreslonym interwalem.
 * @retval None
 */
void AdcBatteryStatusSend(void) {
	static uint16_t licznik = 0;
	static uint8_t sendBuffor[5];
	licznik++;
	if (licznik == batteryValuePeriod) {
		licznik = 0;
		sendBuffor[0] = '#';
		sendBuffor[1] = 'B';
		sendBuffor[2] = 0xFF & adcValue[0];
		sendBuffor[3] = (0xFF00 & adcValue[0]) >> 8;
		UART2wyslij(&sendBuffor[0], 4);
	}
}


/*
 *  @brief  Pomiar napiêcia na 5 pinach (do obs³ugi potecjometrów -> pomiar wychyleñ osi) i bateria
 * @note   Zmierzone wartoœci wpisywane do tablicy adcValue[5] z wykorzystaniem DMA
 * @retval None
 */
void initAdc() {

	ADC_InitTypeDef ADC_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	DMA_InitTypeDef DMA_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(ADC1->DR);
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &adcValue[0];
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = 5;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);
	DMA_Cmd(DMA2_Stream0, ENABLE);


	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);


	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_1;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_18Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 5;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_28Cycles);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_28Cycles);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 3, ADC_SampleTime_28Cycles);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 4, ADC_SampleTime_28Cycles);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 5, ADC_SampleTime_28Cycles);


	ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);

	ADC_DMACmd(ADC1, ENABLE);

	ADC_Cmd(ADC1, ENABLE);

	ADC_SoftwareStartConv(ADC1);
}
