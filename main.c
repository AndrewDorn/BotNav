#include <stdint.h>
#include <stdlib.h>
#include "lcd.h"
#include "timer.h"
#include <stdbool.h>
#include "driverlib/interrupt.h"
#include "button.h"
#include <string.h>

int main(void) {
	lcd_init();
	button_init();

}

