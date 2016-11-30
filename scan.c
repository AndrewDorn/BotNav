/*
 * scan.c
 *
 *  Created on: Nov 15, 2016
 *      Author: asmola
 */

#include <stdint.h>
#include "scan.h"
#include <math.h>
#include<stdio.h>
#include"lcd.h"
#include"uart.h"

#define M_PI 3.14159265358979323846
#define CMS_FROM_SENSOR_TO_SERVO 4
#define CHAOS 5
#define BIT0		0x01
#define BIT1		0x02
#define BIT2		0x04
#define BIT3		0x08
#define BIT4		0x10
#define BIT5		0x20
#define BIT6		0x40
#define BIT7		0x80

double centerPulseWidth = 1.4;
double pulseVariance = .85;
int pulse_period = 320000; //16,000,000 * .02
int chaos_zone = 10;
int position;

volatile uint32_t startTime = 0;
volatile uint32_t endTime = 0;
volatile int flag = 0;
volatile long int duration = 0;
volatile unsigned int overflows = 0;

void move_servo(int degree) {
	position = degree;
	int degreesFrom90 = position - 90;
	double pulseWidth = degreesFrom90 * pulseVariance / 90 + centerPulseWidth;

	TIMER1_TBMATCHR_R = pulse_period - (pulseWidth * 16000); //16,000 cycles per millisecond
	TIMER1_TBPMR_R = ((int) (pulse_period - (pulseWidth * 16000))) >> 16;

	//call timer_waitMillis( ) here to enforce a delay
	timer_waitMillis(50);

	lcd_printf("Degree: %d", position);
}

void center_servo() {
	double pulseWidth = centerPulseWidth;

	TIMER1_TBMATCHR_R = pulse_period - (pulseWidth * 16000); //16,000 cycles per millisecond
	TIMER1_TBPMR_R = ((int) (pulse_period - (pulseWidth * 16000))) >> 16;

	TIMER1_CTL_R |= TIMER_CTL_TBEN; //enable timer
	// you need to call timer_waitMillis( ) here to enforce a delay for the servo to
	timer_waitMillis(500);
	// move to the position
	//TIMER1_CTL_R &= (~TIMER_CTL_TBEN);	//diable timer
	//move_servo(25);
	position = 90;
	move_servo(0);
	lcd_printf("Degree: %d", position);
}

void send_pulse() {
	TIMER3_CTL_R &= ~(TIMER_CTL_TBEN);
	GPIO_PORTB_AFSEL_R &= 0b11110111; //diable alternate functions on port b pin 3
	GPIO_PORTB_PCTL_R &= ~(0xF000); //Turns off the function
	GPIO_PORTB_DIR_R |= 0x08; // set PB3 as output
	GPIO_PORTB_DATA_R |= 0x08; // set PB3 to high
	timer_waitMicros(5);
	GPIO_PORTB_DATA_R &= 0xF7; // set PB3 to low
	GPIO_PORTB_DIR_R &= 0xF7; // set PB3 as input
	GPIO_PORTB_AFSEL_R |= 0b00001000; //enable alternate functions on port b pin 3
	GPIO_PORTB_PCTL_R |= 0x7000; //Turns on the function
	TIMER3_CTL_R |= TIMER_CTL_TBEN; //Restart timer 3B
}

void TIMER3B_Handler(void) {
	TIMER3_ICR_R = TIMER_ICR_CBECINT; //Clear /flag
	uint32_t time = TIMER3_TBR_R;

	if (flag == 0) {
		startTime = TIMER3_TBR_R; //time;
		flag = 1;
	} else if (flag == 1) {
		endTime = TIMER3_TBR_R; //time;
		duration = endTime - startTime; //time - startTime;
		if (startTime > endTime) {
			duration += 65535;
			overflows++;
		}
		flag = 2;
	} else if (flag == 2)
		return;
	//TIMER3_ICR_R = TIMER_ICR_TBTOCINT; //clear flag
}

void init_sonar_timer() {
	//enable clock to GPIO, R1 = port B
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1;

	//set pin 3 to digital
	GPIO_PORTB_DEN_R |= BIT3;

	//Turn on clock to timer3
	SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R3;

	//Disable timer 3B (clear bit TnEN in GPTMCTL)
	TIMER3_CTL_R &= (~TIMER_CTL_TBEN);

	//Set as 16-bit timer
	TIMER3_CFG_R = TIMER_CFG_16_BIT;

	TIMER3_TBPR_R = 0;

	//Edge-Time Mode, capture mode, count up
	TIMER3_TBMR_R |=
			(TIMER_TBMR_TBCDIR | TIMER_TBMR_TBCMR | TIMER_TBMR_TBMR_CAP); //pg 729

	//Set for both edges
	TIMER3_CTL_R |= 0b0000110000000000;

	//Set Maximum
	//TIMER3_TBILR_R = 0xFFFF;

	TIMER3_ICR_R = TIMER_ICR_CBECINT; //clear flag

	//Enable Interrupt
	TIMER3_IMR_R |= (TIMER_IMR_CBEIM);

	NVIC_EN1_R = BIT4;	//Enable interrupt for Timer 3B

	IntRegister(INT_TIMER3B, TIMER3B_Handler);	//Attach handler

	IntMasterEnable();	//Enable all interrupts

	//Enable Timer3B
	//TIMER3_CTL_R |= TIMER_CTL_TBEN;
}

