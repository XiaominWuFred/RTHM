/*
 * i2cnew.h
 *
 *  Created on: Apr 3, 2017
 *      Author: xwu
 */

#ifndef I2CNEW_H_
#define I2CNEW_H_

#include <FreeRTOS.h>
#include <semphr.h>
#include "hw_memmap.h"

#define SYS_CLK                 80000000
#define FAILURE                 -1			//wrong length
#define SUCCESS                 0
#define WRITETIMEOUT			-100
#define READTIMEOUT           	-101
#define I2C_FIFOMAX				8
#define I2C_MASTER_MODE_STD     0
#define I2C_MASTER_MODE_FST     1
#define I2C0_BASE 				I2CA0_BASE
#define	I2C0_PRCM				PRCM_I2CA0

typedef struct {
	int32_t Speed;							//I2C speed mode
	uint32_t base_address;					//I2C base address
	uint32_t peripheral;					//I2C peripheral address
	uint32_t WriteBufSize;   				//size of write buffer
	uint32_t ReadBufSize;					//size of read buffer
	int32_t WriteLength;					//length of message to write
	int32_t ReadLength;						//length of message to read
	int32_t ReadLengthcpy;					//length of message to read for copy data
	uint8_t *pWriteBuf;						//pointer to write buffer
	uint8_t *pReadBuf;						//pointer to read buffer
	uint8_t *pCurrentWrite;					//current position of pointer to write buffer
	uint8_t *pCurrentRead;					//current position of pointer to read buffer
	uint8_t ReadFinish;						//flag to indicate read finish
	uint8_t *pDataPut; 	     				//pointer points to destination address

	SemaphoreHandle_t I2CWrite_Semaphore;	//write semaphore for sequential control
	SemaphoreHandle_t I2CRead_Semaphore;	//read semaphore for sequential control

	uint8_t loopCount, loopEnd;
	int32_t reValue; // return error check

	uint32_t sentCount;						//for error checking and debugging
	uint32_t readCount;						//debugging 0920

}I2C_STATE;

typedef I2C_STATE* I2C_HANDLE;

I2C_HANDLE I2C_open(uint32_t peripheral, uint32_t base_address, int32_t speed);

int32_t I2C_Write(I2C_HANDLE I2C_Handle, uint8_t DeviceAddr, uint8_t RegisterAddr, uint8_t *pData, uint16_t length, uint32_t time_out);

int32_t I2C_read(I2C_HANDLE I2C_Handle, uint8_t DeviceAddr, uint8_t RegisterAddr, uint8_t *pData, uint16_t length, uint32_t time_out);

int32_t I2C_readwrite(I2C_HANDLE I2C_Handle, uint8_t DeviceAddr, uint32_t time_out);

int32_t I2C_close();

#endif /* I2CNEW_H_ */
