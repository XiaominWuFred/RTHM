
//*****************************************************************************
//
// Application Name     - Multi-task frame for Free-RTOS
// Application Overview -
// Application Details  -
//
//
//*****************************************************************************


// Standard includes.
#include <stdio.h>
#include <stdlib.h>


#include "osi.h"


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
#include "UARTnew.h"
#include "i2cnew.h"
#include "pinmux.h"
#include "i2c_if.h"
#include "timer_control.h"
#include "gpio.h"
#include "gpio_if.h"
#include "control_task.h"
#include "application_task.h"
#include "task1.h"
#include "common.h"



//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION     "1.1.1"
#define UART_PRINT              Report
#define SPAWN_TASK_PRIORITY     9
#define OSI_STACK_SIZE          2048
#define APP_NAME                "FreeRTOS "
#define MAX_MSG_LENGTH			16

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start

//*****************************************************************************

#ifndef USE_TIRTOS
/* in case of TI-RTOS don't include startup_*.c in app project */
#if defined(gcc) || defined(ccs)
extern void (* const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif
#endif
//*****************************************************************************
//                 GLOBAL VARIABLES -- End
//*****************************************************************************


//*****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//*****************************************************************************

//void vTestTask1( void *pvParameters ); //for testing

//void control_task( void *pvParameters );
//void TASK1( void *pvParameters );
//void TASK2( void *pvParameters );
//void TASK3( void *pvParameters );
void TASK4( void *pvParameters );
//void TASK5( void *pvParameters );
void TASK6(void *pvParameters );
void BoardInit();


#ifdef USE_FREERTOS
//*****************************************************************************
// FreeRTOS User Hook Functions enabled in FreeRTOSConfig.h
//*****************************************************************************

//*****************************************************************************
//
//! \brief Application defined hook (or callback) function - assert
//!
//! \param[in]  pcFile - Pointer to the File Name
//! \param[in]  ulLine - Line Number
//!
//! \return none
//!
//*****************************************************************************
void
vAssertCalled( const char *pcFile, unsigned long ulLine )
{
    //Handle Assert here
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Application defined idle task hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void
vApplicationIdleHook( void)
{
    //Handle Idle Hook for Profiling, Power Management etc
}

//*****************************************************************************
//
//! \brief Application defined malloc failed hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationMallocFailedHook()
{
    //Handle Memory Allocation Errors
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Application defined stack overflow hook
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void vApplicationStackOverflowHook( OsiTaskHandle *pxTask,
                                   signed char *pcTaskName)
{
    //Handle FreeRTOS Stack Overflow
    while(1)
    {
    }
}
#endif //USE_FREERTOS

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
  //
  // Set vector table base
  //
#if defined(ccs) || defined(gcc)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
  //
  // Enable Processor
  //
  MAP_IntMasterEnable();
  MAP_IntEnable(FAULT_SYSTICK);

  PRCMCC3200MCUInit();
}

//*****************************************************************************
//
//!  main function handling the freertos_demo.
//!
//! \param  None
//!
//! \return none
//
//*****************************************************************************
int main( void )
{
    //
    // Initialize the board
    //
    BoardInit();

    PinMuxConfig();

//    osi_TaskCreate( vTestTask1, "TASK1",\
       							OSI_STACK_SIZE, NULL, 2, NULL ); //for testing




//    ControlTaskInitiail;
//    Task1Initial;
//    Task2Initial;
    Task4Initial;
//    Task3Initial;
    Task6Initial;


//    osi_TaskCreate( control_task, "Control", OSI_STACK_SIZE, NULL, 2, NULL );
    osi_TaskCreate( TASK6, "appTask6", OSI_STACK_SIZE,NULL, 1, NULL );

    osi_TaskCreate( TASK4, "appTask4", OSI_STACK_SIZE,NULL, 1, NULL );

//    osi_TaskCreate( TASK1, "appTask1", OSI_STACK_SIZE,NULL, 1, NULL );

//    osi_TaskCreate( TASK2, "appTask2", OSI_STACK_SIZE,NULL, 1, NULL );

//    osi_TaskCreate( TASK3, "uartTask", OSI_STACK_SIZE,NULL, 1, NULL );
  //  osi_TaskCreate( TASK5, "appTask5", OSI_STACK_SIZE,NULL, 1, NULL );



    //
    // Start the task scheduler
    //
    osi_start();

    return 0;
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
