#include "MAGlib.h"

void dataCoversionMAG(void);

//Inicjalizacja I2C i wstêpene ustawienie parametrów IMU
void MAGinit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C3, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_I2C3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource9, GPIO_AF_I2C3);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStruct.I2C_ClockSpeed = 400000;
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStruct.I2C_OwnAddress1 = 0x60;
	I2C_Init(I2C3, &I2C_InitStruct);

	I2C_Cmd(I2C3, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = I2C3_EV_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//konfiguracja IMU
	MAGsetReg(0x26, 0);

	//Zezwolenie na przerwania (od teraz tylko pobiera dane)
	I2C_ITConfig(I2C3, I2C_IT_EVT, ENABLE);

}


//Funkcja do inicjalizacji z obs³ug¹ Timeout
volatile uint32_t Timeout3 = 0xFFFF;

void MAGsetReg(uint8_t regNum, uint8_t regValue) {
	Timeout3 = 0xFFFF;

	I2C_AcknowledgeConfig(I2C3, ENABLE);   //zazwolenie na potwierdzenie odebrania bitu ACK
	I2C_GenerateSTART(I2C3, ENABLE); 		//wygenerowanie START (potem sprawdzam czy start przesz³o)
	while (I2C_GetFlagStatus(I2C3, I2C_FLAG_SB) != SET) {
		Timeout3--;
		if (Timeout3 == 0) {
			return;
		}
	}
	I2C_Send7bitAddress(I2C3, (MAGaddress << 1), I2C_Direction_Transmitter); //nadanie adresu
	while (I2C_GetFlagStatus(I2C3, I2C_FLAG_ADDR) != SET) {
		Timeout3--;
		if (Timeout3 == 0) {
			return;
		}
	}
	I2C3->SR2;									//NIE WIEM CO TO ALE BEZ TEGO NIE DZIA£A!!!
	I2C_SendData(I2C3, regNum);			//Wys³anie numeru rejestru
	while (I2C_GetFlagStatus(I2C3, I2C_FLAG_TXE) != SET) {
		Timeout3--;
			if (Timeout3 == 0) {
				return;
			}
	}
	I2C_SendData(I2C3, regValue); 		//Wys³anie wartoœci do wpisania do rejestru
	while (I2C_GetFlagStatus(I2C3, I2C_FLAG_TXE) != SET) {
		Timeout3--;
			if (Timeout3 == 0) {
				return;
			}
	}
	I2C_GenerateSTOP(I2C3, ENABLE);			//Wygenerowanie STOP
}


//Rozpoczyna komunikacjê z IMU (potem wszystko idzie w przerwaniu
void I2C3startDataAcquisition(void) {
	I2C_AcknowledgeConfig(I2C3, ENABLE);
	I2C_GenerateSTART(I2C3, ENABLE);
}

//Funkcja testowa chujowa ale dzia³a wed³ug niej s¹ kolejne kroki w przerwaniu robione
void I2C3test(void) {
	I2C_AcknowledgeConfig(I2C3, ENABLE);
	I2C_GenerateSTART(I2C3, ENABLE);
	while (I2C_GetFlagStatus(I2C3, I2C_FLAG_SB) != SET) {
		;
	}
	I2C_Send7bitAddress(I2C3, (MAGaddress << 1), I2C_Direction_Transmitter);
	while (I2C_GetFlagStatus(I2C3, I2C_FLAG_ADDR) != SET) {
		;
	}
	I2C3->SR2;
	I2C_SendData(I2C3, (0x08 | 0x80));
	while (I2C_GetFlagStatus(I2C3, I2C_FLAG_BTF) != SET) {
		;
	}
	I2C_GenerateSTART(I2C3, ENABLE);
	while (I2C_GetFlagStatus(I2C3, I2C_FLAG_SB) != SET) {
		;
	}
	I2C_Send7bitAddress(I2C3, (MAGaddress << 1), I2C_Direction_Receiver);
	while (I2C_GetFlagStatus(I2C3, I2C_FLAG_ADDR) != SET) {
		;
	}
	I2C3->SR2;
	for (int i = 0; i < 6; i++) {
		while (I2C_GetFlagStatus(I2C3, I2C_FLAG_RXNE) != SET) {
			;
		}
		I2C_ClearFlag(I2C3, I2C_FLAG_RXNE);
		MAGrawData[i] = I2C_ReceiveData(I2C3);
	}
	I2C_AcknowledgeConfig(I2C3, DISABLE);
	I2C_GenerateSTOP(I2C3, ENABLE);
	dataCoversionMAG();
}


