/*
 * timer_control.h
 *
 *  Created on: May 26, 2017
 *      Author: xwu
 */

#ifndef SOURCE_TIMER_CONTROL_H_
#define SOURCE_TIMER_CONTROL_H_

#include <FreeRTOS.h>
#include <semphr.h>
#include "timer.h"
#include "hw_memmap.h"
#include "i2cnew.h"


#define TIMER0BASE				TIMERA0_BASE
#define TIMER1BASE				TIMERA1_BASE
#define TIMER2BASE				TIMERA2_BASE
#define TIMER0PRCM				PRCM_TIMERA0
#define TIMER1PRCM				PRCM_TIMERA1
#define TIMER2PRCM				PRCM_TIMERA2
#define TIMERFAIL				-1
#define TIMERSUCCESS			1
#define MINUTE					0X11E1A3000   					//1 min in COUNT unit


typedef struct {
	unsigned long count;
	uint16_t T2C;
	uint16_t T2CMAX;
	uint8_t TimerOn;
	SemaphoreHandle_t timer_Semaphore;


} TIMER_STATE;

typedef TIMER_STATE* TIMER_HANDLE;

typedef struct{
	uint32_t periodMeasured;
	uint32_t timerBase_address;
	uint8_t counterOn;
} COUNTER_STATE;

typedef COUNTER_STATE* COUNTER_HANDLE;

void blocking_timer_ISR();

void counting_timer_ISR();

TIMER_HANDLE timer_configure(unsigned long Timer_base, unsigned long Timer_peripheral, unsigned long realtime, void (*pfnHandler)(void));

COUNTER_HANDLE counter_configure(unsigned long Timer_base, unsigned long Timer_peripheral);

void setT2CMAX(uint16_t max);

void timerS_start();

void counter_start(COUNTER_HANDLE COUNTER_Handle);

void counter_reset(COUNTER_HANDLE COUNTER_Handle);

uint32_t counter_getValue(COUNTER_HANDLE COUNTER_Handle);

uint32_t counter_periodMeasured(COUNTER_HANDLE COUNTER_Handle);

void counter_stop(unsigned long Timer_base, unsigned long Timer_peripheral);

void timer_close(unsigned long Timer_base, unsigned long Timer_peripheral);

void counter_close(unsigned long Timer_base, unsigned long Timer_peripheral, COUNTER_HANDLE COUNTER_Handle);

int32_t timer_hold();


void timer_stop(unsigned long Timer_base, unsigned long Timer_peripheral);

void timer_close(unsigned long Timer_base, unsigned long Timer_peripheral);

unsigned long getTimerCount();

#endif /* SOURCE_TIMER_CONTROL_H_ */
