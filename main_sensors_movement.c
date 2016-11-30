/*
 * main.c
 */

#include "lcd.h"
#include "open_interface.h"
#include "button.h"
#include "timer.h"



//1 if a hazard is detected, 0 otherwise
uint8_t hazard = 0;

//This variable calibrates the rotation of the bot.
int rotateConstant = (int)(920000/90);

//max value for the cliff sensor signal to detect white.
uint16_t max_white_color = 2850;

//min value for the cliff sensor signal to detect white.
uint16_t min_white_color = 2750;



//Moves the bot foreward
void movebot_Forward(int speed, int distance, oi_t *self);


void movebot_Backward(int speed, int distance, oi_t *self);

//Turns the bot clockwise
void rotatebot_ClockWise(int degrees, oi_t *self);

//Turns the bot counter clockwise
void rotatebot_CounterClockWise(int degrees, oi_t *self);

//sets up a song.
void songSetup(void);

//Called when Hazard occurs.
void movebot_Hazard(oi_t *self);


int main(void) {
	oi_t *sensor_data = oi_alloc();
	oi_init(sensor_data);
	lcd_init();
	button_init();
	songSetup();

	while(1)
	{

		lcd_printf("1.ccw 5.Hazard Tone\n2.cw  6.Secret Tone\n3.Foreward\n4.Backward");
		//lcd_printf("left: %d\nfleft: %d\nfright: %d\nright: %d",sensor_data->cliffLeftSignal,sensor_data->cliffFrontLeftSignal,sensor_data->cliffFrontRightSignal,sensor_data->cliffRightSignal);

		uint8_t buttonValue = button_getButton();
		while(!buttonValue)
		{
			buttonValue = button_getButton();
		}

		//Turns the bot counter clockwise
		if(buttonValue == 1)
		{
			lcd_printf("Turning 90deg ccw");
			rotatebot_CounterClockWise(90, sensor_data);
		}

		//Turns the bot clockwise
		if(buttonValue == 2)
		{
			lcd_printf("Turning 90deg cw");
			rotatebot_ClockWise(90, sensor_data);
		}

		//Moves the bot foreward
		if(buttonValue == 3)
		{
			movebot_Forward(200, 1000,sensor_data);
		}

		//Moves the bot backward
		if(buttonValue == 4)
		{
			movebot_Backward(200, 1000,sensor_data);
		}

		//Plays the hazard tone
		if(buttonValue == 5)
		{
			oi_play_song(0);
		}

		//Plays secret song
		if(buttonValue == 6)
		{
			oi_play_song(1);
		}


	}

	return 0;
}

/*
 * Moves the bot forewards.
 * 	param 1, speed: The speed of the bot (0 < speed <= 500).
 * 	param 2, distance: The distance in centimeters the bot will travel (0 < distance).
 * 	param 3, *self: sensor_data pointer (see begining of main()).
 */
void movebot_Forward(int speed, int distance, oi_t *self)
{
	lcd_printf("%s","MovingForward");
	int sum = 0;
	oi_setWheels(speed, speed); // move forward; speed
	while (sum < distance)
	{
		oi_update(self);

		if(self->bumpLeft || self->bumpRight || self->cliffLeft || self->cliffFrontLeft || self->cliffFrontRight || self->cliffRight || self->wheelDropLeft || self->wheelDropRight)
		{
			sum = distance;
			movebot_Hazard(self);
		}
		else if(self->cliffLeftSignal > min_white_color && self->cliffLeftSignal < max_white_color)
		{
			sum = distance;
			movebot_Hazard(self);
		}
		else if(self->cliffFrontLeftSignal > min_white_color && self->cliffFrontLeftSignal < max_white_color)
		{
			sum = distance;
			movebot_Hazard(self);
		}
		else if(self->cliffFrontRightSignal > min_white_color && self->cliffFrontRightSignal < max_white_color)
		{
			sum = distance;
			movebot_Hazard(self);
		}
		else if(self->cliffRightSignal > min_white_color && self->cliffRightSignal < max_white_color)
		{
			sum = distance;
			movebot_Hazard(self);
		}
		else
		{
			sum += self->distance;
		}
	}
	lcd_printf("Done with move");
	oi_setWheels(0,0);
}

/*
 * Called when Hazard occurs.
 */
void movebot_Hazard(oi_t *self)
{
	lcd_printf("HAZARD!!!!");
	oi_play_song(0);
	hazard = 1;
	oi_setWheels(0,0);
	movebot_Backward(100,300, self);
}

/*
 * Moves the bot backwards.
 * 	param 1, speed: The speed of the bot (0 < speed <= 500).
 * 	param 2, distance: The distance in centimeters the bot will travel (0 < distance).
 * 	param 3, *self: sensor_data pointer (see begining of main()).
 */
void movebot_Backward(int speed, int distance, oi_t *self)
{
	if(!hazard)
		lcd_printf("MovingBackward");
	speed = -1*speed;
	distance = -1*distance;

	int sum = 0;
	oi_setWheels(speed, speed); // move forward; speed
	while (sum > distance)
	{
		oi_update(self);
		sum += self->distance;
	}
	oi_setWheels(0,0);
	hazard = 0;
}


/*
 *rotates the bot clockwise.
 *	param 1, degrees: number of degrees to rotate by.
 *  param 2, *self: sensor_data pointer (see begining of main()).
 *
 *  **IMPORTANT** make sure that the value of rotateConstant is calibrated properly.
 */
void rotatebot_ClockWise(int degrees, oi_t *self)
{
	oi_setWheels(-200, 200); // move forward; speed
	int i = 0;
	for(i = 0; i < degrees; i++)
		timer_waitMicros(rotateConstant);
	oi_setWheels(0,0);
}

/*
 *Rotates the bot counter clockwise.
 *	param 1, degrees: number of degrees to rotate by.
 *  param 2, *self: sensor_data pointer (see begining of main()).
 *
 *  **IMPORTANT** make sure that the value of rotateConstant is calibrated properly.
 */
void rotatebot_CounterClockWise(int degrees, oi_t *self)
{
	oi_setWheels(200, -200); // move forward; speed
	int i = 0;
	for(i = 0; i < degrees; i++)
		timer_waitMicros(rotateConstant);
	oi_setWheels(0,0);
}

/*
 * Creates a custom song.
 */
void songSetup(void)
{
	//song0, hazard tone
	unsigned char note0[] = {46,46};
	unsigned char dur0[] = {8,8};
	oi_loadSong(0, 2, note0, dur0);

	//song1, secret
	unsigned char note1[] = {79, 78, 75, 69, 68, 75, 80, 84};
	unsigned char dur1[] = {8, 8, 8, 8, 8, 8, 8, 32};
	oi_loadSong(1, 8, note1, dur1);
}









