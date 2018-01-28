/*
 * timeEvent.c
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */ 
 
#include "timeEvent.h"
#include "peripherals.h"

static void eventsPerform(TimeEvent_t cpu_total_sample);

static events_t events[EVENT_MAX];

static cpu_t * cpu = NULL;


void eventInit(cpu_t * cpuPtr)
{
	cpu = cpuPtr;
}


bool eventAdd(TimeEvent_t interval_ms, int8_t repeation, void (*funcPtr)())
{
	for (uint8_t i = 0; i < EVENT_MAX; i++) 
	{
		if (events[i].interval == 0)
		{
			events[i].funcPtr = funcPtr;
			events[i].interval = interval_ms; // use units of 1 ms
			events[i].timeLeft = interval_ms;
			events[i].repeation = repeation & 0x7F;
			events[i].isAbsoluteTime = 0;
			return true;
		}
	}
	println("Event overflow error!");
	return false;
}


bool eventAddAlarm(time_t start, time_t interval, int8_t repeation, void (*funcPtr)())
{
	for (uint8_t i = 0; i < EVENT_MAX; i++)
	{
		if (events[i].interval == 0)
		{
			events[i].funcPtr = funcPtr;
			//events[i].interval = interval; // use units of 1 ms
			//events[i].timeLeft = RTCtimeUntil(&hardware.datetime, ;////////////////////////////////
			events[i].repeation = repeation & 0x7F;
			events[i].isAbsoluteTime = 1;
			return true;
		}
	}
	println("Event overflow error!");
	return false;
}

// Add singleton event, if no other event exists with the same event
// bool eventAddSafe(TimeEvent_t interval_ms, int8_t repeation, void (*funcPtr)())
// {
// 	if (eventFindCount(funcPtr) == 0)
// 	{
// 		return eventAdd(interval_ms, repeation, funcPtr);
// 	}
// 	return false;
// }


// Remove all events with given function pointer
uint8_t eventRemove(void (*funcPtr)())
{
	uint8_t count = 0;
	
	for (uint8_t i = 0; i < EVENT_MAX; i++) 
	{
		if (events[i].funcPtr == funcPtr && events[i].interval != 0) 
		{
			memset(&events[i], 0, sizeof * events);
			count++;
		}
	}
	//println("Events removed: %u", count);
	return count;
}


// Change timeLeft to interval to restart timer
bool eventTimerRestart(void (*funcPtr)())
{
	events_t * ev = eventFind(funcPtr);
	if (ev != NULL)
	{
		ev->timeLeft = ev->interval;
		return true;
	}
	return false;
}


// Change timeLeft to zero and trigger immediately
bool eventTimerTrigger(void (*funcPtr)())
{
	events_t * ev = eventFind(funcPtr);
	if (ev != NULL) 
	{
		ev->timeLeft = 0;
		return true;
	}
	return false;
}


// Find an event with a given function pointer
events_t * eventFind(void (*funcPtr)())
{
	for (uint8_t i = 0; i < EVENT_MAX; i++)
	{
		if (events[i].funcPtr == funcPtr)
		{
			return &events[i];
		}
	}
	return NULL;
}


// Get number of events with given function pointer
uint8_t eventFindCount(void (*funcPtr)())
{
	uint8_t cnt = 0;
	for (uint8_t i = 0; i < EVENT_MAX; i++)
	{
		if (events[i].funcPtr == funcPtr)
		{
			cnt++;
		}
	}
	return cnt;
}


TimeEvent_t eventGetTimeleft(void (*funcPtr)())
{
	events_t * ev = eventFind(funcPtr);
	return (ev) ? (ev->timeLeft) : (0);
}


TimeEvent_t eventGetInterval(void (*funcPtr)())
{
	events_t * ev = eventFind(funcPtr);
	return (ev) ? (ev->interval) : (0);
}


int8_t eventGetRepeation(void (*funcPtr)())
{
	events_t * ev = eventFind(funcPtr);
	return (ev) ? (ev->repeation) : (0);
}


bool eventSetTimeleft(void (*funcPtr)(), TimeEvent_t timeleft)
{
	events_t * ev = eventFind(funcPtr);
	if (ev)
	{
		ev->timeLeft = timeleft;
		return true;
	}
	return false;
}


bool eventSetInterval(void (*funcPtr)(), TimeEvent_t interval, TimeEvent_t timeleft)
{
	events_t * ev = eventFind(funcPtr);
	if (ev)
	{
		ev->interval = interval;
		ev->timeLeft = timeleft;
		return true;
	}
	return false;
}


bool eventSetRepeation(void (*funcPtr)(), int8_t repeation)
{
	events_t * ev = eventFind(funcPtr);
	if (ev)
	{
		ev->repeation = repeation & 0x7F;
		return true;
	}
	return false;
}


