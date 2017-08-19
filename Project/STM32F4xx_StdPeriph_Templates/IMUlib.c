#include "IMUlib.h"

void dataCoversionIMU(void);

//Inicjalizacja I2C i wstêpene ustawienie parametrów IMU
void IMUinit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStruct.I2C_ClockSpeed = 400000;
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStruct.I2C_OwnAddress1 = 0x60;
	I2C_Init(I2C1, &I2C_InitStruct);

	I2C_Cmd(I2C1, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = I2C1_EV_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	//konfiguracja IMU
	IMUsetReg(107, 0); //wybudzenie IMU z uspienia bez tego nic nie mierzy (ale odpowiada)

	//Zezwolenie na przerwania (od teraz tylko pobiera dane)
	I2C_ITConfig(I2C1, I2C_IT_EVT, ENABLE);

}


//Funkcja do inicjalizacji z obs³ug¹ Timeout
volatile uint32_t Timeout = 0xFFFF;
volatile uint32_t licznik = 0;

void IMUsetReg(uint8_t regNum, uint8_t regValue) {
	Timeout = 0xFFFF;
	licznik = 0;
	I2C_AcknowledgeConfig(I2C1, ENABLE);   //zazwolenie na potwierdzenie odebrania bitu ACK
	I2C_GenerateSTART(I2C1, ENABLE); 		//wygenerowanie START (potem sprawdzam czy start przesz³o)
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) != SET) {
		Timeout--;
		if (Timeout == 0) {
			licznik++;
			Timeout = 0xFFFF;
		}
		if (licznik == 3) // je¿eli wykona pêtle 3x i nic to chuj
			return;
	}
	I2C_Send7bitAddress(I2C1, (IMUaddress << 1), I2C_Direction_Transmitter); //nadanie adresu
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) != SET) {
		Timeout--;
		if (Timeout == 0) {
			licznik++;
			Timeout = 0xFFFF;
		}
		if (licznik == 3) // je¿eli wykona pêtle 3x i nic to chuj
			return;
	}
	I2C1->SR2;									//NIE WIEM CO TO ALE BEZ TEGO NIE DZIA£A!!!
	I2C_SendData(I2C1, regNum);			//Wys³anie numeru rejestru
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) != SET) {
		Timeout--;
		if (Timeout == 0) {
			licznik++;
			Timeout = 0xFFFF;
		}
		if (licznik == 3) // je¿eli wykona pêtle 3x i nic to chuj
			return;
	}
	I2C_SendData(I2C1, regValue); 		//Wys³anie wartoœci do wpisania do rejestru
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) != SET) {
		Timeout--;
		if (Timeout == 0) {
			licznik++;
			Timeout = 0xFFFF;
		}
		if (licznik == 3) // je¿eli wykona pêtle 3x i nic to chuj
			return;
	}
	I2C_GenerateSTOP(I2C1, ENABLE);			//Wygenerowanie STOP
}


//Rozpoczyna komunikacjê z IMU (potem wszystko idzie w przerwaniu
void I2CstartDataAcquisition(void) {
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	I2C_GenerateSTART(I2C1, ENABLE);
}

//Funkcja testowa chujowa ale dzia³a wed³ug niej s¹ kolejne kroki w przerwaniu robione
void I2Ctest(void) {
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	I2C_GenerateSTART(I2C1, ENABLE);
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) != SET) {
		;
	}
	I2C_Send7bitAddress(I2C1, (IMUaddress << 1), I2C_Direction_Transmitter);
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) != SET) {
		;
	}
	I2C1->SR2;
	I2C_SendData(I2C1, 59);
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) != SET) {
		;
	}
	I2C_GenerateSTART(I2C1, ENABLE);
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) != SET) {
		;
	}
	I2C_Send7bitAddress(I2C1, (IMUaddress << 1), I2C_Direction_Receiver);
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) != SET) {
		;
	}
	I2C1->SR2;
	for (int i = 0; i < 14; i++) {
		while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) != SET) {
			;
		}
		I2C_ClearFlag(I2C1, I2C_FLAG_RXNE);
		IMUrawData[i] = I2C_ReceiveData(I2C1);
	}
	I2C_AcknowledgeConfig(I2C1, DISABLE);
	I2C_GenerateSTOP(I2C1, ENABLE);
	dataCoversionIMU();
}


//konwersja danych z rejestrów IMU na faktyczne wartoœci
void dataCoversionIMU(void) {
	IMUdata.Accel[0] = IMUrawData[0] << 8 | IMUrawData[1];
	IMUdata.Accel[1] = IMUrawData[2] << 8 | IMUrawData[3];
	IMUdata.Accel[2] = IMUrawData[4] << 8 | IMUrawData[5];
	IMUdata.Temp = IMUrawData[6] << 8 | IMUrawData[7];
	IMUdata.Gyro[0] = IMUrawData[8] << 8 | IMUrawData[9];
	IMUdata.Gyro[1] = IMUrawData[10] << 8 | IMUrawData[11];
	IMUdata.Gyro[2] = IMUrawData[12] << 8 | IMUrawData[13];
}



//Przerwanie od zdarzen w I2C
//Kolejne kroki okreœla zmienna imInterruptSeqence
//Sprawdzam flagi i po kolei robiê tak jak w I2Ctest
volatile uint8_t imuInterruptSequence;

void I2C1_EV_IRQHandler(void) {
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_SB) == SET) {
		I2C_ClearFlag(I2C1, I2C_FLAG_SB);
		if (imuInterruptSequence == 0) {
			I2C_Send7bitAddress(I2C1, (IMUaddress << 1),
			I2C_Direction_Transmitter);
			imuInterruptSequence++;
			return;
		} else if (imuInterruptSequence == 3) {
			I2C_Send7bitAddress(I2C1, (IMUaddress << 1),
			I2C_Direction_Receiver);
			imuInterruptSequence++;
			return;
		}
	}
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == SET) {
		I2C_ClearFlag(I2C1, I2C_FLAG_ADDR);
		if (imuInterruptSequence == 1) {
			I2C1->SR2;
			I2C_SendData(I2C1, 59);
			imuInterruptSequence++;
			return;
		} else if (imuInterruptSequence == 4) {
			I2C1->SR2;
			imuInterruptSequence++;
			return;
		}
	}
	if ((I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == SET)
			|| (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == SET)) {
		I2C_ClearFlag(I2C1, I2C_FLAG_BTF);

		if (imuInterruptSequence == 2) {
			I2C_GenerateSTART(I2C1, ENABLE);
			imuInterruptSequence++;
		} else if (imuInterruptSequence == 5) {
			static int i = 0;
			IMUrawData[i] = I2C_ReceiveData(I2C1);
			i++;
			if (i >= 13) { //po odebraniu przedostatniego bitu, w czasie transmisji ostatniego
				I2C_AcknowledgeConfig(I2C1, DISABLE);
				I2C_GenerateSTOP(I2C1, ENABLE);
				if (i >= 14) {  //po odebraniu ostatniego
					i = 0;
					imuInterruptSequence = 0;
					dataCoversionIMU();
				}
			}
			return;
		}
	}
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == SET) {
		I2C_ClearFlag(I2C1, I2C_FLAG_RXNE);
	}
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_ADD10) == SET) {
		I2C_ClearFlag(I2C1, I2C_FLAG_ADD10);
	}
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_STOPF) == SET) {
		I2C_ClearFlag(I2C1, I2C_FLAG_STOPF);
	}
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE) == SET) {
		I2C_ClearFlag(I2C1, I2C_FLAG_TXE);
	}
}

