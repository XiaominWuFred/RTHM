/*
 * application_task.h
 *
 *  Created on: Jun 9, 2017
 *      Author: xwu
 */

#ifndef SOURCE_APPLICATION_TASK_H_
#define SOURCE_APPLICATION_TASK_H_

#include "i2cnew.h"
#include "UARTnew.h"
#include "control_task.h"

#define MAX_MSG_LENGTH			16
#define MAX_MSG_NUMBER			10
#define MEMORYALOCATIONFAILED	-1
#define SUCCESS					0

//********TASKID AREA***************
#define TASK1ID			 0X61
#define TASK2ID			 0x62
#define TASK3ID			 0x63
#define TASK4ID			 0x64



I2C_HANDLE getI2CHandle();

#endif /* SOURCE_APPLICATION_TASK_H_ */
