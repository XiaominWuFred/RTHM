#ifndef UARTNEW_H_
#define UARTNEW_H_

#include <FreeRTOS.h>
#include <semphr.h>

#define UART_BAUDRATE  			115200
#define SYSTEMCLK          		80000000
#define PORT1         			UARTA0_BASE
#define PORT_PERIPH1 		 	PRCM_UARTA0

#define PORT2         			UARTA1_BASE
#define PORT_PERIPH2 		 	PRCM_UARTA1

#define UARTBUFFERSIZE 				144
#define FIFOMAX					16
#define FIFOEMPTY				0
#define BUFFEMPTY				0
#define FAILURE                 -1			//wrong length
#define SUCCESS                 0
#define TIMEOUT 				-111		//time out
#define WRITESUCCESS			1
#define WRITETIMEOUT			-100
#define READSUCCESS				2
#define READTIMEOUT           	-101


typedef struct {
	uint32_t UART_PORT;
	uint32_t UART_BRATE;
	uint32_t UART_PORTPERIPH;

	uint32_t WriteBufSize;
	uint32_t ReadBufSize;
	uint32_t WriteLength;
	uint32_t ReadLength;
	uint32_t ReadLengthcpy;
	uint32_t templength;
	int32_t  Cindex;
	int32_t  reValue;

	uint32_t sentCount;	//debugging

	uint8_t *pWriteBuf;
	uint8_t *pReadBuf;
	uint8_t *pCurrentWrite;
	uint8_t *pCurrentRead;
	uint8_t *dataput;
	uint8_t UART_SendComplete;

	SemaphoreHandle_t UARTWrite_Semaphore;
	SemaphoreHandle_t UARTRead_Semaphore;
		SemaphoreHandle_t UARTprotect_Semaphore;
}UART_STATE;

typedef UART_STATE* UART_HANDLE;

int32_t UART_read(UART_STATE *UART_Handle, uint8_t *pData, uint32_t length,uint32_t time_out );

int32_t UART_write(UART_STATE *UART_Handle, uint8_t *pData, uint32_t length, uint32_t time_out );

UART_HANDLE UART_open(uint32_t UART_port, uint32_t UART_portperiph, uint32_t UART_baudrate);

int32_t UART_close();

uint32_t UART_getSendCount();

#endif
