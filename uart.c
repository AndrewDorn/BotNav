/*
 * uart.c
 *
 *  Created on: Nov 15, 2016
 *      Author: asmola
 */

#include "uart.h"
#include <stdint.h>

#define BIT0		0x01
#define BIT1		0x02
#define BIT2		0x04
#define BIT3		0x08
#define BIT4		0x10
#define BIT5		0x20
#define BIT6		0x40
#define BIT7		0x80

void uart_init(void) {
	//enable clock to GPIO, R1 = port B
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;
	//enable clock to UART1, R1 = UART1. ***Must be done before setting Rx and Tx
	SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R1;
	//enable alternate functions on port b pins 0 and 1
	GPIO_PORTB_AFSEL_R |= (BIT0 | BIT1);
	//enable Rx and Tx on port B on pins 0 and 1
	GPIO_PORTB_PCTL_R |= 0x00000011;
	//set pin 0 and 1 to digital
	GPIO_PORTB_DEN_R |= (BIT0 | BIT1);
	//set pin 0 to Rx or input
	GPIO_PORTB_DIR_R &= ~BIT0;
	//set pin 1 to Tx or output
	GPIO_PORTB_DIR_R |= BIT1;
	//calculate baudrate
	uint16_t iBRD = 8;
	uint16_t fBRD = 44;
	//turn off uart1 while we set it up
	UART1_CTL_R &= ~(UART_CTL_UARTEN);
	//set baud rate
	UART1_IBRD_R = iBRD;
	UART1_FBRD_R = fBRD;
	//set frame, 8 data bits, 1 stop bit, no parity, no FIFO
	UART1_LCRH_R = UART_LCRH_WLEN_8;
	//use system clock as source
	UART1_CC_R = UART_CC_CS_SYSCLK;
	//re-enable enable RX, TX, and uart1
	UART1_CTL_R = (UART_CTL_RXE | UART_CTL_TXE | UART_CTL_UARTEN);

	WiFi_start();
} //END of uart_init()

//blocking call that sends 1 char over uart1
void uart_sendChar(char data) {
	//check to see if room to send
	while ((UART1_FR_R & 0x20) != 0)
		;
	//send
	UART1_DR_R = data;
}

void uart_sendString(char* data) {
	int i;
	for (i = 0; i < strlen(data); i++) {
		uart_sendChar(data[i]);
	}
}

//non-blocking call to receive over uart1/
//returns char with data
char uart_receive(void) {
	//wait to receive
	while ((UART1_FR_R & UART_FR_RXFE));
	//mask the 4 error bits and grab only 8 data bits
	char data = (char) (UART1_DR_R & 0xFF);
	return data;
}

char* uart_getCommand(void)
{
	char message[256];
	int i = 0;

	do{
		message[i] = uart_receive();
		i++;
	}while(message[i - 1] != 13);

	message[i - 1] = '\0';

	return message;
}

void WiFi_start() {
	SYSCTL_RCGCGPIO_R |= BIT1;
	GPIO_PORTB_DEN_R |= 0b00000100; //set bit 2 to 1
	GPIO_PORTB_DIR_R |= 0b00000100; // Set PB2 to output
	GPIO_PORTB_DATA_R |= 0b00000100; //Enter command mode
	uart_sendChar(0x00); //Send command
	uart_sendString("thisisapassword12345"); //Send WiFi PSK
	uart_sendChar('\0'); //NULL terminator
	char response = uart_receive(); //Wait for response
	GPIO_PORTB_DATA_R &= 0b11111011;
	if (response != 0) { //An error occurred…
	}
}
