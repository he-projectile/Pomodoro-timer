/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MICROS_H
#define __MICROS_H

#include "main.h"

static struct systemTimerEntity{
	TIM_HandleTypeDef *htim;
	uint32_t secs;
} systemTimer1;

struct timerEntity{
	uint64_t microsToStop;
	uint64_t microsStart;
	TIM_HandleTypeDef *htim;
};

void systemTimerInit(TIM_HandleTypeDef *_htim, uint32_t _timerClockMHz){
	systemTimer1.htim = _htim;
	systemTimer1.htim->Init.Prescaler = (_timerClockMHz/1000000)-1;
	systemTimer1.htim->Init.Period = 1000000;
	HAL_TIM_Base_DeInit(_htim);
	HAL_TIM_Base_Init(_htim);
	HAL_TIM_Base_Start_IT(systemTimer1.htim);
}

void systemTimerInterrupt(){
	systemTimer1.secs++;	
}

void timerStart(struct timerEntity* timer, uint64_t secsToStop, uint64_t microsToStop){
	timer->microsStart = (uint64_t) systemTimer1.secs  * 1000000 +  systemTimer1.htim->Instance->CNT;
	timer->microsToStop = secsToStop * 1000000 + microsToStop + timer->microsStart;	
}

uint8_t isTimerFinished(struct timerEntity *timer){  // checks if time stored in timer passed	
	if ( (uint64_t) systemTimer1.secs * 1000000 + systemTimer1.htim->Instance->CNT  >= timer->microsToStop)
		return 1;
	return 0;
}

uint64_t whatTimeIsIt( struct timerEntity *timer ){
	return (uint64_t) systemTimer1.secs * 1000000 + systemTimer1.htim->Instance->CNT - timer->microsStart;	
}

int64_t whatTimeIsToFinish( struct timerEntity *timer ){
	return (int64_t) timer->microsToStop - (uint64_t) systemTimer1.secs * 1000000 - systemTimer1.htim->Instance->CNT;
}

#endif