//konwersja danych z rejestrów IMU na faktyczne wartoœci
void dataCoversionMAG(void) {
	MagnetometrData[0] = MAGrawData[1] << 8 | MAGrawData[0];
	MagnetometrData[1] = MAGrawData[3] << 8 | MAGrawData[2];
	MagnetometrData[2] = MAGrawData[5] << 8 | MAGrawData[4];
}



//Przerwanie od zdarzen w I2C
//Kolejne kroki okreœla zmienna imInterruptSeqence
//Sprawdzam flagi i po kolei robiê tak jak w I2Ctest
volatile uint8_t magInterruptSequence;

void I2C3_EV_IRQHandler(void) {
	if (I2C_GetFlagStatus(I2C3, I2C_FLAG_SB) == SET) {
		I2C_ClearFlag(I2C3, I2C_FLAG_SB);
		if (magInterruptSequence == 0) {
			I2C_Send7bitAddress(I2C3, (MAGaddress << 1),
			I2C_Direction_Transmitter);
			magInterruptSequence++;
			return;
		} else if (magInterruptSequence == 3) {
			I2C_Send7bitAddress(I2C3, (MAGaddress << 1),
			I2C_Direction_Receiver);
			magInterruptSequence++;
			return;
		}
	}
	if (I2C_GetFlagStatus(I2C3, I2C_FLAG_ADDR) == SET) {
		I2C_ClearFlag(I2C3, I2C_FLAG_ADDR);
		if (magInterruptSequence == 1) {
			I2C3->SR2;
			I2C_SendData(I2C3, (0x08 | 0x80));
			magInterruptSequence++;
			return;
		} else if (magInterruptSequence == 4) {
			I2C3->SR2;
			magInterruptSequence++;
			return;
		}
	}
	if (I2C_GetFlagStatus(I2C3, I2C_FLAG_BTF) == SET) {
		I2C_ClearFlag(I2C3, I2C_FLAG_BTF);
		if (magInterruptSequence == 2) {
			I2C_GenerateSTART(I2C3, ENABLE);
			magInterruptSequence++;
		} else if (magInterruptSequence == 5) {
			static int i = 0;
			MAGrawData[i] = I2C_ReceiveData(I2C3);
			i++;
			if (i >= 5) { //po odebraniu przedostatniego bitu, w czasie transmisji ostatniego
				I2C_AcknowledgeConfig(I2C3, DISABLE);
				I2C_GenerateSTOP(I2C3, ENABLE);
				if (i >= 6) {  //po odebraniu ostatniego
					i = 0;
					magInterruptSequence = 0;
					dataCoversionMAG();
				}
			}
			return;
		}
	}
	if (I2C_GetFlagStatus(I2C3, I2C_FLAG_RXNE) == SET) {
		I2C_ClearFlag(I2C3, I2C_FLAG_RXNE);
	}
	if (I2C_GetFlagStatus(I2C3, I2C_FLAG_ADD10) == SET) {
		I2C_ClearFlag(I2C3, I2C_FLAG_ADD10);
	}
	if (I2C_GetFlagStatus(I2C3, I2C_FLAG_STOPF) == SET) {
		I2C_ClearFlag(I2C3, I2C_FLAG_STOPF);
	}
	if (I2C_GetFlagStatus(I2C3, I2C_FLAG_TXE) == SET) {
		I2C_ClearFlag(I2C3, I2C_FLAG_TXE);
	}
}

