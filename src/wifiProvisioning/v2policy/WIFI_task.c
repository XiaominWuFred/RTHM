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
#include <stdbool.h>
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

WiFi_INFO_HANDLE_t wifi_info_handle;

bool AP_config_finish;
extern SemaphoreHandle_t WIFIAP_Semaphore;
extern unsigned char  WiFi_ConnectionSSID[SSID_LEN_MAX+1];

/*
 * return wifi_info_handle
 * for other task to access same wifi_info_handle
 */
WiFi_INFO_HANDLE_t getWifi_info_handle(){
	return wifi_info_handle;
}

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

	//necessary local variable
	int32_t lRetVal;	//variable for checking function return

	AP_config_finish = 0;

	//file pre-construction
	lRetVal = Initial_SimpleLink();
	if(lRetVal < 0){
		//error
	}

	wifi_info_handle = CreateWifiInfo();//create wifiinfo data field

	//get flash file
	lRetVal = AutoConnectFromProfile(wifi_info_handle); //precondtion: sl_start() on
	if(lRetVal == 0){
		//success

		//CloseWifiInfo(wifi_info_handle);
	}else if(lRetVal == NO_USABLE_WIFI_INFO_CONTAINED){
		//ap mode to sta mode and update profile
		// no match, go SA mode wait for configuration

			//Configure_AP(wifi_info_handle); //SA mode for user configuration
			xSemaphoreGive( WIFIAP_Semaphore );

			while(!AP_config_finish); //waiting AP configuration in other task
			AP_config_finish = 0;

	}else{
		//other errors
		//CloseWifiInfo(wifi_info_handle);
	}

	//success, start use WIFI
	send_connected_SSID(WiFi_ConnectionSSID,3232235779,5000,6666);



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

