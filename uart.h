/*
 * uart.h
 *
 *  Created on: Nov 15, 2016
 *      Author: asmola
 */

#ifndef UART_H_
#define UART_H_

#include <inc/tm4c123gh6pm.h>

void uart_init();
void uart_sendString();
char* uart_getCommand(void);

#endif /* UART_H_ */
