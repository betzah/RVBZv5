/*
 * main.cpp
 *
 * Created: 1/1/2015 0:00:0 PM
 *  Author: Mohammed
 */ 


extern "C" 
{
	#include "peripherals.h"
}


void led()
{
	static uint8_t k = 0;
	//k = (k + 1) & 0x03;
	
	remoteCommand_t cmd;
	cmd.devices_bm = 0x01;
	cmd.channelNumber = k++;
	cmd.device_type = device_bein;
	cmd.key = channel_number;
 	remoteSendCommand(&cmd);

	//printAllDataRawCommaSeperated(stdout);
	
	//remoteSetPowerGPIO(k & 0x01, k & 0x02, 0xFFF);
}


/* bugs: 
 *
 * repeat off by 1 bug 
 * adc 2092 mA
 *
 *
*/



int main()
{
	// Initialization
	initHardware();
	
	// Start Timer Events
	//eventAdd(5000,				-1, &led);
	eventAdd(1000,				-1, &adcMeasure);
	eventAdd(125,				-1, &TMP112Read);
	//eventAdd(500,				-1, &RX8900Read);
	eventAdd(1000,				-1, &ledController);
	eventAdd((uint32_t)1000 * 120,		-1, &roomReset);				// Return to main menu after few min of inactivity
	eventAdd(3000,				-1, &roomClean);				// Clean screen after few sec of inactivity
	eventAdd(100,				-1, &roomKeys);					// Read USB input
	eventAdd(1000,				-1, &roomPrintDeviceStuff);		// Update UI top screen
	eventAdd(1000,				-1, &roomPrintMenus);			// Update UI menus
	//eventAdd(1000,				-1, &psuVoltageController);		//
	eventAdd((uint32_t)1000 * 900,		-1, &writeTimeToEE);			// Save every 15 min: 35k/y cycles

// 	eventAdd(5000,				-1, &alarmCheck);				//
// 	eventAdd((uint32_t)1000 * 10,		-1, &alarmCancelButton);		//
// 	eventAdd((uint32_t)1000 * 3600 * 3,	-1, &alarmSetChannel);		//
	
	// Infinite loop
	eventControllerLoop(); 
}
