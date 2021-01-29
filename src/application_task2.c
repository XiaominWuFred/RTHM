/*
 * application_task2.c
 *
 *  Created on: Jun 9, 2017
 *      Author: xwu
 */

#include "control_task.h"
#include "string.h"
// Standard includes.
#include <stdio.h>
#include <stdlib.h>


#include "osi.h"

// Driverlib includes
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "hw_types.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"
#include "prcm.h"
#include "utils.h"
#include "pin.h"

// Common interface includes
#include "uart_if.h"
#include "UARTnew.h"
#include "i2cnew.h"
#include "i2c_if.h"
#include "timer_control.h"
#include "gpio.h"
#include "gpio_if.h"
#include "command.h"
#include "gpio_control.h"
// signal processing

#include "RRSignalProc.h"

#define SENDMAX				45


/********************************************
 * Task's data field handle
 * each application task should have different data field handle
 * customer's responsibility to rename the handle
 * and part in its setter after create new app task
 */
TASK_HANDLE_t TASK2HANDLE;

void SETTASK2HANDLE(TASK_HANDLE_t taskHandle){
	TASK2HANDLE = taskHandle;
}



/*********************************************
 * main task function
 * the label of this function is used to created a new task in the main function
 * of customer's program
 * it should be the same as in this file and in main.c file
 * customer has to declare the function in main.c and create new task using this label
 *
 * example in main.c:
 *
 * declaration:
 * void application_task2( void *pvParameters );
 *
 * task creation:
 * osi_TaskCreate( application_task2, "TASK3", OSI_STACK_SIZE,NULL, 1, NULL );
 */
