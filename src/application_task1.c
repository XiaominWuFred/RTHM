/*
 * application_task1.c
 *
 *  Created on: Jun 9, 2017
 *      Author: xwu
 */

#include "control_task.h"

// Standard includes.
#include <stdio.h>
#include <stdlib.h>


#include "osi.h"
#include "string.h"
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

I2C_STATE* I2C_Handle;

TASK_HANDLE_t TASK1HANDLE;

void SETTASK1HANDLE(TASK_HANDLE_t taskHandle){
	TASK1HANDLE = taskHandle;
}

void TASK1(){
	DATA_HANDLE_t apptaskhandle;

	//initialize

	//****************
	uint8_t I2Creadbuff[2];
	uint16_t tempvalue;
	uint8_t Displaybuf[] = "      C read from tmp 102";

	I2C_Handle = I2C_open(I2C0_PRCM, I2C0_BASE,1);//I2C_MASTER_MODE_FST

	while(1){

		// deQueue
		xQueueReceive( TASK1HANDLE->TASKQ, &apptaskhandle, portMAX_DELAY );
		//do the task's mission using the data in the stucture(put by control task)

		//**************temperature read and converting**********************
		//0x48 0x00 tmp102
		I2C_read(I2C_Handle, 0x41, 0x01, I2Creadbuff, 2, 0x1000); //read tmp on power up

		tempvalue = (I2Creadbuff[0] << 4) + (I2Creadbuff[1] >> 4);
		tempvalue = (tempvalue*100) / 16;

		Displaybuf[0] = tempvalue / 1000 + 48;
		Displaybuf[1] = (tempvalue % 1000) / 100 + 48;
		Displaybuf[2] = 0x2e;
		Displaybuf[3] = ((tempvalue % 1000) % 100) / 10 + 48;
		Displaybuf[4] = ((tempvalue % 1000) % 100) % 10 + 48;
		//***********test forward data to UART*************

		//malloc new memory for new command and data
		DATA_HANDLE_t apptaskouthandle;

		apptaskouthandle = (DATA_HANDLE_t)malloc(sizeof(DATA_STATE_t));
		  //check memory allocation result
		  if(apptaskouthandle == NULL){
			  while(1);
		  }

		  //create new command and data according to input command and data

		  apptaskouthandle->controlCommand = UARTPRINT;
		  apptaskouthandle->dataSize = sizeof(Displaybuf);
		  apptaskouthandle->dataBuffer = (uint8_t *)malloc(apptaskouthandle->dataSize * sizeof(uint8_t));
		  //check memory allocation result
		  if(apptaskouthandle->dataBuffer == NULL){
			  free(apptaskouthandle);
			  while(1);
		  }

		  memcpy(apptaskouthandle->dataBuffer, Displaybuf, apptaskouthandle->dataSize);

		//free input command and data memory space
		free(apptaskhandle->dataBuffer);
		free(apptaskhandle); // free memory space

		//send back new command and data to control , call SendControlData

		SendControlData(apptaskouthandle->controlCommand, apptaskouthandle->dataSize, apptaskouthandle->dataBuffer);

		//free new allocated memory

		free(apptaskouthandle->dataBuffer);
		free(apptaskouthandle);

	}
}

I2C_HANDLE getI2CHandle(){
	while(I2C_Handle == NULL){
		osi_Sleep(100);
	}
	return I2C_Handle;
}

