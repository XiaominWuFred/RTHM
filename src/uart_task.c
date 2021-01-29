/*
 * uart_task.c
 *
 *  Created on: Jun 23, 2017
 *      Author: xwu
 */

#include "application_task.h"

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

TASK_HANDLE_t TASK3HANDLE;

void SETTASK3HANDLE(TASK_HANDLE_t taskHandle){
	TASK3HANDLE = taskHandle;
}


void TASK3(){
	//*********display buffer for test
	uint8_t uartwritebuffer[] = "UART_TASK PROCESSING COMMAND";

	//***************************************
	DATA_HANDLE_t UARTTaskHandle;

	//initialize
	//****************

	//**************************

	UART_HANDLE UART_Handle;
	UART_Handle = UART_open(PORT1, PORT_PERIPH1, UART_BAUDRATE);

	uint8_t readingCheck;
	uint8_t readbuff[2];

	while(1){
		//check if message on Queue -> read or check UART input
		if(uxQueueMessagesWaiting( TASK3HANDLE->TASKQ ) != 0){ //may have bugs

		// deQueue
		xQueueReceive( TASK3HANDLE->TASKQ, &UARTTaskHandle, 0x0A );
		//do the task's mission using the data in the stucture(put by control task)
		//Print out the input data.

		UART_write(UART_Handle, UARTTaskHandle->dataBuffer, UARTTaskHandle->dataSize, 0xff );


		//let control task take new command
		//free allocated memory
		free(UARTTaskHandle->dataBuffer);
		free(UARTTaskHandle); // free memory space
		}else{
			//check uart
			readingCheck = 0;

			UART_read(UART_Handle, &readingCheck,0x01, 0x14); //wait 10Ms for Enter
			//user must enter a single Carriage return in order to input command
			if(readingCheck == 0x0D){
				//uart_write to show user the input can begin

//				UART_write(UART_Handle, tempbuff, sizeof(tempbuff) , 0xff );

				//start uart read
				UARTTaskHandle = (DATA_HANDLE_t)malloc(sizeof(DATA_STATE_t));

				 //check memory allocation result
				  if(UARTTaskHandle == NULL){
					  while(1);// error checked for debugging
				  }

				UART_read(UART_Handle, readbuff, 0x02, 0xffff); // read command uint16_t

				UARTTaskHandle->controlCommand = (readbuff[0] << 8) + readbuff[1]; //may have bug

				UART_read(UART_Handle, readbuff, 0x02, 0xffff); // read dataSize uint16

//				UARTTaskHandle->dataSize = ((readbuff[0]-48) << 8) + (readbuff[1]-48);

				UARTTaskHandle->dataSize = ((readbuff[0]) << 8) + (readbuff[1]);

				UARTTaskHandle->dataBuffer = (uint8_t *)malloc(UARTTaskHandle->dataSize * sizeof(uint8_t));

					if(UARTTaskHandle->dataBuffer == NULL){
						free(UARTTaskHandle);
					    while(1);// error checked for debugging
					}

				UART_read(UART_Handle, UARTTaskHandle->dataBuffer, UARTTaskHandle->dataSize, 0xffff); // read data

				//***********self ID?
				if((uint8_t)(UARTTaskHandle->controlCommand >> 8) == TASK3HANDLE->taskID){
					//processing input command with data
					//for test, using a UART write to show program passed here
					UART_write(UART_Handle, uartwritebuffer, sizeof(uartwritebuffer) , 0xff );
				}else{
					//send command and data to control
					//call SetControlData
					SendControlData(UARTTaskHandle->controlCommand, UARTTaskHandle->dataSize, UARTTaskHandle->dataBuffer);
				}

				//finished using memory, free it
				free(UARTTaskHandle->dataBuffer);
				free(UARTTaskHandle);

			}
		}
	}
}


