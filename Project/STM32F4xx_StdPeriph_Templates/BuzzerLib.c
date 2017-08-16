#include "BuzzerLib.h"

TIM_OCInitTypeDef TIM_OCInitStructure;

void buzzerInit(void) {
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_TIM2);

	TIM_TimeBaseStructure.TIM_Period = 1000;
	TIM_TimeBaseStructure.TIM_Prescaler = 45;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM2, ENABLE);
	TIM_Cmd(TIM2, DISABLE);
}

volatile uint16_t timeOn;
volatile uint16_t timeOff;
volatile uint16_t repetition;

void buzzerOn(uint16_t FtimeOn,uint16_t FtimeOff, uint16_t Frepetition){
timeOn=FtimeOn;
timeOff=FtimeOff;
repetition=Frepetition;
TIM_OCInitStructure.TIM_Pulse = 500;
TIM_OC1Init(TIM2, &TIM_OCInitStructure);
TIM_Cmd(TIM2,ENABLE);
}

volatile uint16_t state = 0;
volatile uint16_t time=0;

void TIM2_IRQHandler(void){
	if(TIM_GetFlagStatus(TIM2,TIM_FLAG_CC1)==SET){
		TIM_ClearFlag(TIM2,TIM_FLAG_CC1);
		time++;
		if(state==0 && time==timeOn){
			repetition--;
			if(repetition==0){
				time = 0;
				TIM_Cmd(TIM2, DISABLE);
				return;
			}
			time = 0;
			state = 1;
			TIM_OCInitStructure.TIM_Pulse = 0;
			TIM_OC1Init(TIM2, &TIM_OCInitStructure);
		}
		if(state==1 && time==timeOff){
			time=0;
			state = 0;
			TIM_OCInitStructure.TIM_Pulse = 500;
			TIM_OC1Init(TIM2, &TIM_OCInitStructure);
		}
	}
}
