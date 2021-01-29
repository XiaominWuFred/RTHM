/*
 * task1.c
 *
 *  Created on: Jun 13, 2017
 *      Author: xwu
 */
#include "string.h"
#include "hw_types.h"
#include "pin.h"
#include "gpio.h"
#include "gpio_if.h"
#include "task1.h"
#include "i2cnew.h"
#include "UARTnew.h"
#include "timer_control.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "osi.h"

void vTestTask1( void *pvParameters )
{

	UART_STATE* UART_Handle;
	UART_Handle = UART_open(PORT1, PORT_PERIPH1, UART_BAUDRATE);

	I2C_STATE* I2C_Handle;
	I2C_Handle = I2C_open(I2C0_PRCM, I2C0_BASE,1);//I2C_MASTER_MODE_FST
	//I2C_IF_Open(I2C_MASTER_MODE_FST);


    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);

    // Configure PIN_64 for GPIOOutput
	MAP_PinTypeGPIO(PIN_64, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIOA1_BASE, 0x2, GPIO_DIR_MODE_OUT);

	GPIO_IF_LedConfigure(LED1);
	GPIO_IF_LedOff(9);


	TIMER_HANDLE timer_handle;
	timer_handle = timer_configure(TIMER0BASE, TIMER0PRCM, 5, blocking_timer_ISR); // 5ms
	timer_handle = timer_configure(TIMER1BASE, TIMER1PRCM, 200, counting_timer_ISR);
	setT2CMAX(3000); // 10 min

	uint8_t buffer[144];

				  uint8_t* bufferpointer = buffer;
	unsigned long count = 0;

	int32_t buffercount = 0; //reset buffer count

	uint8_t I2Creadbuff[6]; // acceleration data xyz
	uint8_t write[1];

	uint8_t Displaybuf[25]; // acceleration UART display data xyz each has 3 bytes
	uint16_t positiondatabuff[3];
	write[0] = 0x11;
	uint8_t writefirst[1];
	writefirst[0] = 0x01;
	I2C_Write(I2C_Handle, 0x1d, 0x2A, write, 1, 0x500);


	timerS_start();

    for( ;; )
    {

    	if(timer_handle->TimerOn == 1){
    	timer_hold(30);


    //	memset(I2Creadbuff,0,sizeof(I2Creadbuff)); // read buffer clear

    	I2C_read(I2C_Handle, 0x1d, 0x01, bufferpointer, 6, 0x2000);
    //	I2C_IF_ReadFrom(0x1d, writefirst, 0x01, bufferpointer, 6);

    	bufferpointer = bufferpointer+6;
    	buffercount = buffercount + 6;

    	if(buffercount == 144){
    					//buffer full

    					//***********task forward data to other task*************
    					UART_write( UART_Handle, buffer, sizeof(buffer), 0xff );
    					memset(buffer,0,144*sizeof(uint8_t)); // read buffer clear  bufferpointercpy
    					bufferpointer = buffer;
    					buffercount = 0; //reset buffer count
    				}



    	/*
    	//x data converting limitation: -2g - 2g
  					positiondatabuff[0] = (I2Creadbuff[0] << 4) + (I2Creadbuff[1] >> 4); // x

    					Displaybuf[0] = 0x20;
    					Displaybuf[1] = 0x20;

    					if(positiondatabuff[0] <= 2047){
    					//positive
    						Displaybuf[2] = 0x2b;
    						Displaybuf[3] = positiondatabuff[0] / 1000 + 48;
    						Displaybuf[4] = 0x2e;
    						Displaybuf[5] = (positiondatabuff[0] % 1000) / 100 + 48;
    						Displaybuf[6] = ((positiondatabuff[0] % 1000) % 100) / 10 + 48;
    						Displaybuf[7] = ((positiondatabuff[0] % 1000) % 100) % 10 + 48;
    						Displaybuf[8] = 0x20;
    					}else{
    					//negative
    						positiondatabuff[0] = positiondatabuff[0] &0x7ff;
    						positiondatabuff[0] = 2048 - positiondatabuff[0];
    						Displaybuf[2] = 0x2d;
    						Displaybuf[3] = positiondatabuff[0] / 1000 + 48;
    						Displaybuf[4] = 0x2e;
    						Displaybuf[5] = (positiondatabuff[0] % 1000) / 100 + 48;
    				        Displaybuf[6] = ((positiondatabuff[0] % 1000) % 100) / 10 + 48;
    						Displaybuf[7] = ((positiondatabuff[0] % 1000) % 100) % 10 + 48;
    						Displaybuf[8] = 0x20;
    					}

    					//y data converting limitation: -2g - 2g
    						positiondatabuff[1] = (I2Creadbuff[2] << 4) + (I2Creadbuff[3] >> 4); // x

    						if(positiondatabuff[1] <= 2047){
    					    //positive
    							Displaybuf[9] = 0x2b;
    							Displaybuf[10] = positiondatabuff[1] / 1000 + 48;
    							Displaybuf[11] = 0x2e;
    							Displaybuf[12] = (positiondatabuff[1] % 1000) / 100 + 48;
    							Displaybuf[13] = ((positiondatabuff[1] % 1000) % 100) / 10 + 48;
    							Displaybuf[14] = ((positiondatabuff[1] % 1000) % 100) % 10 + 48;
    							Displaybuf[15] = 0x20;
    						}else{
    						//negative
    							positiondatabuff[1] = positiondatabuff[1] &0x7ff;
    							positiondatabuff[1] = 2048 - positiondatabuff[1];
    							Displaybuf[9] = 0x2d;
    							Displaybuf[10] = positiondatabuff[1] / 1000 + 48;
    							Displaybuf[11] = 0x2e;
    							Displaybuf[12] = (positiondatabuff[1] % 1000) / 100 + 48;
    							Displaybuf[13] = ((positiondatabuff[1] % 1000) % 100) / 10 + 48;
    							Displaybuf[14] = ((positiondatabuff[1] % 1000) % 100) % 10 + 48;
    							Displaybuf[15] = 0x20;
    						}

    					//z data converting limitation: -2g - 2g
    						positiondatabuff[2] = (I2Creadbuff[4] << 4) + (I2Creadbuff[5] >> 4); // x

    						if(positiondatabuff[2] <= 2047){
    						//positive
    							Displaybuf[16] = 0x2b;
    							Displaybuf[17] = positiondatabuff[2] / 1000 + 48;
    							Displaybuf[18] = 0x2e;
    							Displaybuf[19] = (positiondatabuff[2] % 1000) / 100 + 48;
    							Displaybuf[20] = ((positiondatabuff[2] % 1000) % 100) / 10 + 48;
    							Displaybuf[21] = ((positiondatabuff[2] % 1000) % 100) % 10 + 48;
    							Displaybuf[22] = 0x20;
    						}else{
    						//negative
    							positiondatabuff[2] = positiondatabuff[2] &0x7ff;
    							positiondatabuff[2] = 2048 - positiondatabuff[2];
    							Displaybuf[16] = 0x2d;
    							Displaybuf[17] = positiondatabuff[2] / 1000 + 48;
    							Displaybuf[18] = 0x2e;
    							Displaybuf[19] = (positiondatabuff[2] % 1000) / 100 + 48;
    							Displaybuf[20] = ((positiondatabuff[2] % 1000) % 100) / 10 + 48;
    							Displaybuf[21] = ((positiondatabuff[2] % 1000) % 100) % 10 + 48;
    							Displaybuf[22] = 0x20;
    						}

    						Displaybuf[23] = 13;
    					    Displaybuf[24] = 10;

*/
    //	UART_write(UART_Handle, I2Creadbuff, sizeof(I2Creadbuff) , 0x100 );
    	}else{
    		osi_Sleep(5);
    	}
    }
}
