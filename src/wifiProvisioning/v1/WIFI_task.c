/*
 * WIFI_task.c
 *
 *  Created on: Oct 31, 2017
 *      Author: xwu
 */


#include "string.h"
// Standard includes.
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "osi.h"





#include "WiFiConnection.h"
#include "WiFiEvent.h"
#include "WiFiSetting.h"

#include "control_task.h"
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

#include "command.h"
// signl processing
#include "common.h"

#define LISTENPORT			10

/********************************************
 * Task's data field handle
 * each application task should have different data field handle
 * customer's responsibility to rename the handle
 * and part in its setter after create new app task
 */
TASK_HANDLE_t TASK4HANDLE;

void SETTASK4HANDLE(TASK_HANDLE_t taskHandle){
	TASK4HANDLE = taskHandle;
}

unsigned long status;



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
void TASK4(){
	//handle variable for data communication
	DATA_HANDLE_t apptaskhandle;

	//necessary handler
	WiFi_INFO_HANDLE_t wifi_info_handle;
	//necessary local variable
	tBoolean ReCheck;
	int32_t lRetVal;
	//file pre-construction
	lRetVal = Initial_SimpleLink();
	if(lRetVal < 0){
		//error
	}
	wifi_info_handle = CreateWifiInfo();

	//get flash file
	wifi_info_handle = GetWiFiInfoFromFlash(wifi_info_handle); //precondtion: sl_start() on

	//search wifi list and compare if any match
	ReCheck = SearchWiFiList(wifi_info_handle);
	if(ReCheck == true){
		// match, do STA connection
		sl_Start(NULL,NULL,NULL);
		ReCheck = Configure_STA(wifi_info_handle);
		if(ReCheck == true){
			//success and finish

			CloseWifiInfo(wifi_info_handle);
		}else{
			//passcode not match go SA mode wait for configuration
			while(true){
				wifi_info_handle = Configure_AP(wifi_info_handle); //SET SA mode for user configuration
				ReCheck = Configure_STA(wifi_info_handle); //STA mode try to connect router
				if(ReCheck == true){
					break; //done
				}
			}
			//success
			ReCheck = UpdateWiFiInfoToFlash(wifi_info_handle);
			if(ReCheck == true){
				//success
				CloseWifiInfo(wifi_info_handle);
			}else{
				//flash write error
			}
		}

	}else{
		// no match, go SA mode wait for configuration
		while(true){
			wifi_info_handle = Configure_AP(wifi_info_handle); //SA mode for user configuration
			ReCheck = Configure_STA(wifi_info_handle);	//STA mode try to connect router
			if(ReCheck == true){
				break; //done
			}
		}
		//success
		ReCheck = UpdateWiFiInfoToFlash(wifi_info_handle); //update good wifi profile into flash memory
		if(ReCheck == true){
			//success
			CloseWifiInfo(wifi_info_handle);
		}else{
			//flash write error
			CloseWifiInfo(wifi_info_handle);
		}
	}

	//testing by starting a TCP connection
	TCPHandle_t handle;
	handle = TCPOpenConnectionByIP(3232235779, 5000, 10000); //connect to another laptop by IP

	if(handle < 0){
		while(1){//error
		}
	}

	//task's infinite loop
	while(1){
		xQueueReceive( TASK4HANDLE->TASKQ, &apptaskhandle, portMAX_DELAY ); // deQueue
		//do the task's mission using the data in the stucture(put by control task)


//		length = apptaskhandle->dataSize;
//		length = sl_Htonl(length);
//		 TCPWrite(handle,(void *)&length, sizeof(long),5000);
//		 TCPWrite(handle,apptaskhandle->dataBuffer,apptaskhandle->dataSize,5000);


		//free input command and data memory space
		free(apptaskhandle->dataBuffer);
		free(apptaskhandle); // free memory space

	}

}

