//*****************************************
//i2cnew.c
//i2c without polling
//*****************************************
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "string.h"
#include "i2cnew.h"
#include "hw_types.h"
#include "pin.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "hw_i2c.h"
#include "i2c.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "rom_map.h"
#include <queue.h>

#define WRITEBUFFERSIZE 				16
#define READBUFFERSIZE 				16
static BaseType_t xHigherPriorityTaskWoken;

I2C_HANDLE I2C_Handle;

//***********************************************************
//ISRI2C
//Interrupt service routine for I2C
//parameters: none
//description: ISR for both I2C write and I2C read
//return: none
//***********************************************************
void ISRI2C(void) {

		xHigherPriorityTaskWoken = pdFALSE; //if giving semaphore wake a task, forcing context exchange

		//if T FIFO threshold is passed
			if (I2CMasterIntStatusEx(I2C_Handle->base_address, false) & I2C_MASTER_INT_TX_FIFO_REQ){


					I2CTxFIFOFlush(I2C_Handle->base_address);

					if(I2C_Handle->WriteLength <= I2C_FIFOMAX){
						//last <8 bytes to send

						I2C_Handle->loopEnd = I2C_Handle->WriteLength;
						I2C_Handle->loopCount = 0;
						while(I2C_Handle->loopCount < I2C_Handle->loopEnd){
							//I2CFIFODataPut(I2C_Handle->base_address, *I2C_Handle->pCurrentWrite);
							if(I2CFIFODataPutNonBlocking(I2C_Handle->base_address, *I2C_Handle->pCurrentWrite)){
								(I2C_Handle->pCurrentWrite)++; // I2C buffer pointer add one
							    (I2C_Handle->WriteLength)--;	// length minus one
							    I2C_Handle->loopCount++;

							    I2C_Handle->sentCount++; //0920
							}

						}

						I2CMasterIntClearEx(I2C_Handle->base_address, I2C_MASTER_INT_TX_FIFO_REQ); //clear raw interrupt


						xSemaphoreGiveFromISR( I2C_Handle->I2CWrite_Semaphore, &xHigherPriorityTaskWoken );// release semaphore
						portYIELD_FROM_ISR( xHigherPriorityTaskWoken );//forcing context exchange


						I2CMasterControl(I2C_Handle->base_address, I2C_MASTER_CMD_FIFO_BURST_SEND_FINISH);
					}else{
						I2C_Handle->loopCount = 0;
							while(I2C_Handle->loopCount < I2C_FIFOMAX){
							//I2CFIFODataPut(I2C_Handle->base_address, *I2C_Handle->pCurrentWrite);
							if(I2CFIFODataPutNonBlocking(I2C_Handle->base_address, *I2C_Handle->pCurrentWrite)){
								(I2C_Handle->pCurrentWrite)++; // I2C buffer pointer add one
							    (I2C_Handle->WriteLength)--;	// length minus one
							    I2C_Handle->loopCount++;

							    I2C_Handle->sentCount++; //0920
							}

						}

		                I2CMasterIntClearEx(I2C_Handle->base_address, I2C_MASTER_INT_TX_FIFO_REQ); //clear raw interrupt

						I2CMasterControl(I2C_Handle->base_address, I2C_MASTER_CMD_FIFO_BURST_SEND_CONT);
					}

					return;
			}

	//if R FIFO threshold passed or data remain while burstcount is 0 (finished transmit)
		if (I2CMasterIntStatusEx(I2C_Handle->base_address, false) & I2C_MASTER_INT_RX_FIFO_REQ){

			while(I2CFIFODataGetNonBlocking(I2C_Handle->base_address, I2C_Handle->pCurrentRead)){
					(I2C_Handle->ReadLength)--;
					(I2C_Handle->pCurrentRead)++; // for next step determination

					I2C_Handle->readCount++;			//debugging 0920
			}

			I2CMasterIntClearEx(I2C_Handle->base_address, I2C_MASTER_INT_RX_FIFO_REQ); //clear raw interrupt

			if(I2C_Handle->ReadLength == 0 & I2C_Handle->ReadFinish == 1){
				memcpy(I2C_Handle->pDataPut, I2C_Handle->pReadBuf,I2C_Handle->ReadLengthcpy); //copy data from I2C buffer to general read buffer

				if(I2C_Handle->ReadLengthcpy > 8){
				I2CMasterBurstLengthSet(I2C_Handle->base_address, I2C_Handle->ReadLength); // update burstlength before calling command
				I2CMasterControl(I2C_Handle->base_address, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_FINISH);
				}

				I2C_Handle->ReadFinish = 0;
				xSemaphoreGiveFromISR( I2C_Handle->I2CRead_Semaphore, &xHigherPriorityTaskWoken );// release semaphore
				portYIELD_FROM_ISR( xHigherPriorityTaskWoken );//forcing context exchange
			}

		}
}
//***********************************************************
//I2C_write
//parameters:
//unsigned char DeviceAddr :     slave device address
//unsigned char length :    length of data need to send
//description: function to execute single and burst I2C write
//return: none
 //***********************************************************
