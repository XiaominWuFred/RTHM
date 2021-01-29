#include <stdlib.h>
#include "hw_types.h"
#include "timer_control.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "systick.h"
#include "gpio_if.h"
#include "gpio_control.h"
#include "interrupt.h"

TIMER_HANDLE TIMER_Handle;

static BaseType_t xHigherPriorityTaskWoken;
unsigned long re[2];

void timer_stop(unsigned long Timer_base, unsigned long Timer_peripheral);
void timer_close(unsigned long Timer_base, unsigned long Timer_peripheral);


void blocking_timer_ISR(){
	TimerIntClear(TIMER0BASE, TIMER_TIMA_TIMEOUT);

	if(TIMER_Handle->TimerOn == 1){

//	if(TIMER_Handle->count %2 == 0){
//		GPIO_IF_LedOn(9);
//	}else{
//		GPIO_IF_LedOff(9);
//	}

//    TIMER_Handle->count = TIMER_Handle->count + 1;

	xSemaphoreGiveFromISR( TIMER_Handle->timer_Semaphore, &xHigherPriorityTaskWoken );// release semaphore
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );//forcing context exchange
	}

}

void counting_timer_ISR(){
	TimerIntClear(TIMER1BASE, TIMER_TIMA_TIMEOUT);

	if(TIMER_Handle->TimerOn == 1){
	TIMER_Handle->T2C = TIMER_Handle->T2C + 1;

	if(TIMER_Handle->T2C == TIMER_Handle->T2CMAX){
		TIMER_Handle->TimerOn = 0;

		timer_stop(TIMER0BASE, TIMER0PRCM);
		timer_stop(TIMER1BASE, TIMER1PRCM);
		counter_stop(TIMER2BASE, TIMER2PRCM);
	}
	}
}

unsigned long* period_convert(unsigned long realtime){


	re[0] = 1;

	realtime = ((realtime * 1000000) * 2 )/ 25;

	while(realtime > 65535){
		realtime = realtime/2;
		re[0] = re[0]*2;
	}

	re[1] = realtime-1; // load
	re[0] = re[0] - 1; // prescaler

	return re;
}

TIMER_HANDLE timer_configure(unsigned long Timer_base, unsigned long Timer_peripheral, unsigned long realtime, void (*pfnHandler)(void)){
	//TIMER0 AND TIMER1 SHARE ONE HANDLE
	if(Timer_base == TIMER0BASE){
		TIMER_Handle = (TIMER_HANDLE)malloc(sizeof(TIMER_STATE));
	}

	if(TIMER_Handle == NULL){
		return NULL;
	}



	TIMER_Handle->count = 0;

	unsigned long *timevalue;

	timevalue = period_convert(realtime);

	//only timer 0 use semaphore
	if(Timer_base == TIMER0BASE){
	vSemaphoreCreateBinary( TIMER_Handle->timer_Semaphore ); //semaphore create

    	if(TIMER_Handle->timer_Semaphore == NULL){
    		timer_close(TIMER0BASE, TIMER0PRCM);
    		//timer_close(TIMER1BASE, TIMER1PRCM);
    		return NULL;
    	}

	}
	PRCMPeripheralClkEnable(Timer_peripheral, PRCM_RUN_MODE_CLK);
	PRCMPeripheralReset(Timer_peripheral);


	TimerConfigure(Timer_base,  TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);// counting down mode
	TimerIntRegister(Timer_base, TIMER_A, pfnHandler);
	TimerIntEnable(Timer_base, TIMER_TIMA_TIMEOUT);
	TimerLoadSet(Timer_base, TIMER_A, *(timevalue+1));

	TimerPrescaleSet(Timer_base, TIMER_A, *timevalue);

	TimerControlStall(Timer_base, TIMER_A, true); //timer stop wihle debugger halt

	return TIMER_Handle;
}

COUNTER_HANDLE counter_configure(unsigned long Timer_base, unsigned long Timer_peripheral){
	COUNTER_HANDLE COUNTER_Handle;
	COUNTER_Handle = (COUNTER_HANDLE)malloc(sizeof(COUNTER_STATE));

	COUNTER_Handle->timerBase_address = Timer_base;
	COUNTER_Handle->counterOn = 0x00;

	PRCMPeripheralClkEnable(Timer_peripheral, PRCM_RUN_MODE_CLK);
	PRCMPeripheralReset(Timer_peripheral);

	TimerConfigure(Timer_base, TIMER_CFG_ONE_SHOT_UP);


	return COUNTER_Handle;
}

void counter_start(COUNTER_HANDLE COUNTER_Handle){
	TimerValueSet(COUNTER_Handle->timerBase_address, TIMER_A, 0x0); //reset counter
	TimerEnable(COUNTER_Handle->timerBase_address, TIMER_A);
	COUNTER_Handle->counterOn = 0x01;
}

void counter_reset(COUNTER_HANDLE COUNTER_Handle){
	TimerValueSet(COUNTER_Handle->timerBase_address, TIMER_A, 0x0); //reset counter
}

uint32_t counter_getValue(COUNTER_HANDLE COUNTER_Handle){

	return (uint32_t)TimerValueGet(COUNTER_Handle->timerBase_address, TIMER_A);
}

uint32_t counter_periodMeasured(COUNTER_HANDLE COUNTER_Handle){
	COUNTER_Handle->periodMeasured = counter_getValue(COUNTER_Handle);

	return COUNTER_Handle->periodMeasured;
}

void setT2CMAX(uint16_t max){
	if(TIMER_Handle != NULL){
		TIMER_Handle->T2CMAX = max;
	}
}

void timerS_start(){
	if(xSemaphoreTake(TIMER_Handle->timer_Semaphore, portMAX_DELAY) == pdTRUE){

		TIMER_Handle->T2C = 0;
		TIMER_Handle->TimerOn = 1;

		TimerEnable(TIMERA1_BASE, TIMER_A);
		TimerEnable(TIMERA0_BASE, TIMER_A);
	}
}

int32_t timer_hold(uint32_t time_out){
	if(TIMER_Handle->timer_Semaphore != NULL){
	if(xSemaphoreTake(TIMER_Handle->timer_Semaphore, time_out/(portTICK_PERIOD_MS)) == pdTRUE){
		return 0;
	}else{
		return -1;
	}
	}else{
		return -2;
	}
}


void timer_stop(unsigned long Timer_base, unsigned long Timer_peripheral){
	TimerDisable(Timer_base, TIMER_A);
}

void counter_stop(unsigned long Timer_base, unsigned long Timer_peripheral){
	TimerDisable(Timer_base, TIMER_A);
}

void timer_close(unsigned long Timer_base, unsigned long Timer_peripheral){

	TimerIntClear(Timer_base, TIMER_TIMA_TIMEOUT);
	PRCMPeripheralClkDisable(Timer_peripheral,PRCM_RUN_MODE_CLK);

	if(TIMER_Handle->timer_Semaphore != NULL)
	    vSemaphoreDelete( TIMER_Handle->timer_Semaphore );

	if(TIMER_Handle != NULL)
		free(TIMER_Handle);
}

void counter_close(unsigned long Timer_base, unsigned long Timer_peripheral, COUNTER_HANDLE COUNTER_Handle){
	PRCMPeripheralClkDisable(Timer_peripheral,PRCM_RUN_MODE_CLK);
	if(COUNTER_Handle != NULL)
			free(COUNTER_Handle);
}
//*********for testing*************
unsigned long getTimerCount(){
	return TIMER_Handle->count;
}



