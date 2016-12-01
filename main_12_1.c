/*
 * main.c
 */

#include "lcd.h"
#include "open_interface.h"
#include "button.h"
#include "timer.h"
#include "Wifi.h"
#include "uart.h"
#include "scan.h"
#include <stdio.h>
#include <stdlib.h>

/*////////////////////////////COMMANDS////////////////////////////////
 *
 * Name:			 Command:		Desciption:
 *-----------------------------------------------------------
 * MoveForeward		 mf				Moves the bot foreward a specified distance.
 * MoveBackward 	 mb				Moves the bot backward a specified distance.
 * ClockWise 		 cw				Rotates the bot clockwise at a specified angle.
 * CounterClockWise	 ccw			Rotates the bot counterclockwise at a specified angle.
 * PlaySong 		 ps				Plays a specified song.
 */
char foreward_cmd[] = { "mf" };
char backward_cmd[] = { "mb" };
char clockwise_cmd[] = { "cw" };
char counterclockwise_cmd[] = { "ccw" };
char scan_cmd[] = { "s" };
char getIRData_cmd[] = {"d"};
char getCliffSignal_cmd[] = {"gcs"};
char playsong_cmd[] = { "ps" };

//IR Data
struct distance_info *distances;

//1 if a hazard is detected, 0 otherwise
uint8_t hazard = 0;

//1 if we are done with program.
uint8_t botDone = 0;

//1 when command is being executed.
uint8_t pending = 0;

//This variable calibrates the rotation of the bot.
int rotateConstant = (int) (920000 / 90);

//max value for the cliff sensor signal to detect white.
uint16_t max_white_color = 2850;

//min value for the cliff sensor signal to detect white.
uint16_t min_white_color = 2750;

//max value for the cliff sensor signal to detect red.
uint16_t max_black_color = 1600;

//min value for the cliff sensor signal to detect red.
uint16_t min_black_color = 0;

//Speed of the robot.
int botSpeed = 200;

uint8_t task = 3;

//list of commands
void commandList(char *string, oi_t *sensor_data);

//compares two strings, returns 1 if equal, 0 otherwise.
uint8_t compare_strings(char* inputCommand, char* programCommand);

//Moves the bot foreward
int movebot_Forward(int speed, int distance, oi_t *self);

void movebot_Backward(int speed, int distance, oi_t *self);

//Turns the bot clockwise
void rotatebot_ClockWise(int degrees, oi_t *self);

//Turns the bot counter clockwise
void rotatebot_CounterClockWise(int degrees, oi_t *self);

//sets up a song.
void songSetup(void);

//Called when Hazard occurs.
void movebot_Hazard(oi_t *self);

//Prints commands.
void printBotCommands(void);

void main_1(oi_t *sensor_data);
void main_2(oi_t *sensor_data);
void main_3(oi_t *sensor_data);

int main(void) {
	lcd_init();
	lcd_printf("Starting...");
	oi_t *sensor_data = oi_alloc();
	oi_init(sensor_data);
	button_init();
	ADC0_init();
	scanner_init();
	init_sonar_timer();
	songSetup();
	uart_init();
	WiFi_start("password");
	uart_sendString("START!!!!!!!!\n\r");
	uart_sendString("\n\r");

	lcd_printf("Buttons:\n1 main_1\n2 main_2");

	while (!task) {
		task = button_getButton();
	}
	while (button_getButton())
		;

	if (task == 1)
		main_1(sensor_data);
	if (task == 2)
		main_2(sensor_data);
	if (task == 3)
		main_3(sensor_data);

	uart_sendString("EXITING!!!!!!!!\n\r");
	return 0;
}

/*
 * Moves the bot forewards.
 * 	param 1, speed: The speed of the bot (0 < speed <= 500).
 * 	param 2, distance: The distance in centimeters the bot will travel (0 < distance).
 * 	param 3, *self: sensor_data pointer (see begining of main()).
 */
