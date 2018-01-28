/*
 * menu.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#pragma once

#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>

/*
extern EEMEM bool ethernetEnabledEEPROM;
extern EEMEM uint8_t ethernetIPEEPROM[4];
extern EEMEM uint8_t deviceTypeEEPROM;
extern EEMEM datetime_t alarmEEPROM, timeEEPROM;
*/


#define EVERYTHINGDATA_MAXLENGTH		512

typedef struct
{
	char str[EVERYTHINGDATA_MAXLENGTH];
	uint16_t len;
	bool busy;
} everythingData_t;

void roomInit();
void roomReset();
void roomClean();

void roomKeys();
void roomPrintMenus();
void roomPrintDeviceStuff();

void printAllDataRawCommaSeperated(FILE * stream);

//void alarmSetChannel();
void alarmCancelButton();
void alarmCheck();
void writeTimeToEE();
void psuVoltageController();

