 /*  Created on: July 20, 2017
 *      Author: xwu
 */

#ifndef GPIO_CONTROL_H_
#define GPIO_CONTROL_H_

#include "timer_control.h"

#define INHALE			0x01
#define EXHALE			0x00

void RedLED_configure();

void PushButton_configure();

void LEDControlOnPaulse(uint8_t control, COUNTER_HANDLE COUNTER_Handle, uint32_t preciousPeriod);

void PushButton_close();

void RedLED_close();

#endif /* SOURCE_TIMER_CONTROL_H_ */
