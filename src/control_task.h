/*
 * control_task.h
 *
 *  Created on: Jun 9, 2017
 *      Author: xwu
 */

#ifndef SOURCE_CONTROL_TASK_H_
#define SOURCE_CONTROL_TASK_H_

#include "i2cnew.h"
#include "UARTnew.h"
#include "osi.h"

#define TASKNUMBER				3
#define OUTOFLIST		   		-11
#define MEMORYALOCATIONFAILED	-1
#define NOMATCH					-2
#define ERROR					-5
#define SUCCESS					0

//MAX_MSG_LENGTH and MAX_MSG_NUMBER should be determined by certain project need
#define MAX_MSG_LENGTH			sizeof(void*)           //message size (size of buffer pointer)
#define MAX_MSG_NUMBER			0x05                    //message number holding
#define MEMORYALOCATIONFAILED	-1
#define SUCCESS					0

//CONTROLTASKID can be modified by user if needed
#define CONTROLTASKID			0x41 //A
//call ControlTaskInitiail; in the main before other app tasks initial
#define ControlTaskInitiail		setControlHandle(initialControlTask())

//*************************Task Infomation Area*********************************
//this area contain all the infomation of application tasks
//if user want to add more application tasks
//step1: copy a application_taskN.c file
//step2: change the identify label in that file
//step3: user must copy one block of application task infomation and paste here
//step4: all infomation of one app task should be modified to be unique
//       and identical to those label in the application_taskN.c file
//step5: in the main.c file call Task1Initial; and create task using TASKN
//example:
//      declaration:
//       void control_task( void *pvParameters );
//       void TASK1( void *pvParameters );
//       void TASK2( void *pvParameters );
//       void TASK3( void *pvParameters );
//
//      call initial:
//       ControlTaskInitiail;
//       Task1Initial;
//       Task2Initial;
//       Task3Initial;
//
//      task create:
//       osi_TaskCreate( control_task, "Control", OSI_STACK_SIZE, NULL, 2, NULL );
//       osi_TaskCreate( TASK1, "appTask1", OSI_STACK_SIZE,NULL, 1, NULL );
//       osi_TaskCreate( TASK2, "appTask2", OSI_STACK_SIZE,NULL, 1, NULL );
//       osi_TaskCreate( TASK3, "uartTask", OSI_STACK_SIZE,NULL, 1, NULL );
//
//template:
//#define TASKn				    taskname
//#define TASKnID				0x61                           // any 8bits binary
//#define TASKnHANDLE			tasknHandle
//#define SETTASKnHANDLE		SetTasknHandle
//#define TasknInitial		    SETTASKnHANDLE(initialAppTask(TASKnID))

//example for 3 app tasks: can be deleted
#define TASK1				taskname1
#define TASK1ID				0X61
#define TASK1HANDLE			task1Handle
#define SETTASK1HANDLE		SetTask1Handle
#define Task1Initial		SETTASK1HANDLE(initialAppTask(TASK1ID))

#define TASK2				taskname2
#define TASK2ID				0x62
#define TASK2HANDLE			task2Handle
#define SETTASK2HANDLE		SetTask2Handle
#define Task2Initial		SETTASK2HANDLE(initialAppTask(TASK2ID))

#define TASK3				taskname3
#define UARTTASKID			0x63
#define TASK3HANDLE			uartTaskHandle
#define SETTASK3HANDLE		SetTask3Handle
#define Task3Initial		SETTASK3HANDLE(initialAppTask(TASK3ID))

#define TASK4				WIFItask
#define TASK4ID				0x64
#define TASK4HANDLE			WIFITaskHandle
#define SETTASK4HANDLE		SetTask4Handle
#define Task4Initial		SETTASK4HANDLE(initialAppTask(TASK4ID))

#define TASK6				demotask
#define TASK6ID				0x66
#define TASK6HANDLE			DemoTaskHandle
#define SETTASK6HANDLE		SetTask6Handle
#define Task6Initial		SETTASK6HANDLE(initialAppTask(TASK6ID))

//******************************************************************************
typedef struct {
uint8_t taskID;
OsiMsgQ_t taskQ;
} TASKINLIST_t;

typedef TASKINLIST_t* TASKINLIST_HANDLE_t;

typedef struct{
	uint16_t controlCommand;
	uint16_t dataSize;
	uint8_t *dataBuffer;
}DATA_STATE_t;

typedef DATA_STATE_t* DATA_HANDLE_t;

typedef struct{
	OsiMsgQ_t ControlTaskQ;
	SemaphoreHandle_t SendControlData_Semaphore;
	TASKINLIST_t taskList[TASKNUMBER]; // array for 3 tasks
	uint8_t task_ID;
	int32_t loopIndex;
	int32_t listIndex;

}CONTROL_STATE_t;

typedef CONTROL_STATE_t* CONTROL_HANDLE_t;

typedef struct {
	OsiMsgQ_t TASKQ;
	uint8_t taskID;
}TASK_STATE_t;

typedef TASK_STATE_t* TASK_HANDLE_t;

void setControlHandle(CONTROL_HANDLE_t taskHandle);
void SETTASK1HANDLE(TASK_HANDLE_t taskHandle);
void SETTASK2HANDLE(TASK_HANDLE_t taskHandle);
void SETTASK3HANDLE(TASK_HANDLE_t taskHandle);

int32_t SendControlData_S_initial();

int32_t SendControlData(uint16_t controlCommand, uint16_t dataSize, uint8_t* dataBuffer);

int32_t SetCommandToTask(OsiMsgQ_t TaskQ, uint16_t controlCommand, uint16_t dataSize, uint8_t* dataBuffer);

int32_t registerTaskToControl(uint8_t taskID, OsiMsgQ_t taskQ);

CONTROL_HANDLE_t initialControlTask();

TASK_HANDLE_t initialAppTask(uint8_t TASKID);

OsiMsgQ_t taskQCreat(OsiMsgQ_t TASKQ);

int32_t CurrentCommendStop(TASK_HANDLE_t TASKHANDLE);
//**************************customer function declaration if needed*************
I2C_HANDLE getI2CHandle();

#endif /* SOURCE_CONTROL_TASK_H_ */