int32_t I2C_Write(I2C_HANDLE I2C_Handle, uint8_t DeviceAddr, uint8_t RegisterAddr, uint8_t *pData, uint16_t length, uint32_t time_out) {


	if(length + 1 <= I2C_Handle->WriteBufSize){
		I2C_Handle->WriteLength = length + 1; // 1 for register address space
		I2C_Handle->ReadLength = 0;
		*I2C_Handle->pWriteBuf = RegisterAddr;	//put register address in write buffer
		memcpy(I2C_Handle->pWriteBuf + 1, pData , length); //move data from general write buffer into I2C buffer
		I2C_Handle->reValue = I2C_readwrite(I2C_Handle, DeviceAddr, time_out); //call readwrite

		return I2C_Handle->reValue; // return error check
	}
	else {
		return FAILURE; // directly return error, wrong length
	}
}



//***********************************************************
//I2C_read
//parameters:
//unsigned char DeviceAddr :     slave device address
//unsigned char length :    length of data need to receive
//description: function to execute burst I2C read
//return: error check
//***********************************************************
int32_t I2C_read(I2C_HANDLE I2C_Handle, uint8_t DeviceAddr, uint8_t RegisterAddr, uint8_t *pData, uint16_t length ,uint32_t time_out) {


	if(length <= I2C_Handle->ReadBufSize){
		I2C_Handle->ReadFinish = 1;
		I2C_Handle->pDataPut = pData; // store address to put data
		I2C_Handle->ReadLength = length; // receiving n length data only need n-1 as determine number in this method
		I2C_Handle->ReadLengthcpy = length;
		I2C_Handle->WriteLength = 1;	//set length of write to write register address to read from
		*I2C_Handle->pWriteBuf = RegisterAddr;	//put register address in write buffer
		I2C_Handle->reValue = I2C_readwrite(I2C_Handle, DeviceAddr, time_out); //call readwrite

		return I2C_Handle->reValue; // return error check
	}
	else {
		return FAILURE; // directly return error, wrong length
	}

}
//***********************************************************
//I2C_readwrite
//***********************************************************
int32_t I2C_readwrite(I2C_HANDLE I2C_Handle, uint8_t DeviceAddr, uint32_t time_out){


	//write part:
	if(I2C_Handle->WriteLength > 0){
	//			I2CMasterIntClearEx(I2C_Handle->base_address, I2C_MASTER_INT_TX_FIFO_REQ); // clear INT to make sure the first INR can be triggered
				I2CMasterSlaveAddrSet(I2C_Handle->base_address, DeviceAddr, false); // set address
				I2C_Handle->pCurrentWrite = I2C_Handle->pWriteBuf; //initial I2C buffer pointer to the start of I2C Write buffer
				I2CTxFIFOConfigSet(I2C_Handle->base_address, I2C_FIFO_CFG_TX_MASTER | I2C_FIFO_CFG_TX_TRIG_1 );
				I2CMasterBurstLengthSet(I2C_Handle->base_address, I2C_Handle->WriteLength); // maximum is 256

				if(I2C_Handle->WriteLength <= I2C_FIFOMAX){

					I2C_Handle->loopEnd = I2C_Handle->WriteLength;
					for(I2C_Handle->loopCount = 0; I2C_Handle->loopCount < I2C_Handle->loopEnd; I2C_Handle->loopCount++){
						//I2CFIFODataPut(I2C_Handle->base_address, *I2C_Handle->pCurrentWrite);
						if(	I2CFIFODataPutNonBlocking(I2C_Handle->base_address, *I2C_Handle->pCurrentWrite) == 0){
							while(1); //error
						}
						(I2C_Handle->pCurrentWrite)++; // I2C buffer pointer add one
						(I2C_Handle->WriteLength)--;	// length minus one

						I2C_Handle->sentCount++; //0920
					}

					I2CMasterControl(I2C_Handle->base_address, I2C_MASTER_CMD_FIFO_SINGLE_SEND);

					if(I2C_Handle->WriteLength == 0){
						xSemaphoreGive( I2C_Handle->I2CWrite_Semaphore );// release semaphore
					}

				}else{
					I2C_Handle->loopCount = 0;
					for(I2C_Handle->loopCount = 0; I2C_Handle->loopCount < I2C_FIFOMAX; I2C_Handle->loopCount++){
						//I2CFIFODataPut(I2C_Handle->base_address, *I2C_Handle->pCurrentWrite);
						if(I2CFIFODataPutNonBlocking(I2C_Handle->base_address, *I2C_Handle->pCurrentWrite) == 0){
							while(1); // error
						}
						(I2C_Handle->pCurrentWrite)++; // I2C buffer pointer add one
						(I2C_Handle->WriteLength)--;	// length minus one

						I2C_Handle->sentCount++; //0920
					}

					I2CMasterControl(I2C_Handle->base_address, I2C_MASTER_CMD_FIFO_BURST_SEND_START);
				}


				if(xSemaphoreTake(I2C_Handle->I2CWrite_Semaphore, time_out/(portTICK_PERIOD_MS) ) == pdTRUE){
					I2C_Handle->reValue = SUCCESS; //wait return till write complete
				}
				else{
					return WRITETIMEOUT; // timeout (ms)
				}

	}

	//read part

	if(I2C_Handle->ReadLength > 0){
	//			I2CMasterIntClearEx(I2C_Handle->base_address, I2C_MASTER_INT_RX_FIFO_REQ); // clear INT
				memset(I2C_Handle->pReadBuf,0,I2C_Handle->ReadBufSize);//clear buffer
				I2C_Handle->pCurrentRead = I2C_Handle->pReadBuf; // pointer initialization
				I2CTxFIFOConfigSet(I2C_Handle->base_address, I2C_FIFO_CFG_RX_MASTER | I2C_FIFO_CFG_RX_TRIG_7);
				I2CMasterBurstLengthSet(I2C_Handle->base_address, I2C_Handle->ReadLength); // maximum is 256
				I2CMasterSlaveAddrSet(I2C_Handle->base_address, DeviceAddr , true); // set slave address

				// check if next transmit is last <= 8byte to receive
					if (I2C_Handle->ReadLength <= I2C_FIFOMAX){
						I2CMasterControl(I2C_Handle->base_address, I2C_MASTER_CMD_FIFO_SINGLE_RECEIVE);
					}else{
						I2CMasterControl(I2C_Handle->base_address, I2C_MASTER_CMD_FIFO_BURST_RECEIVE_START); //execute first transmit
					}

			if(xSemaphoreTake( I2C_Handle->I2CRead_Semaphore,  time_out/(portTICK_PERIOD_MS) ) == pdTRUE){

				I2C_Handle->reValue = SUCCESS; //success
			}
			else{
				I2C_Handle->reValue = READTIMEOUT; //timeout
			}

	}

	return I2C_Handle->reValue; // return error check

}

