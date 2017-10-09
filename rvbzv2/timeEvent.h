/*
 * timeEvent.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

// Buffer size
#define EVENT_MAX		20 // one element uses 11 bytes RAM + 4 bytes RAM for cpu calculation

// Timer to count the cpu-time
#define TC_CPU						TCD0
#define EVENT_REFRESH_TIME_MS		20

#define TC_CCA_INT_TIME				(EVENT_REFRESH_TIME_MS * (F_CPU / 64 / 1000)  - 1)
#define CONVERT_TO_MS(TC_VALUE)		do { TC_VALUE /= (F_CPU / 64 / 1000); } while(0);

//#define MEASURE_EVENTS_CPU


/* CPU Cycle counter */
#define TC_CPUMETER					TCE0
#define cpuMeterStart()				do { TC_CPUMETER.CCDBUF = TC_CPUMETER.CNT; } while (0);
#define cpuMeterStop()				do { uint16_t temp = (TC_CPUMETER.CCDBUF) - (TC_CPUMETER.CNT); appUIPrintln("CPU Cycles: %u us", temp * 2); } while (0);

typedef void(*functionPtr)(void);
typedef uint32_t TimeEvent_t;


// Events struct
typedef struct
{
	functionPtr funcPtr;
	TimeEvent_t interval;		// units of 1 ms
	TimeEvent_t timeLeft;		// units of 1 ms
	
#ifdef MEASURE_EVENTS_CPU
	uint16_t cputime;		// units of 
	uint16_t cputime_temp;	// units of 
#endif

	int8_t repeation;
} events_t;


// Cpu usage struct
typedef struct
{
	uint16_t total;			// units of 1 ms
	uint16_t on;			// units of 1 ms
	uint16_t off;			// units of 1 ms
	float usage;			// in percentage
} cpu_t;



void eventInit(cpu_t * cpuPtr);
bool eventAdd(TimeEvent_t interval_ms, int8_t repeation, void (*funcPtr)());
bool eventAddSafe(TimeEvent_t interval_ms, int8_t repeation, void (*funcPtr)());
uint8_t eventRemove(void (*funcPtr)());
bool eventTimerRestart(void (*funcPtr)());
bool eventTimerTrigger(void (*funcPtr)());
events_t * eventFind(void (*funcPtr)());
uint8_t eventFindCount(void (*funcPtr)());

TimeEvent_t eventGetTimeleft(void (*funcPtr)());
TimeEvent_t eventGetInterval(void (*funcPtr)());
int8_t eventGetRepeation(void (*funcPtr)());
bool eventSetTimeleft(void (*funcPtr)(), TimeEvent_t timeleft);
bool eventSetInterval(void (*funcPtr)(), TimeEvent_t interval);
bool eventSetRepeation(void (*funcPtr)(), int8_t repeation);

void eventControllerLoop(void);

