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



/* bugs: 
 *
 * adc 2092 mA
 * date / time full rewritten, retest?
 * check time events code
 *
 * FIXED bugs
 * repeat off by 1 bug = fixed
 *
*/

uint32_t getCpuCycles()
{
	static uint32_t prev = 0;
	uint32_t t, diff;
	uint16_t t1, t2;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		t1 = (uint16_t) TCC0.CNT;
		t2 = (uint16_t) TCC1.CNT;	
	}
	t1 *= 256;
	t = (uint32_t) t2 << 16 | t1;
	t/= 1000;
	if (t > prev) {
		diff = t - prev;
	}
	else {
		diff = prev - t;
	}
	prev = t;
	
	appUIPrintln("%lu, %u, %u, %lu", diff, t1, t2, prev);
	
	return diff;
}


void test()
{
	 
}


int main()
{
	// Initialization
	initHardware();
	
// 	TCC1.CTRLA = TC_CLKSEL_OFF_gc;
// 	TCC1.CTRLFSET = TC_CMD_RESET_gc;
// 	TCC1.CNT = 60000;
// 	TCC1.PER = 0xFFFF;
// 	TCC1.CTRLD = TC_EVACT_CAPT_gc | TC0_EVDLY_bm | TC_EVSEL_CH4_gc;
// 	TCC1.CTRLA = TC_CLKSEL_EVCH4_gc;
// 	EVSYS.CH4MUX = EVSYS_CHMUX_TCC0_OVF_gc;
// 	TCC0.CTRLA = TC_CLKSEL_OFF_gc;
// 	TCC0.CTRLFSET = TC_CMD_RESET_gc;
// 	TCC0.PER = 0xFFFF;
// 	//TCC0.CCA = 31; // 1 us
// 	//TCC0.CCB = 31999; // 1 ms
// 	TCC0.CTRLD = TC_EVACT_CAPT_gc | TC0_EVDLY_bm;// | TC_EVSEL_CH4_gc;
// 	TCC0.CTRLA = TC_CLKSEL_DIV1_gc;
	
	
	
	eventAdd(100UL,				0, &test);
	
	// Start Timer Events
	eventAdd(1000UL,				0, &adcMeasure);
	eventAdd(125UL,				0, &TMP112Read);
	//eventAdd(500UL,				0, &RX8900Read);
	eventAdd(1000UL,				0, &ledController);
	eventAdd(1000UL * 120,		0, &roomReset);				// Return to main menu after few min of inactivity
	eventAdd(3000UL,				0, &roomClean);				// Clean screen after few sec of inactivity
	eventAdd(100UL,				0, &roomKeys);					// Read USB input
	eventAdd(1000UL,				0, &roomPrintDeviceStuff);		// Update UI top screen
	eventAdd(1000UL,				0, &roomPrintMenus);			// Update UI menus
	//eventAdd(1000UL,				0, &psuVoltageController);		//
	eventAdd(1000UL * 900,		0, &writeTimeToEE);			// Save every 15 min: 35k/y cycles

	eventAdd(5000UL,				0, &alarmCheck);				//
	eventAdd(1000UL * 10,		0, &alarmCancelButton);		//
	//eventAdd(1000UL * 3600 * 3,	0, &alarmSetChannel);		//
	
	// Infinite loop
	eventControllerLoop(); 
}
