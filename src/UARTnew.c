#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "hw_types.h"
#include "pin.h"
#include "uart.h"
#include "UARTnew.h"
#include "prcm.h"
#include "string.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "hw_memmap.h"
#include "hw_uart.h"
#include "rom_map.h"
#include "osi.h"
#include "debug.h"

static BaseType_t xHigherPriorityTaskWoken;

UART_HANDLE UART_Handle;

//**********************
//UARTStatus
//check 0x4000c018 UART status register
//**********************
unsigned long UARTStatus(unsigned long ulBase){
	ASSERT(UARTBaseValid(ulBase));
	return(HWREG(ulBase + UART_O_FR));
}
//*****************************************
//UART_ISR
//Interrupt service routine for
//the UART read and write process
//*****************************************
void UART_ISR(){


	//read FIFO full or read time out
	if(UARTIntStatus(UART_Handle->UART_PORT,false) & (UART_INT_RX | UART_INT_RT)){
		UARTIntClear(UART_Handle->UART_PORT, UART_INT_RX | UART_INT_RT);	//clear INT flag

		while (!(UARTStatus(UART_Handle->UART_PORT) & UART_FR_RXFE)){
			//data reading
			*UART_Handle->pCurrentRead = UARTCharGet(UART_Handle->UART_PORT); //read autoly clear INT
			UART_Handle->pCurrentRead++;
			UART_Handle->ReadLength--;

			//adjust code here:
			if(UART_Handle->ReadLength == 0){
				break;
			}
		}

		//check if read certain bytes finished
		if(UART_Handle->ReadLength == 0){
			memcpy(UART_Handle->dataput, UART_Handle->pReadBuf,UART_Handle->ReadLengthcpy); // copy data back
			xSemaphoreGiveFromISR( UART_Handle->UARTRead_Semaphore, &xHigherPriorityTaskWoken );// release semaphore
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );//forcing context exchange
		}

	}

	//send FIFO empty
	if(UARTIntStatus(UART_Handle->UART_PORT,false) & UART_INT_TX){

		UARTIntClear(UART_Handle->UART_PORT, UART_INT_TX);	//clear INT flag

		if(UART_Handle->WriteLength == BUFFEMPTY){

			UART_Handle->UART_SendComplete = true;

			xSemaphoreGiveFromISR( UART_Handle->UARTWrite_Semaphore, &xHigherPriorityTaskWoken );// release semaphore
			portYIELD_FROM_ISR( xHigherPriorityTaskWoken );//forcing context exchange
		}

			//putting data into send FIFO
				if(UART_Handle->WriteLength > FIFOMAX){
					for( UART_Handle->Cindex = 0 ; UART_Handle->Cindex < FIFOMAX ;){
						if(UARTCharPutNonBlocking(UART_Handle->UART_PORT, *(UART_Handle->pCurrentWrite))){//write autoly clear INT
							(UART_Handle->pCurrentWrite) = (UART_Handle->pCurrentWrite) + 1;
							(UART_Handle->WriteLength) = (UART_Handle->WriteLength) - 1;
							UART_Handle->Cindex = UART_Handle->Cindex + 1;
							UART_Handle->sentCount = UART_Handle->sentCount + 1;
						}
					}
				}else{
					UART_Handle->templength = UART_Handle->WriteLength;
					for( UART_Handle->Cindex = 0; UART_Handle->Cindex < UART_Handle->templength ;){
						if(UARTCharPutNonBlocking(UART_Handle->UART_PORT, *(UART_Handle->pCurrentWrite))){//write autoly clear INT
							(UART_Handle->pCurrentWrite) = (UART_Handle->pCurrentWrite) + 1;
							(UART_Handle->WriteLength) = (UART_Handle->WriteLength) - 1;
							UART_Handle->Cindex = UART_Handle->Cindex + 1;
							UART_Handle->sentCount = UART_Handle->sentCount + 1;
						}
					}
				}

	}

}