//***********************************************************
//I2C_open
//parameters:
//ulMode : hardware operation code
//description: function to open I2C
//return: none
//***********************************************************
I2C_HANDLE I2C_open(uint32_t peripheral, uint32_t base_address, int32_t speed){

	I2C_Handle = (I2C_HANDLE)malloc(sizeof(I2C_STATE));

	if(I2C_Handle == NULL){
		return NULL;
	}

	I2C_Handle->Speed = speed;
	I2C_Handle->WriteBufSize = WRITEBUFFERSIZE;
	I2C_Handle->ReadBufSize = READBUFFERSIZE;
	I2C_Handle->pWriteBuf = NULL;
	I2C_Handle->pReadBuf = NULL;

	I2C_Handle->sentCount = 0;	//debugging purpose
	I2C_Handle->readCount = 0;	//0920

	I2C_Handle->pWriteBuf = (uint8_t*)malloc(I2C_Handle->WriteBufSize * sizeof(uint8_t));
	if(I2C_Handle->pWriteBuf == NULL){
		I2C_close(I2C_Handle);
		return NULL;
	}

	I2C_Handle->pReadBuf = (uint8_t*)malloc(I2C_Handle->ReadBufSize * sizeof(uint8_t));
	if(I2C_Handle->pReadBuf == NULL){
		I2C_close(I2C_Handle);
		return NULL;
	}

	I2C_Handle->I2CWrite_Semaphore = NULL;
	I2C_Handle->I2CRead_Semaphore = NULL;

    vSemaphoreCreateBinary( I2C_Handle->I2CWrite_Semaphore ); //semaphore create

    if(I2C_Handle->I2CWrite_Semaphore == NULL){
    	I2C_close(I2C_Handle);
    	return NULL;
    }

    vSemaphoreCreateBinary( I2C_Handle->I2CRead_Semaphore ); //semaphore create

    if(I2C_Handle->I2CRead_Semaphore == NULL){
        I2C_close(I2C_Handle);
        return NULL;
    }

	I2C_Handle->base_address = base_address;
	I2C_Handle->peripheral = peripheral;


 // Enable I2C Peripheral
     //MAP_HwSemaphoreLock(HWSEM_I2C, HWSEM_WAIT_FOR_EVER);
     MAP_PRCMPeripheralClkEnable(I2C_Handle->peripheral, PRCM_RUN_MODE_CLK);
     MAP_PRCMPeripheralReset(I2C_Handle->peripheral);

     // Configure PIN_01 for I2C0 I2C_SCL

     MAP_PinTypeI2C(PIN_01, PIN_MODE_1);

     // Configure PIN_02 for I2C0 I2C_SDA

     MAP_PinTypeI2C(PIN_02, PIN_MODE_1);

     // Configure I2C module in the specified mode

     switch(I2C_Handle->Speed)
     {
         case I2C_MASTER_MODE_STD:       /* 100000 */
             MAP_I2CMasterInitExpClk(I2C_Handle->base_address,SYS_CLK,false);
             break;

         case I2C_MASTER_MODE_FST:       /* 400000 */
             MAP_I2CMasterInitExpClk(I2C_Handle->base_address,SYS_CLK,true);
             break;

         default:
             MAP_I2CMasterInitExpClk(I2C_Handle->base_address,SYS_CLK,true);
             break;
     }

     I2CMasterIntEnableEx(I2C_Handle->base_address, I2C_MASTER_INT_RX_FIFO_REQ | I2C_MASTER_INT_TX_FIFO_REQ ); //set unmask for I2C data raw interrupt
     I2CIntRegister(I2C_Handle->base_address, ISRI2C); // hook ISR for I2C
     IntEnable(INT_I2CA0); // enable I2C interrupt, also need to enable global interrupt, which is done in BoardInit function


     xSemaphoreTake( I2C_Handle->I2CRead_Semaphore, portMAX_DELAY );
     xSemaphoreTake( I2C_Handle->I2CWrite_Semaphore, portMAX_DELAY );

     return I2C_Handle;
}



int32_t I2C_close(I2C_HANDLE I2C_Handle)
{
    // Power OFF the I2C peripheral
    MAP_PRCMPeripheralClkDisable(I2C_Handle->peripheral, PRCM_RUN_MODE_CLK);

    // Release created semaphores
    if(I2C_Handle->I2CWrite_Semaphore != NULL)
    	vSemaphoreDelete( I2C_Handle->I2CWrite_Semaphore );

    if(I2C_Handle->I2CRead_Semaphore != NULL)
    	vSemaphoreDelete( I2C_Handle->I2CRead_Semaphore );

    // Release allocated memory space
    if(I2C_Handle->pReadBuf != NULL)
    	free(I2C_Handle->pReadBuf);

    if(I2C_Handle->pWriteBuf != NULL)
    	free(I2C_Handle->pWriteBuf);

    if(I2C_Handle != NULL)
    	free(I2C_Handle);

    return SUCCESS;
}

 //***********************************************************