uint16_t cpuGetCyclesSinceLastCall()
{
	static uint16_t prevValue = 0;
	static bool isInitialized = false;
	uint16_t sample = TC_CPU.CNT;
	uint16_t result = sample - prevValue + 1;
	prevValue = sample;
	
	//TC_CPU.CTRLFSET = TC_CMD_RESTART_gc;
	if (!isInitialized) 
	{
		TC_CPU.PER = 65535;
		TC_CPU.CCA = TC_CCA_INT_TIME;
		TC_CPU.CTRLA = TC_CLKSEL_DIV64_gc;
		TC_CPU.CTRLFSET = TC_CMD_RESTART_gc;
		TC_CPU.INTCTRLB = TC_CCAINTLVL_LO_gc;
	}
	
	return result;
}


// Timer Event Controller
void eventControllerLoop(void) 
{
	uint32_t cpu_on_sum = 0, cpu_off_sum = 0, cpu_total_sample = 0;
	uint8_t i, second_processed = hardware.datetime.time.second;
	
	TC_CPU.PER = 65535;
	TC_CPU.CCA = TC_CCA_INT_TIME;
	TC_CPU.CTRLA = TC_CLKSEL_DIV64_gc;
	TC_CPU.CTRLFSET = TC_CMD_RESTART_gc;
	TC_CPU.INTCTRLB = TC_CCAINTLVL_LO_gc;
	
	//cpuGetCyclesSinceLastCall();
	
	// Initialise
	set_sleep_mode(SLEEP_MODE_IDLE);
	println("Application Started!");
	
	while(1) 
	{
		// Read CPU off value
		uint16_t sample = TC_CPU.CNT + 1;
		TC_CPU.CTRLFSET = TC_CMD_RESTART_gc;
		//uint16_t sample = cpuGetCyclesSinceLastCall(); // raw timer unit
		cpu_off_sum += (uint32_t)sample;
		cpu_total_sample += (uint32_t)sample;
		
		// Perform all events
		eventsPerform(CONVERT_TO_MS(cpu_total_sample));
		
		// Read CPU on value
		sample = TC_CPU.CNT + 1;
		TC_CPU.CTRLFSET = TC_CMD_RESTART_gc;
		//sample = cpuGetCyclesSinceLastCall(); // raw timer unit
		cpu_on_sum += (uint32_t)sample;
		cpu_total_sample = (uint32_t)sample;
		
		i = hardware.datetime.time.second;
		if (second_processed != i) 
		{
			second_processed = i;
			
			#ifdef MEASURE_EVENTS_CPU
			for (i = 0; i < EVENT_MAX; i++)
			{
				events[i].cputime = events[i].cputime_temp;
				events[i].cputime_temp = 0;
				CONVERT_TO_MS(events[i].cputime);
			}
			#endif
			
			uint32_t cpu_total_sum = cpu_off_sum + cpu_on_sum; // raw timer unit
			
			// Save measured data in ms
			cpu->total = CONVERT_TO_MS(cpu_total_sum);
			cpu->on = CONVERT_TO_MS(cpu_on_sum);
			cpu->off = CONVERT_TO_MS(cpu_off_sum);
			
			// Calc CPU usage in percent
			cpu->usage = (float) 100 * cpu_on_sum / cpu_total_sum;
			
			// Reset counters
			//cpu_total_sum = 0; // not necessary
			cpu_on_sum = 0;
			cpu_off_sum = 0;
		}
		
		//Sleep till interrupt
		WDT_RESET();
		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();
		WDT_RESET();
	}
}


static void eventsPerform(TimeEvent_t cpu_total_sample)
{
	//Loop through all events
	for (uint8_t i = 0; i < EVENT_MAX; i++)
	{
		//Check if event is valid
		if (events[i].interval == 0) continue;
		
		if (events[i].timeLeft <= cpu_total_sample)
		{
			bool isContinueExecution = false;
			
			if (events[i].repeation != 1) // if not last execution, recalculate interval
			{
				isContinueExecution = true;
				do {
					events[i].timeLeft += events[i].interval;		// inefficient workaround for underflow bug...
				} while (events[i].timeLeft <= cpu_total_sample);
				//events[i].timeLeft += events[i].interval - cpu_total_sample; // save processing time at cost of accuracy...
			}
			
			if (events[i].repeation >= 1) // if not infinite execution, count down
			{
				events[i].repeation--;
			}
			
			//Perform function and count CPU-time
			#ifdef MEASURE_EVENTS_CPU
			uint16_t temp = TC_CPU.CNT;
			#endif
			
			WDT_RESET();
			events[i].funcPtr();
			WDT_RESET();
			
			#ifdef MEASURE_EVENTS_CPU
			events[i].cputime_temp += TC_CPU.CNT - temp
			#endif
			
			if (!isContinueExecution) // if last execution
			{
				memset(&events[i], 0, sizeof * events);
			}
		}
		else 
		{
			events[i].timeLeft -= cpu_total_sample;
		}
	}
}


//EMPTY_INTERRUPT(TCD0_CCA_vect)
ISR(TCD0_CCA_vect)
{
	CDC_Device_USBTask(&USBSerialClass);
	USB_USBTask();
	freeRam();
}