//*****************************************
//UART_write
//write certain length of data to UART port
//*****************************************
int32_t UART_write( UART_STATE *UART_Handle, uint8_t *pData, uint32_t length, uint32_t time_out ){


	while(!UART_Handle->UART_SendComplete);	//debugging purpose
	UART_Handle->UART_SendComplete = false;//debugging purpose

	UART_Handle->WriteLength = length;

	if(UART_Handle->WriteLength <= UART_Handle->WriteBufSize){

		UARTIntClear(UART_Handle->UART_PORT, UART_INT_TX);	//clear INT flag

		memcpy(UART_Handle->pWriteBuf,pData,UART_Handle->WriteLength); //copy data into writebuff
		UART_Handle->pCurrentWrite = UART_Handle->pWriteBuf;
		//putting data into send FIFO
		if(UART_Handle->WriteLength > FIFOMAX){
			// if
			for( UART_Handle->Cindex = 0 ; UART_Handle->Cindex < FIFOMAX ;){
				if(UARTCharPutNonBlocking(UART_Handle->UART_PORT, *(UART_Handle->pCurrentWrite))){//write autoly clear INT
					(UART_Handle->pCurrentWrite) = (UART_Handle->pCurrentWrite) + 1;
					(UART_Handle->WriteLength) = (UART_Handle->WriteLength) - 1;
					UART_Handle->Cindex = UART_Handle->Cindex + 1;
					UART_Handle->sentCount = UART_Handle->sentCount + 1;
				}
			}
		}else{
			for( UART_Handle->Cindex = 0 ; UART_Handle->Cindex < FIFOMAX ;){
				if(UARTCharPutNonBlocking(UART_Handle->UART_PORT, *(UART_Handle->pCurrentWrite))){//write autoly clear INT
					(UART_Handle->pCurrentWrite) = (UART_Handle->pCurrentWrite) + 1;
					(UART_Handle->WriteLength) = (UART_Handle->WriteLength) - 1;
					UART_Handle->Cindex = UART_Handle->Cindex + 1;
					UART_Handle->sentCount = UART_Handle->sentCount + 1;
				}
			}
		}

		//start sending
		//UARTEnable(UART_Handle->UART_PORT); //start

		if(UART_Handle->UARTWrite_Semaphore != NULL ) {
			if(xSemaphoreTake(UART_Handle->UARTWrite_Semaphore, time_out/(portTICK_PERIOD_MS) ) == pdTRUE){
				UART_Handle->reValue = WRITESUCCESS; //wait return till write complete

			}else{
				UART_Handle->reValue = WRITETIMEOUT; // timeout (ms)

				 }
		}else{
			while(1); //no Semaphore
			 }

		return UART_Handle->reValue;

	}else{
		return FAILURE;	//wrong length
	}

}

//*****************************************
//UART_read
//read certain length of data from UART port
//*****************************************
int32_t UART_read(UART_STATE *UART_Handle, uint8_t *pData, uint32_t length, uint32_t time_out){



	//later added part
	UARTDisable(UART_Handle->UART_PORT);	//clearUART
	UARTFIFOEnable(UART_Handle->UART_PORT);
	//

	UART_Handle->ReadLength = length;	// set readlength
	UART_Handle->ReadLengthcpy = length;

	if(UART_Handle->ReadLength <= UART_Handle->ReadBufSize){

		UARTIntClear(UART_Handle->UART_PORT, UART_INT_RX | UART_INT_RT);	//clear INT flag
		UART_Handle->dataput = pData; //store the destination buffer address
		UART_Handle->pCurrentRead = UART_Handle->pReadBuf; //set current read

		UARTEnable(UART_Handle->UART_PORT); //start receiving

		//suspend before read ISR finish whole process
		if(UART_Handle->UARTRead_Semaphore != NULL ) {
					if(xSemaphoreTake(UART_Handle->UARTRead_Semaphore, time_out/(portTICK_PERIOD_MS) ) == pdTRUE){
						UART_Handle->reValue = READSUCCESS; //wait return till write complete
					}else{
						UART_Handle->reValue = READTIMEOUT; // timeout (ms)
						 }
				}else{
					while(1); //no Semaphore
					 }

				return UART_Handle->reValue;

	}else{
		return FAILURE; //wrong length
	}
}