int movebot_Forward(int speed, int distance, oi_t *self) {
	lcd_printf("%s", "MovingForward");
	int sum = 0;
	oi_setWheels(speed, speed); // move forward; speed
	while (sum < distance) {
		oi_update(self);

		if (self->wheelDropLeft || self->wheelDropRight) {
			uart_sendString("Wheels Dropped!!!!\n\r");
			movebot_Hazard(self);
			sum -= 50;
			return sum;
		} else if (self->cliffLeft) {
			lcd_printf("Cliff to Left!!!!");
			uart_sendString("Cliff to Left!!!!\n\r");
			sum = distance;
			movebot_Hazard(self);
			sum -= 50;
			return sum;
		} else if (self->cliffFrontLeft) {
			lcd_printf("Cliff to Front Left!!!!");
			uart_sendString("Cliff to front Left!!!!\n\r");
			sum = distance;
			movebot_Hazard(self);
			sum -= 50;
			return sum;
		} else if (self->cliffFrontRight) {
			lcd_printf("Cliff to Front Right!!!!");
			uart_sendString("Cliff to front right!!!!\n\r");
			sum = distance;
			movebot_Hazard(self);
			sum -= 50;
			return sum;
		} else if (self->cliffRight) {
			lcd_printf("Cliff to Right!!!!");
			uart_sendString("Cliff to right!!!!\n\r");
			sum = distance;
			movebot_Hazard(self);
			sum -= 50;
			return sum;
		} else if (self->bumpLeft) {
			lcd_printf("Object Hit to Left!!!!");
			uart_sendString("Object Hit to Left!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		} else if (self->bumpRight) {
			lcd_printf("Object Hit to Right!!!!");
			uart_sendString("Object Hit to Right!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		}
		else if (self->cliffLeftSignal < max_black_color) {
			lcd_printf("End Goal left!!!!");
			uart_sendString("End Goal left!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		} else if (self->cliffFrontLeftSignal < max_black_color) {
			lcd_printf("End Goal front left!!!!");
			uart_sendString("End Goal front left!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		} else if (self->cliffFrontRightSignal < max_black_color) {
			lcd_printf("End Goal front right!!!!");
			uart_sendString("End Goal front right!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		} else if (self->cliffRightSignal < max_black_color) {
			lcd_printf("End Goal right!!!!");
			uart_sendString("End Goal right!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		}
		else if (self->cliffLeftSignal > min_white_color
				&& self->cliffLeftSignal < max_white_color) {
			lcd_printf("Border left!!!!");
			uart_sendString("Border left!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		} else if (self->cliffFrontLeftSignal > min_white_color
				&& self->cliffFrontLeftSignal < max_white_color) {
			lcd_printf("Border front left!!!!");
			uart_sendString("Border front left!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		} else if (self->cliffFrontRightSignal > min_white_color
				&& self->cliffFrontRightSignal < max_white_color) {
			lcd_printf("Border front right!!!!");
			uart_sendString("Border front right!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		} else if (self->cliffRightSignal > min_white_color
				&& self->cliffRightSignal < max_white_color) {
			lcd_printf("Border right!!!!");
			uart_sendString("Border right!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			return sum;
		}  else {
			sum += self->distance;
		}
	}
	lcd_printf("Done with move");
	oi_setWheels(0, 0);
	return sum;
}

/*
 * Called when Hazard occurs.
 */
void movebot_Hazard(oi_t *self) {
	//lcd_printf("HAZARD!!!!");
	oi_play_song(0);
	//hazard = 1;
	oi_setWheels(0, 0);
	movebot_Backward(100, 50, self);
}

/*
 * Moves the bot backwards.
 * 	param 1, speed: The speed of the bot (0 < speed <= 500).
 * 	param 2, distance: The distance in centimeters the bot will travel (0 < distance).
 * 	param 3, *self: sensor_data pointer (see begining of main()).
 */
void movebot_Backward(int speed, int distance, oi_t *self) {
	if (!hazard)
		lcd_printf("MovingBackward");
	speed = -1 * speed;
	distance = -1 * distance;

	int sum = 0;
	oi_setWheels(speed, speed); // move forward; speed
	while (sum > distance) {
		oi_update(self);
		sum += self->distance;
	}
	oi_setWheels(0, 0);
	hazard = 0;
}

/*
 *rotates the bot clockwise.
 *	param 1, degrees: number of degrees to rotate by.
 *  param 2, *self: sensor_data pointer (see begining of main()).
 *
 *  **IMPORTANT** make sure that the value of rotateConstant is calibrated properly.
 */
void rotatebot_ClockWise(int degrees, oi_t *self) {
	oi_setWheels(-200, 200); // move forward; speed
	int i = 0;
	for (i = 0; i < degrees; i++)
		timer_waitMicros(rotateConstant);
	oi_setWheels(0, 0);
}

/*
 *Rotates the bot counter clockwise.
 *	param 1, degrees: number of degrees to rotate by.
 *  param 2, *self: sensor_data pointer (see begining of main()).
 *
 *  **IMPORTANT** make sure that the value of rotateConstant is calibrated properly.
 */
void rotatebot_CounterClockWise(int degrees, oi_t *self) {
	oi_setWheels(200, -200); // move forward; speed
	int i = 0;
	for (i = 0; i < degrees; i++)
		timer_waitMicros(rotateConstant);
	oi_setWheels(0, 0);
}

/*
 * Creates a custom song.
 */
void songSetup() {
	//song0, hazard tone
	unsigned char note0[] = { 46, 46 };
	unsigned char dur0[] = { 8, 8 };
	oi_loadSong(0, 2, note0, dur0);

	//song1, secret
	unsigned char note1[] = { 79, 78, 75, 69, 68, 75, 80, 84 };
	unsigned char dur1[] = { 8, 8, 8, 8, 8, 8, 8, 32 };
	oi_loadSong(1, 8, note1, dur1);
}

void printBotCommands() {
	uart_sendString("Bot Commands:\n\r");
	uart_sendString("   0- Print Commands\n\r");
	uart_sendString("   1- Move\n\r");
	uart_sendString("   2- Rotate\n\r");
	uart_sendString("   3- Play Song\n\r");
	uart_sendString("   4- Done\n\r");
	uart_sendString("\n\r");
}

uint8_t compare_strings(char* inputCommand, char* programCommand) {
	uint8_t compare = 1;
	uint8_t nullFound = 0;
	int index = 0;

	while (compare && index < 255 && !nullFound) {
		if (inputCommand[index] != programCommand[index]) {
			compare = 0;
		}
		if (inputCommand[index] == '\0') {
			nullFound = 1;
		}
		index++;
	}
	return compare;
}

void main_1(oi_t *sensor_data) {
	while (1) {
		lcd_printf(
				"1.ccw 5.Hazard Tone\n2.cw  6.Secret Tone\n3.Foreward\n4.Backward");
		//lcd_printf("left: %d\nfleft: %d\nfright: %d\nright: %d",sensor_data->cliffLeftSignal,sensor_data->cliffFrontLeftSignal,sensor_data->cliffFrontRightSignal,sensor_data->cliffRightSignal);

		uint8_t buttonValue = button_getButton();
		while (!buttonValue) {
			buttonValue = button_getButton();
		}

		//Turns the bot counter clockwise
		if (buttonValue == 1) {
			lcd_printf("Turning 90deg ccw");
			rotatebot_CounterClockWise(90, sensor_data);
		}

		//Turns the bot clockwise
		if (buttonValue == 2) {
			lcd_printf("Turning 90deg cw");
			rotatebot_ClockWise(90, sensor_data);
		}

		//Moves the bot foreward
		if (buttonValue == 3) {
			movebot_Forward(200, 1000, sensor_data);
		}

		//Moves the bot backward
		if (buttonValue == 4) {
			movebot_Backward(200, 1000, sensor_data);
		}

		//Plays the hazard tone
		if (buttonValue == 5) {
			oi_play_song(0);
		}

		//Plays secret song
		if (buttonValue == 6) {
			oi_play_song(1);
		}
	}
}
void main_2(oi_t *sensor_data) {
	lcd_printf("left: %d\nfleft: %d\nfright: %d\nright: %d",
			sensor_data->cliffLeftSignal, sensor_data->cliffFrontLeftSignal,
			sensor_data->cliffFrontRightSignal, sensor_data->cliffRightSignal);

}
void main_3(oi_t *sensor_data) {
	lcd_printf("%s", "Type a Command");
	uint8_t firstTime = 1;
	char* string;

	while (1) {
		string = uart_getCommand();
		int temp = 0;
		while (temp < 255 && !firstTime) {
			string[temp] = string[temp + 1];
			temp++;
		}

		lcd_printf("%s", string);

		commandList(string, sensor_data);

		firstTime = 0;
	}
}

void commandList(char *string, oi_t *sensor_data) {
	int temp = 0;

	if (compare_strings(string, foreward_cmd) == 1) {
		uart_sendString("Forwards: distance in millimeters? (0-1000mm)\n\r");
		string = uart_getCommand();
		temp = 0;
		while (temp < 255) {
			string[temp] = string[temp + 1];
			temp++;
		}
		int dist = atoi(string);
		int distanceMoved = movebot_Forward(200, dist, sensor_data);
		char buffer[100];
		sprintf(buffer, "%s%d%s\n\r", "Distance Moved: ",distanceMoved, " mm");
		uart_sendString(buffer);
	}
	if (compare_strings(string, backward_cmd) == 1) {
		uart_sendString("Backwards: distance in millimeters? (0-1000mm)\n\r");
		string = uart_getCommand();
		temp = 0;
		while (temp < 255) {
			string[temp] = string[temp + 1];
			temp++;
		}
		int dist = atoi(string);
		movebot_Backward(200, dist, sensor_data);
	}
	if (compare_strings(string, clockwise_cmd) == 1) {
		uart_sendString("Clockwise: Angle in degrees?\n\r");
		string = uart_getCommand();
		temp = 0;
		while (temp < 255) {
			string[temp] = string[temp + 1];
			temp++;
		}
		int angle = atoi(string);
		rotatebot_ClockWise(angle, sensor_data);
	}
	if (compare_strings(string, counterclockwise_cmd) == 1) {
		uart_sendString("Counterclockwise: Angle in degrees?\n\r");
		string = uart_getCommand();
		temp = 0;
		while (temp < 255) {
			string[temp] = string[temp + 1];
			temp++;
		}
		int angle = atoi(string);
		rotatebot_CounterClockWise(angle, sensor_data);
	}
	if (compare_strings(string, scan_cmd) == 1) {
			uart_sendString("Scanning...\n\r");
			distances = perform_scan();
			check_for_objects(distances);
		}
	if (compare_strings(string, getCliffSignal_cmd) == 1) {

			char buffer[100];
			sprintf(buffer, "%s: %d\t", "cliffLeftSignal", sensor_data->cliffLeftSignal);
			uart_sendString(buffer);
			sprintf(buffer, "%s: %d\t", "cliffFrontLeftSignal", sensor_data->cliffFrontLeftSignal);
			uart_sendString(buffer);
			sprintf(buffer, "%s: %d\t", "cliffFrontRightSignal", sensor_data->cliffFrontRightSignal);
			uart_sendString(buffer);
			sprintf(buffer, "%s: %d\n\r", "cliffRightSignal", sensor_data->cliffRightSignal);
			uart_sendString(buffer);


		}
	if (compare_strings(string, getIRData_cmd) == 1) {
		int i = 0;
		uart_sendString("Angle\tIR Data:\tSonar data:\n\r");
		for(i = 0; i < 180; i++)
		{
			char buffer[100];
			sprintf(buffer, "%d\t%d\t\t%f\n\r", i, distances[i].IR_cm, distances[i].sonar_cm);
			uart_sendString(buffer);
		}

		}
	if (compare_strings(string, playsong_cmd) == 1) {
		uart_sendString("SongPlayer: Enter a song index\n\r");
		string = uart_getCommand();
		temp = 0;
		while (temp < 255) {
			string[temp] = string[temp + 1];
			temp++;
		}
		int index = atoi(string);
		oi_play_song(index);
	}
}

//lcd_printf("left: %d\nfleft: %d\nfright: %d\nright: %d",sensor_data->cliffLeftSignal,sensor_data->cliffFrontLeftSignal,sensor_data->cliffFrontRightSignal,sensor_data->cliffRightSignal);
