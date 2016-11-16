#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"
#include "timer.h"
#include <stdbool.h>
#include "driverlib/interrupt.h"
#include "button.h"
#include <string.h>
#include "scan.h"
#include "uart.h"

int main(void) {
	lcd_init();
	uart_init();
	char* command = uart_getCommand();
	uart_sendString("Can't do that yet. Sorry.");
	button_init();
	scanner_init();
	struct distance_info *distances = perform_scan();
	int i = distances[0].sonar_cm;
	int j = distances[2].sonar_cm;
	lcd_printf("done");
}

