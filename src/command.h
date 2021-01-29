/*
 * command.h
 *
 *  Created on: Jun 20, 2017
 *      Author: xwu
 */

#ifndef SOURCE_COMMAND_H_
#define SOURCE_COMMAND_H_

//	command format: XXXX | XXXX | XXXX | XXXX   TASKID | operation
#define READ				 0x0010
#define PROCESSING			 0x0020
#define READANDPROCESSING    0x0030
#define UARTPRINT			 0x6300
#define READ1UART			 0x6201 //b1
#define STOP				 0x00ff

#endif /* SOURCE_COMMAND_H_ */