void ADC0_init(void) {
	//enable ADC 0 module on port D
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R3;
	//enable clock for ADC
	SYSCTL_RCGCADC_R |= 0x1;
	//enable port D pin 0 to work as alternate functions
	GPIO_PORTD_AFSEL_R |= 0x01;
	//set pin to input - 0
	GPIO_PORTD_DEN_R &= 0b11111110;
	//disable analog isolation for the pin
	GPIO_PORTD_AMSEL_R |= 0x01;
	//initialize the port trigger source as processor (default)
	GPIO_PORTD_ADCCTL_R = 0x00;
	//disable SS0 sample sequencer to configure it
	ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN0;
	//initialize the ADC trigger source as processor (default)
	ADC0_EMUX_R = ADC_EMUX_EM0_PROCESSOR;
	//set 1st sample to use the AIN10 ADC pin
	ADC0_SSMUX0_R |= 0x000A;
	//enable raw interrupt
	ADC0_SSCTL0_R |= (ADC_SSCTL0_IE0 | ADC_SSCTL0_END0);
	//enable oversampling to average
	ADC0_SAC_R |= ADC_SAC_AVG_64X;
	//re-enable ADC0 SS0
	ADC0_ACTSS_R |= ADC_ACTSS_ASEN0;
}

void scanner_init() { //1B
	//***set GPIO PB5, turn on clk, alt. function, output, enable***

	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; //enable clock to GPIO, R1 = port B
	GPIO_PORTB_AFSEL_R |= BIT5; //enable alternate functions on port b pin 5
	GPIO_PORTB_DEN_R |= BIT5; //set to digital
	GPIO_PORTB_DIR_R |= BIT5; //set for output

	GPIO_PORTB_PCTL_R |= 0x00700000;

	SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1; //turn on clk for timer1
	TIMER1_CTL_R &= (~TIMER_CTL_TBEN); //disable timer to config
	TIMER1_TBMR_R &= ~(BIT2); //Clear TBCMR bit
	TIMER1_TBMR_R |= BIT1; //TBMR set to periodic
	TIMER1_TBMR_R |= BIT3; //TBAMS enable to pwm

	TIMER1_CFG_R = TIMER_CFG_16_BIT; //set size of timer to 16
	TIMER1_TBILR_R = pulse_period & 0xFFFF; //lower 16 bits of the interval
	TIMER1_TBPR_R = pulse_period >> 16; //set the upper 8 bits of the interval

	center_servo();
	init_sonar_timer();
	ADC0_init();
}

uint32_t GetReading(char channel) {
	//disable ADC0SS0 sample sequencer to configure it
	ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN0;
	//set 1st sample to use the channel ADC pin
	ADC0_SSMUX0_R |= channel;
	//re-enable ADC0 SS0
	ADC0_ACTSS_R |= ADC_ACTSS_ASEN0;
	//initiate SS0 conversion
	ADC0_PSSI_R = ADC_PSSI_SS0;
	//wait for ADC conversion to be complete
	while ((ADC0_RIS_R & ADC_RIS_INR0) == 0) {
	}
	//clear interrupt
	ADC0_ISC_R = ADC_ISC_IN0;
	return ADC0_SSFIFO0_R;
}

struct distance_info * perform_scan() {
	static struct distance_info distances[180];
	while (1) {
		send_pulse();
		uint32_t digVal = 0;	//Get value from register

		int j;
		for (j = 0; j < 3; j++) {
			timer_waitMillis(10);
			digVal += GetReading('b');
		}
		digVal /= j;

		int IRDistance = (pow(digVal, -1.41) * 639685) + CMS_FROM_SENSOR_TO_SERVO;//Equation for converting digital value back to analog for bot 10
		while (flag != 2)
			;	//Check to see if the ping is complete
		double pingDistance = ((((double) duration) / 16000000.0) * 34000.0
				/ 2.0) + CMS_FROM_SENSOR_TO_SERVO;
		flag = 0;

		int arrayPos = position;

		distances[arrayPos].sonar_cm = (int) (pingDistance + .5);
		distances[arrayPos].IR_cm = IRDistance;

		if (position < 180)
			move_servo(position + 1);
		else
			break;
	}

	move_servo(0);
	return distances;
}

void check_for_objects(struct distance_info * distances) {
	uart_sendString("Degree\tDistance\tSize\n\r");
	int inObject = 0;
	int startPosition;
	int i;
	for(i = 0;i < 180;i++)
	{
		if(inObject == 1)
		{
			if(abs(distances[i].IR_cm - distances[i - 1].IR_cm) > CHAOS)//not in same object
			{
				double objectRadians = (i - startPosition) * M_PI / 180.0;
				double cosine = cos(objectRadians);
				double size = pow(pow(distances[startPosition].sonar_cm, 2) + pow(distances[i - 1].sonar_cm, 2) - 2 * distances[startPosition].sonar_cm * distances[i - 1].sonar_cm * cosine, .5);
				if(size > 1)
				{
					char message[100];
					sprintf(message, "%d\t%fcm.\t%fcm.\n\r", (i + startPosition - 1) / 2, distances[(i + startPosition - 1) / 2].sonar_cm + (size / 2), size);
					uart_sendString(message);
				}
				inObject = 0;
				i--;
			}
		}
		else if(distances[i].IR_cm < 80)//In new object
		{
			startPosition = i;
			inObject = 1;
		}
	}

	if(inObject == 1)
	{
		i = 179;
		double objectRadians = (i - startPosition) * M_PI / 180.0;
		double cosine = cos(objectRadians);
		double size = pow(pow(distances[startPosition].sonar_cm, 2) + pow(distances[i - 1].sonar_cm, 2) - 2 * distances[startPosition].sonar_cm * distances[i - 1].sonar_cm * cosine, .5);
		char message[100];
		sprintf(message, "%d\t%fcm.\t%fcm.\n\r", (i + startPosition - 1) / 2, distances[(i + startPosition - 1) / 2].sonar_cm + (size / 2), size);
		uart_sendString(message);
	}
}