//*****************************************
//UART_open
//open UART for certain port and bandrate
//*****************************************
UART_HANDLE UART_open(uint32_t UART_port, uint32_t UART_portperiph, uint32_t UART_baudrate){

	//initialize structure
	UART_Handle = (UART_HANDLE)malloc(sizeof(UART_STATE));

	UART_Handle->ReadBufSize = UARTBUFFERSIZE;
	UART_Handle->WriteBufSize = UARTBUFFERSIZE;
	UART_Handle->UART_PORT = UART_port;
	UART_Handle->UART_PORTPERIPH = UART_portperiph;
	UART_Handle->UART_BRATE = UART_baudrate;
	UART_Handle->pWriteBuf = (uint8_t*)malloc(UART_Handle->WriteBufSize * sizeof(uint8_t));
	UART_Handle->pReadBuf = (uint8_t*)malloc(UART_Handle->ReadBufSize * sizeof(uint8_t));
	UART_Handle->pCurrentWrite = UART_Handle->pWriteBuf;
	UART_Handle->pCurrentRead = UART_Handle->pReadBuf;
	UART_Handle->UARTWrite_Semaphore = NULL;
	UART_Handle->UARTRead_Semaphore = NULL;
	UART_Handle->UARTprotect_Semaphore = NULL;
	UART_Handle->UART_SendComplete = true;

	UART_Handle->sentCount = 0;//debugging purpose

	vSemaphoreCreateBinary( UART_Handle->UARTWrite_Semaphore ); //semaphore create
	vSemaphoreCreateBinary( UART_Handle->UARTRead_Semaphore ); //semaphore create
//	vSemaphoreCreateBinary( UART_Handle->UARTprotect_Semaphore ); //debugging purpose
	xSemaphoreTake( UART_Handle->UARTRead_Semaphore, portMAX_DELAY );	//semaphore take
	xSemaphoreTake( UART_Handle->UARTWrite_Semaphore, portMAX_DELAY );	//semaphore take

    // Enable Peripheral Clocks
    MAP_PRCMPeripheralClkEnable(UART_Handle->UART_PORTPERIPH, PRCM_RUN_MODE_CLK);

    // Configure PIN_55 for UART0 UART0_TX
    MAP_PinTypeUART(PIN_55, PIN_MODE_3);

    // Configure PIN_57 for UART0 UART0_RX
    MAP_PinTypeUART(PIN_57, PIN_MODE_3);

    // configuration, 8 bits length data width, 1 stop bit, no parity check
	UARTConfigSetExpClk(UART_Handle->UART_PORT,PRCMPeripheralClockGet(	UART_Handle->UART_PORTPERIPH),
			UART_Handle->UART_BRATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
             UART_CONFIG_PAR_NONE));

	// disable UART since function above contained UARTenable
	//UARTDisable(UART_Handle->UART_PORT);

	UARTIntEnable(UART_Handle->UART_PORT, UART_INT_TX | UART_INT_RX | UART_INT_RT); // enable interrupt for send and receive and receive timeout
	UARTIntRegister(UART_Handle->UART_PORT, UART_ISR);	//hook ISR
	UARTFIFOEnable(UART_Handle->UART_PORT);	//enable FIFO for send and receive
	UARTFIFOLevelSet(UART_Handle->UART_PORT, UART_FIFO_TX4_8, UART_FIFO_RX4_8);	//Interrupt occur when 7 bytes send from FIFO or read in FIFO


	return UART_Handle;
}


int32_t UART_close(){
    // Power OFF the peripheral
    MAP_PRCMPeripheralClkDisable(UART_Handle->UART_PORTPERIPH, PRCM_RUN_MODE_CLK);
    // release allocated memory space

    free(UART_Handle->pReadBuf);
    free(UART_Handle->pWriteBuf);

    free(UART_Handle);
    return SUCCESS;
}

uint32_t UART_getSendCount(){
	return UART_Handle->sentCount;
}
