/*
 * calculation_task.c
 *
 *  Created on: Nov 10, 2017
 *      Author: xwu
 */




#include "string.h"
// Standard includes.
#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "osi.h"
#include "hw_types.h"
// Driverlib includes
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "prcm.h"



#include "timer_control.h"

// Common interface includes



// signl processing




void TASK5(){
	int s;
	int points;
	unsigned long store[20];
	for(s = 0; s < 20; s++){

	points = 100 + s*100;

	int i;
	COUNTER_HANDLE counter_handle;
	float* x;

	x = malloc(points * sizeof(float));


	for( i = 0 ; i < points; i++){
		*(x+i) = -2 + ((float)4.0/points)*(i+1);
	}

	counter_handle = counter_configure(TIMER2BASE, TIMER2PRCM);

	counter_start(counter_handle);

	for( i = 0 ; i < points ; i++){
		exp((*(x+i)*(*(x+i)))+(*(x+i)));
	}

	store[s] = counter_getValue(counter_handle);

	counter_close(TIMER2BASE, TIMER2PRCM, counter_handle);
	free(x);
	osi_Sleep(100);
	}

	osi_Sleep(100);
}


