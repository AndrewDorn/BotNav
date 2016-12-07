/*
 * scan.h
 *
 *  Created on: Nov 15, 2016
 *      Author: asmola
 */

#ifndef SCAN_H_
#define SCAN_H_

#include <inc/tm4c123gh6pm.h>

struct distance_info
{
	double sonar_cm;
	int IR_cm;
};

int scan_angles[180];
double scan_distance[180];
//Distance Data
struct distance_info *distances;

// Initalaize Scanner
void scanner_init();

// Prefroms Scan
struct distance_info * perform_scan();

//Read Object data
void check_for_objects(struct distance_info * distances);

int get_distanceCounter(void);

int* get_angles(void);

#endif /* SCAN_H_ */
