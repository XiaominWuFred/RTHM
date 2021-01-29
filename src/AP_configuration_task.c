/*
 * AP_configuration.c
 *
 *  Created on: Jan 3, 2018
 *      Author: xwu
 */
//***************************USAGE GUIDE***************************************
//step1: make sure the defined label name is same to those defined in control_task.h file
//make sure those defined label is unique for each application task
//
//step2: include necessary files for your application
//
//step3: put variables, handles, functions in commended area
//
//and then make sure the file is in right path of your project
//done.
//******************************************************************************

// Standard includes.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "osi.h"
// includes for Frame
#include "control_task.h"
#include "string.h"
// includes for operating system

// Common interface includes
#include "command.h"

#include "WiFiSetting.h"
#include "gpio_control.h"

/********************************************
 * Task's data field handle
 * each application task should have different data field handle
 * customer's responsibility to rename the handle
 * and part in its setter after create new app task
 */
TASK_HANDLE_t TASK6HANDLE;
SemaphoreHandle_t WIFIAP_Semaphore;
extern unsigned char  WiFi_ConnectionSSID[SSID_LEN_MAX+1];
extern uint8_t AP_config_finish;
extern bool Configuring;

void SETTASK6HANDLE(TASK_HANDLE_t taskHandle){
	TASK6HANDLE = taskHandle;
}

/*********************************************
 * main task function
 * the label of this function is used to created a new task in the main function
 * of customer's program
 * it should be the same as in this file and in main.c file
 * customer has to declare the function in main.c and create new task using this label
 *
 */
void TASK6(){
    //handle variable for data communication
	DATA_HANDLE_t apptaskhandle;

    //********************customer variable area***********

    //*****************************************************


	WiFi_INFO_HANDLE_t handle;
	int8_t ReCheck;

	RedLED_configure();	//configure a red LED to show push button ISR entered
	PushButton_configure();	//configure push button interrupt

    vSemaphoreCreateBinary( WIFIAP_Semaphore ); //semaphore create
    xSemaphoreTake( WIFIAP_Semaphore,  1000/(portTICK_PERIOD_MS) );	//take semaphore

    if( WIFIAP_Semaphore == NULL){
    	//error
    }

    //task's infinite loop
	while(1){
		Configuring = false;	//blocking bit for ISR miss entering
		xSemaphoreTake( WIFIAP_Semaphore,  portMAX_DELAY );
		Configuring = true;		//the bit is true,then pushbutton ISR can't give semaphore
		while(true){
			handle = getWifi_info_handle();	//get handle from other task
			ReCheck = Configure_AP(handle);	//enter AP configuration
			if(ReCheck < 0){
				while(1);
				//error
			}
			ReCheck = Configure_STA(handle);	//STA mode try to connect router
			if(ReCheck){
				break; //done
			}
		}

		//success
		ReCheck = AddNewProfile(handle); //update good wifi profile into flash memory
		if(ReCheck){
			//success
			//CloseWifiInfo(wifi_info_handle);
		}else{
			//flash write error
			//CloseWifiInfo(wifi_info_handle);
		}

		//success, start use WIFI
		send_connected_SSID(WiFi_ConnectionSSID,3232235779,5000,6666);


	}
}
