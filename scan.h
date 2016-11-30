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
	int sonar_cm;
	int IR_cm;
};

void scanner_init();
struct distance_info * perform_scan();
void check_for_objects(struct distance_info * distances);
#endif /* SCAN_H_ */
