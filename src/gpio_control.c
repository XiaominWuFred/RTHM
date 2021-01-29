#include <stdbool.h>
#include "string.h"
#include "hw_types.h"
#include "pin.h"
#include "gpio.h"
#include "gpio_if.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "gpio_control.h"
#include "hw_memmap.h"

//#define DEBUG

static BaseType_t xHigherPriorityTaskWoken;	//ISR give semaphore needed bits
extern SemaphoreHandle_t WIFIAP_Semaphore;	//semaphore to control AP configuration
extern bool Configuring;	//bits to block wrong ISR entering

/*
 * PushButtonISR
 * entered when SW2 push button on the launch pad pressed
 * interrupt triggered by rising edge
 * ISR clear the raw interrupt
 * and display a LED for clicking feedback
 * give the semaphore, which is used to block AP configuration in
 * the AP_configuration task
 */
void PushButtonISR(void){
static int coun = 0;	//variable for LED control
	GPIOIntClear(GPIOA2_BASE, GPIO_INT_PIN_6);	//clear GPIO raw interrupt
	//LED control
	if(!Configuring){
		if(coun == 1){
		GPIO_IF_LedOn(9);
		coun = 0;
		}
		else{
		GPIO_IF_LedOff(9);
		coun = 1;
		}
		//give semaphore
		xSemaphoreGiveFromISR( WIFIAP_Semaphore, &xHigherPriorityTaskWoken );// release semaphore
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );//forcing context exchange
	}
}

/*
 * RedLED_configure
 * configure the red LED on the lauch pad
 */
void RedLED_configure(){
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);

    // Configure PIN_64 for GPIOOutput
	MAP_PinTypeGPIO(PIN_64, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIOA1_BASE, 0x2, GPIO_DIR_MODE_OUT);

	GPIO_IF_LedConfigure(LED1);
	GPIO_IF_LedOff(9);
}

/*
 * PushButton_configure
 * configure the SW2 push button on the lauch pad
 * to generate interrput
 * and link the ISR with it
 */
void PushButton_configure(){
	//strat peripheral
	MAP_PRCMPeripheralClkEnable(PRCM_GPIOA2, PRCM_RUN_MODE_CLK);

	// Configure PIN_15 for GPIOOutput
	MAP_PinTypeGPIO(PIN_15, PIN_MODE_0, false);
	MAP_GPIODirModeSet(GPIOA2_BASE, GPIO_PIN_6, GPIO_DIR_MODE_IN);

	//register ISR
	GPIOIntRegister(GPIOA2_BASE, PushButtonISR);
	//set interrupt type GPIO_HIGH_LEVEL
	GPIOIntTypeSet(GPIOA2_BASE, GPIO_INT_PIN_6, GPIO_RISING_EDGE);
	//enable interrupt, GPIO22
	GPIOIntEnable(GPIOA2_BASE, GPIO_INT_PIN_6);
}

/*
 * PushButton_close
 * close the SW2 push button
 */
void PushButton_close(){
	//stop peripheral
	PRCMPeripheralClkDisable(PRCM_GPIOA2,PRCM_RUN_MODE_CLK);
	//clear interrupt
	GPIOIntDisable(GPIOA2_BASE, GPIO_INT_PIN_6);
}

/*
 * LEDControlOnPaulse
 * used by the pulse pattern showing
 * logically decide the LED on or off when pulse detected
 */
void LEDControlOnPaulse(uint8_t control, COUNTER_HANDLE COUNTER_Handle, uint32_t preciousPeriod){
	uint32_t temp = counter_getValue(COUNTER_Handle);
	if(control && INHALE){
		GPIO_IF_LedOn(9);
		if(temp > preciousPeriod){
			GPIO_IF_LedOff(9);
		}
	}else{
		GPIO_IF_LedOff(9);
	}
}

/*
 * RedLED_close()
 * close the Red LED on lauch pad
 */
void RedLED_close(){
	GPIO_IF_LedOff(9);
	PRCMPeripheralClkDisable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);
}
