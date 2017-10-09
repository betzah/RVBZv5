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


// Repeation < 0 means infinite; 1 for single execution
bool eventAdd(TimeEvent_t interval_ms, int8_t repeation, void (*funcPtr)())
{
	for (uint8_t i = 0; i < EVENT_MAX; i++) 
	{
		if (events[i].interval == 0)
		{
			events[i].funcPtr = funcPtr;
			events[i].interval = interval_ms; // use units of 1 ms
			events[i].timeLeft = interval_ms;
			events[i].repeation = repeation;
			return true;
		}
	}
	println("Event overflow error!");
	return false;
}


// Add singleton event, if no other event exists with the same event
bool eventAddSafe(TimeEvent_t interval_ms, int8_t repeation, void (*funcPtr)())
{
	if (eventFindCount(funcPtr) == 0)
	{
		return eventAdd(interval_ms, repeation, funcPtr);
	}
	return false;
}


// Remove all events with given function pointer
uint8_t eventRemove(void (*funcPtr)())
{
	uint8_t count = 0;
	
	for (uint8_t i = 0; i < EVENT_MAX; i++) 
	{
		if (events[i].funcPtr == funcPtr && events[i].interval != 0) 
		{
			events[i].funcPtr = 0;
			events[i].interval = 0;
			events[i].timeLeft = 0;
			events[i].repeation = 0;
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


bool eventSetInterval(void (*funcPtr)(), TimeEvent_t interval)
{
	events_t * ev = eventFind(funcPtr);
	if (ev)
	{
		ev->interval = interval;
		return true;
	}
	return false;
}


bool eventSetRepeation(void (*funcPtr)(), int8_t repeation)
{
	events_t * ev = eventFind(funcPtr);
	if (ev)
	{
		ev->repeation = repeation;
		return true;
	}
	return false;
}


// Timer Event Controller
void eventControllerLoop(void) 
{
	//uint16_t cpu_on_sample = 0, cpu_off_sample = 0;
	uint32_t cpu_on_sum = 0,	cpu_off_sum = 0;
	uint32_t cpu_total_sample = 0, cpu_total_sum = 0;
	uint16_t timer_temp_sample = 0;
	uint8_t second_processed = hardware.time.second, i;
	
	TC_CPU.PER = 65535;
	TC_CPU.CCA = TC_CCA_INT_TIME;
	TC_CPU.CTRLA = TC_CLKSEL_DIV64_gc;
	TC_CPU.CTRLFSET = TC_CMD_RESTART_gc;
	TC_CPU.INTCTRLB = TC_CCAINTLVL_LO_gc;
	
	// Initialise
	set_sleep_mode(SLEEP_MODE_IDLE);
	println("Application Started!");
	
	while(1) 
	{
		// Read CPU off value
		timer_temp_sample = TC_CPU.CNT + 1; // raw timer unit
		TC_CPU.CTRLFSET = TC_CMD_RESTART_gc;
		cpu_off_sum += (uint32_t)timer_temp_sample; // raw timer unit
		cpu_total_sample += (uint32_t)timer_temp_sample; // raw timer unit
		CONVERT_TO_MS(cpu_total_sample); // convert to time per 1 ms
		
		// Perform all events
		eventsPerform(cpu_total_sample);
		
		// Read CPU on value
		timer_temp_sample = TC_CPU.CNT + 1; // raw timer unit
		TC_CPU.CTRLFSET = TC_CMD_RESTART_gc;
		cpu_on_sum += (uint32_t)timer_temp_sample; // raw timer unit
		cpu_total_sample = (uint32_t)timer_temp_sample;
		
		i = hardware.time.second;
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
			
			cpu_total_sum = cpu_off_sum + cpu_on_sum; // raw timer unit
			
			// Convert to units of 1 ms
			CONVERT_TO_MS(cpu_total_sum);
			CONVERT_TO_MS(cpu_on_sum);
			CONVERT_TO_MS(cpu_off_sum);
			
			// Save measured data
			cpu->total = cpu_total_sum;
			cpu->on = cpu_on_sum;
			cpu->off = cpu_off_sum;
			
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
			//Delete event if finished
			if (events[i].repeation == 1)
			{
				events[i].funcPtr = 0;
				events[i].interval = 0;
				events[i].timeLeft = 0;
				events[i].repeation = 0;
			}
			else 
			{
				if (events[i].repeation > 1) 
				{
					events[i].repeation--;
				}
				
				//Recalculate interval time
				do {
					events[i].timeLeft += events[i].interval;		// inefficient workaround for underflow bug...
				} while (events[i].timeLeft < cpu_total_sample);
				//events[i].timeLeft += events[i].interval - cpu_total_sample;
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
		}
		else {
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