void TASK2(){
	//handle variable for data communication
	DATA_HANDLE_t apptaskhandle;


	I2C_HANDLE I2C_Handler;
	rr_handle_t handle;

	//I2C_IF_Open(I2C_MASTER_MODE_FST);
	//initialize
	//****************
	uint8_t LEDcontrol = 0x00;
	uint8_t I2Cwritebuff[1];//for write function test
	uint8_t* bufferpointer = NULL;
	uint8_t* bufferpointercpy;
	uint8_t buffercount = 0;
	uint8_t breathRate = 0;
	uint16_t zSignal;
	int16_t output;
	uint32_t periodMesured;
	int8_t waveByte = 7;

	unsigned long period;
	uint16_t max;

	I2C_Handler = getI2CHandle();//I2C_MASTER_MODE_FST

	handle = Initialize_RR_Detector();

	RedLED_configure();
	//process input data and configure timer

	TIMER_HANDLE timer_handle;

	COUNTER_HANDLE counter_handle;

	//task's infinite loop
	while(1){

		//if this task chose by the control task else wait
	 
		xQueueReceive( TASK2HANDLE->TASKQ, &apptaskhandle, portMAX_DELAY ); // deQueue
		//do the task's mission using the data in the stucture(put by control task)

		//malloc for buffer
		bufferpointer = (uint8_t*)malloc(SENDMAX*sizeof(uint8_t));

		if(bufferpointer == NULL){
			while(1); // data allocation bug
		}

		bufferpointercpy = bufferpointer;
		buffercount = 0; //reset buffer count



		period= *apptaskhandle->dataBuffer;
		//timer for blocking measurement
		timer_handle = timer_configure(TIMER0BASE, TIMER0PRCM, period, blocking_timer_ISR); // 5ms or other from input
		//timer for measuring time
		timer_handle = timer_configure(TIMER1BASE, TIMER1PRCM, 200, counting_timer_ISR); //set the time count between each interrupt
		//timer for period measurement
		counter_handle = counter_configure(TIMER2BASE, TIMER2PRCM);

		max = (*(apptaskhandle->dataBuffer+1) * 60 * 1000) / 200; //get the time count interrupt times

		setT2CMAX(max); // 1 min or other, input numbe with min unit

		RedLED_configure(); //in order to show the processing statu

		I2Cwritebuff[0]= 0x11;
		I2C_Write(I2C_Handler, 0x1d, 0x2A, I2Cwritebuff, 1, 0x500); //configure MMA8452 device mode

		timerS_start();

		while(timer_handle->TimerOn){
			timer_hold(30);

			//I2C_read(I2C_Handler, 0x1d, 0x01, bufferpointer, 6, 0x500)

			if( I2C_read(I2C_Handler, 0x1d, 0x01, bufferpointer, 6, 0x500) == 0){

				//*bufferpointer = 1;
				//*(bufferpointer+1) = 2;
			    //*(bufferpointer+2) = 3;
		    	//*(bufferpointer+3) = 4;
			    //*(bufferpointer+4) = 5;
			    //*(bufferpointer+5) = 6;
				bufferpointer = bufferpointer + 6;
				buffercount = buffercount + 6;
//******************new added lines for data analysis 09/08

				zSignal = 0; //clear zSignal
				zSignal = (*(bufferpointer-2) << 8) + *(bufferpointer-1);



				output = Execute_RR_Detector(handle, (zSignal & 0xFFF0));

				if(output & EXHALE_PULSE){
					*bufferpointer = 0x05; // exhale = 0x05
					LEDcontrol = 0x00;	//off
					waveByte = 6;
					//determine starting count or counting
					if(counter_handle->counterOn){
						//get counting
						periodMesured = counter_periodMeasured(counter_handle);
						counter_reset(counter_handle);
						breathRate = MINUTE/periodMesured;
					}else{
						//starting count
						counter_start(counter_handle);
					}

					LEDControlOnPaulse(LEDcontrol,counter_handle, periodMesured);

				}else if(output & INHALE_PULSE){
					*bufferpointer = 0x03; //0x03
					LEDcontrol = 0x01;	//on
					waveByte = 8;
					LEDControlOnPaulse(LEDcontrol,counter_handle, periodMesured);
				}else{
					*bufferpointer = 0x04; // no pulse 0x04
					LEDControlOnPaulse(LEDcontrol,counter_handle, periodMesured);
				}

				bufferpointer = bufferpointer + 1;
				buffercount = buffercount + 1;

				*bufferpointer = breathRate;//breathRate;

				bufferpointer = bufferpointer + 1;
				buffercount = buffercount + 1;

				*bufferpointer = waveByte;

				bufferpointer = bufferpointer + 1;
				buffercount = buffercount + 1;

//*********************************************************
				//I2CreadCount = I2CreadCount + 1;
			}
			//********************
			if(buffercount == SENDMAX){
				//buffer full
				//***********task forward data to other task*************
				SendControlData(UARTPRINT, SENDMAX, bufferpointercpy);
				bufferpointer = memset(bufferpointercpy,0,SENDMAX*sizeof(uint8_t)); // read buffer clear  bufferpointercpy
				// reset buffer pointer  bufferpointercpy
				buffercount = 0; //reset buffer count
				//SendControlData(0x6400, 0x01, &breathRate);
			}

		}

//******************test value area*************

//		unsigned long I2CRC;
//		I2CRC = I2C_Handler->readCount;

//		unsigned long I2CWC;
//		I2CWC = I2C_Handler->sentCount;

//		unsigned long UARTC;
//		UARTC = getUARTC();

//		unsigned long TC;
//		TC = getTimerCount();

		unsigned long UARTsendCount;
		UARTsendCount = UART_getSendCount();
//**********************************************
		RedLED_close();
		//free buffer
		free(bufferpointercpy);

		//close timer
		timer_close(TIMER0BASE, TIMER0PRCM);
		timer_close(TIMER1BASE, TIMER1PRCM);

		//close counter
		counter_close(TIMER2BASE, TIMER2PRCM, counter_handle);

		//free input command and data memory space
		free(apptaskhandle->dataBuffer);
		free(apptaskhandle); // free memory space

	}
}







