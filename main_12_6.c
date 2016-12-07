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
#include <math.h>


#define PI 3.14159265358979323846

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

// Strcut and variables for GUI
struct point
{
	double x_coord;
	double y_coord;
};

int* polar_angles;
struct point points[180];
char grid[29][111];


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
uint16_t max_black_color = 1400;

//min value for the cliff sensor signal to detect red.
uint16_t min_black_color = 0;

//Speed of the robot.
int botSpeed = 200;

int end = 0;


//clears Grid
void clearGrid(void);

//displays a textual protrator on putty.
void print_Protractor();

//Plots points into a textual grid.
void createGrid();

//Converts polar coordinates to cartesian points.
void polarToCartesian();

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


void main_1(oi_t *sensor_data);

int main(void) {
	lcd_init();
	lcd_printf("Start");
	timer_waitMillis(100);
	lcd_printf("Waiting...");
	oi_t *sensor_data = oi_alloc();
	oi_init(sensor_data);
	button_init();
	ADC0_init();
	scanner_init();
	init_sonar_timer();
	songSetup();
	uart_init();
	WiFi_start("password");
	uart_sendString("\n\r");
	uart_sendString("\n\r");
	uart_sendString("START!!!!!!!!\n\r");
	uart_sendString("\n\r");

	lcd_printf("Buttons:\n1 main_1\n2 main_2");

	//goes to Bot programm
	main_1(sensor_data);
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
		// sensor checks
		oi_update(self);

		if (self->wheelDropLeft || self->wheelDropRight) {
			uart_sendString("Wheels Dropped!!!!\n\r");
			movebot_Hazard(self);
			sum -= 50;
			end = 0;
			return sum;
		} else if (self->cliffLeft) {
			lcd_printf("Cliff to Left!!!!");
			uart_sendString("Cliff to Left!!!!\n\r");
			sum = distance;
			movebot_Hazard(self);
			sum -= 50;
			end = 0;
			return sum;
		} else if (self->cliffFrontLeft) {
			lcd_printf("Cliff to Front Left!!!!");
			uart_sendString("Cliff to front Left!!!!\n\r");
			sum = distance;
			movebot_Hazard(self);
			sum -= 50;
			end = 0;
			return sum;
		} else if (self->cliffFrontRight) {
			lcd_printf("Cliff to Front Right!!!!");
			uart_sendString("Cliff to front right!!!!\n\r");
			sum = distance;
			movebot_Hazard(self);
			sum -= 50;
			end = 0;
			return sum;
		} else if (self->cliffRight) {
			lcd_printf("Cliff to Right!!!!");
			uart_sendString("Cliff to right!!!!\n\r");
			sum = distance;
			movebot_Hazard(self);
			sum -= 50;
			end = 0;
			return sum;
		} else if (self->bumpLeft) {
			lcd_printf("Object Hit to Left!!!!");
			uart_sendString("Object Hit to Left!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			end = 0;
			return sum;
		} else if (self->bumpRight) {
			lcd_printf("Object Hit to Right!!!!");
			uart_sendString("Object Hit to Right!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			end = 0;
			return sum;
		}
		else if (self->cliffLeftSignal < max_black_color && end == 0) {
			lcd_printf("End Goal left!!!!");
			uart_sendString("End Goal left!!!!\n\r");
			oi_play_song(1);
			oi_setWheels(0, 0);
			end = 1;
			rotatebot_CounterClockWise(60,self);
			movebot_Forward(botSpeed, 200,self);

			end = 0;
			return sum;
		} else if (self->cliffFrontLeftSignal < max_black_color && end == 0) {
			lcd_printf("End Goal front left!!!!");
			uart_sendString("End Goal front left!!!!\n\r");
			oi_play_song(1);
			oi_setWheels(0, 0);
			end = 1;
			rotatebot_CounterClockWise(30,self);
			movebot_Forward(botSpeed, 200,self);

			end = 0;
			return sum;
		} else if (self->cliffFrontRightSignal < max_black_color && end == 0) {
			lcd_printf("End Goal front right!!!!");
			uart_sendString("End Goal front right!!!!\n\r");
			oi_play_song(1);
			oi_setWheels(0, 0);
			end = 1;
			rotatebot_ClockWise(30, self);
			movebot_Forward(botSpeed, 200, self);
			end = 0;
			return sum;
		} else if (self->cliffRightSignal < max_black_color && end == 0) {
			lcd_printf("End Goal right!!!!");
			uart_sendString("End Goal right!!!!\n\r");
			oi_play_song(1);
			oi_setWheels(0, 0);
			end = 1;
			rotatebot_ClockWise(60,self);
			movebot_Forward(botSpeed, 200, self);
			end = 0;
			return sum;
		}
		else if (self->cliffLeftSignal > min_white_color
				&& self->cliffLeftSignal < max_white_color) {
			lcd_printf("Border left!!!!");
			uart_sendString("Border left!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			end = 0;
			return sum;
		} else if (self->cliffFrontLeftSignal > min_white_color
				&& self->cliffFrontLeftSignal < max_white_color) {
			lcd_printf("Border front left!!!!");
			uart_sendString("Border front left!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			end = 0;
			return sum;
		} else if (self->cliffFrontRightSignal > min_white_color
				&& self->cliffFrontRightSignal < max_white_color) {
			lcd_printf("Border front right!!!!");
			uart_sendString("Border front right!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			end = 0;
			return sum;
		} else if (self->cliffRightSignal > min_white_color
				&& self->cliffRightSignal < max_white_color) {
			lcd_printf("Border right!!!!");
			uart_sendString("Border right!!!!\n\r");
			oi_play_song(0);
			oi_setWheels(0, 0);
			end = 0;
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
	oi_play_song(0);
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
	unsigned char note0[] = { 70, 70 };
	unsigned char dur0[] = { 8, 8 };
	oi_loadSong(0, 2, note0, dur0);

	//song1, secret, Goal Tone
	unsigned char note1[] = { 79, 78, 75, 69, 68, 75, 80, 84 };
	unsigned char dur1[] = { 8, 8, 8, 8, 8, 8, 8, 32 };
	oi_loadSong(1, 8, note1, dur1);
}


/*
 * Checks String Inputs from Putty
 */
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

/*
 * Main program using Putty
 */
void main_1(oi_t *sensor_data) {
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

/*
 * Method for Putty Commands
 */
void commandList(char *string, oi_t *sensor_data) {
	int temp = 0;
	// Move foward
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
	//Move Backward
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
	// Turn Clockwise
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
	//Turn Coutercloackwise
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
	// Do Scan
	if (compare_strings(string, scan_cmd) == 1) {
			uart_sendString("Scanning...\n\r");
			distances = perform_scan();
			check_for_objects(distances);
			clearGrid();
			polarToCartesian();
			createGrid();
			print_Protractor();
		}
	// Show Sensor data
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
	// Show Data
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
	//Play Song
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


//////////////////////////////////////////////////////////////////Stuff For GUI//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *  Clears any preset Characters from Grind
 */
void clearGrid()
{
	int i;
	int j;
	for(i = 0; i < 29; i++)
	{
		for(j = 0; j < 111; j++)
		{
			if(j == 110)
			{
				grid[i][j] = '\0';
			}
			else
			{
				grid[i][j] = 32;
			}
		}
	}
}

/*
 * Converts gathered Data from Polar to Cartesian
 */
void polarToCartesian()
{
	polar_angles = get_angles();
	int i;
	for(i=0;i < 180; i++)
	{
		double sonar_distance = scan_distance[i];
		double x_temp = (sonar_distance * cos(i));
		double y_temp = (sonar_distance * sin(i));
		points[i].x_coord = x_temp;
		points[i].y_coord = y_temp;
	}
}

/*
 *Creates Grid
 */
void createGrid()
{
	grid[0][1] = '*'; grid[1][0] = '9'; grid[1][1] = '0'; grid[2][0] = '1'; grid[2][1] = '8'; grid[2][2] = '0';

	grid[13][10] = '*'; grid[14][9] = '6'; grid[14][10] = '0'; grid[15][8] = '1'; grid[15][9] = '5'; grid[15][10] = '0';

	grid[22][28] = '*'; grid[23][27] = '3'; grid[23][28] = '0'; grid[24][26] = '1'; grid[24][27] = '2'; grid[24][28] = '0';

	grid[26][55] = '*'; grid[27][55] = '0'; grid[28][54] = '9'; grid[28][55] = '0';

	grid[22][80] = '*'; grid[23][80] = '3'; grid[23][81] = '0'; grid[24][80] = '6'; grid[24][81] = '0';

	grid[13][98] = '*'; grid[14][98] = '6'; grid[14][99] = '0'; grid[15][98] = '3'; grid[15][99] = '0';

	grid[0][106] = '*'; grid[1][106] = '9'; grid[1][107] = '0'; grid[2][106] = '0';

	int i;
	for(i = 2; i < 106; i++)
	{
		grid[0][i] = '-';
	}

	grid[0][14] = '1';
	grid[0][94] = '1';
	grid[0][54] = '+';


	for(i = 0; i < 180; i++)
	{

		int gx = (int)((points[i].x_coord + 274)/5);
		int gy = (int)((points[i].y_coord)/5);

		if(gx < 0 || gx > 300 || gy < 0 || gy > 300)
			grid[0][0] = 'x';
		else
			grid[gy][gx] = 'o';
	}
}

/*
 * Prints Characters into Grid on Putty
 */
void print_Protractor()
{
	int i;
	for(i = 28; i >= 0; i--)
	{
		char buffer[120];
		sprintf(buffer, "%s\n\r", grid[i]);
		uart_sendStr(buffer);
	}
}
