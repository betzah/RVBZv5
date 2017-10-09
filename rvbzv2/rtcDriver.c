/*
 * rtcDriver.c
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */ 

#include "rtcDriver.h"
#include "peripherals.h"
#include "remoteDriver.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>

static void RTCProcessTime(time_t * time);
static void RTCProcessOverflow(time_t * time);

static time_t * timeInt;

static uint8_t addSecond = 0;


ISR(RTC_OVF_vect)
{
	RTCProcessTime(timeInt);
}

// https://stackoverflow.com/questions/7960318/math-to-convert-seconds-since-1970-into-date-and-vice-versa
//
// uint32_t RTCConvertToEpoch(time_t * time)
// {
//     uint8_t _year -= (time->month <= 2);
//     uint8_t doy = (153 * (time->month + ((time->month > 2) ? (-3) : (9))) + 2) / 5 + time->day - 1;  // [0, 365]
//     uint8_t doe = _year * 365 + _year / 4 + doy	;         // [0, 146096]
//     return doe - 719468;
// 
// 
// 	uint32_t epoch = 0;
// 	epoch += time->second + time->minute * 60 + time->hour * 3600UL + time->day * 86400UL + (time->year & 0xFC) * 1461 * 86400UL;
// 	
// }


void RTCPrintAll(time_t * time)
{
	RTCPrintDate(time);
	RTCPrintTime(time);
}

void RTCPrintDate(time_t * time)
{
	appUIPrint("%04u-%02u-%02u ", 2000 + (uint16_t)time->year, time->month, time->day);
}

void RTCPrintTime(time_t * time)
{
	appUIPrint("%02u:%02u:%02u ", time->hour, time->minute, time->second);
}


bool RTCUpdateAll(time_t * time, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
	//Error checking
	if (year > 100 || month == 0 || month > 12 || day == 0 || day > RTCGetDaysInMonth(month, year) || 
		hour > 23 || minute > 59 || second > 59) return false;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		time->year = year;
		time->month = month;
		time->day = day;
		time->hour = hour;
		time->minute = minute;
		time->second = second;
	}
	return true;
}


bool RTCUpdateDate(time_t * time, uint8_t year, uint8_t month, uint8_t day)
{
	//Error checking
	if (year > 100 || month == 0 || month > 12 || day == 0 || day > RTCGetDaysInMonth(month, year)) return false;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		time->year = year;
		time->month = month;
		time->day = day;
	}
	return true;
}


bool RTCUpdateTime(time_t * time, uint8_t hour, uint8_t minute, uint8_t second)
{
	//Error checking
	if (hour > 23 || minute > 59 || second > 59) return false;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		time->hour = hour;
		time->minute = minute;
		time->second = second;
	}
	return true;
}


void RTCAddAll(time_t * time, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		time->year += year;
		time->month += month;
		time->day += day;
		time->hour += hour;
		time->minute += minute;
		time->second += second;
		
		RTCProcessOverflow(time);
	}
}


void RTCAddDate(time_t * time, uint8_t year, uint8_t month, uint8_t day)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		time->year += year;
		time->month += month;
		time->day += day;
		
		RTCProcessOverflow(time);
	}
}


void RTCAddTime(time_t * time, uint8_t hour, uint8_t minute, uint8_t second)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		time->hour += hour;
		time->minute += minute;
		time->second += second;
		
		RTCProcessOverflow(time);
	}
}


bool RTCIsValid(time_t * time)
{
	if (time->year < RTC_MIN_VALID_YEAR || time->year > RTC_MAX_VALID_YEAR)
		return false;
	if (time->month > 12)
		return false;
	if (time->day > RTCGetDaysInMonth(time->month, time->year))
		return false;
	if (time->hour > 23)
		return false;
	if (time->minute > 59)
		return false;
	if (time->second > 59)
		return false;
	
	return true;
}

bool RTCIsSurpassed(time_t * realTime, time_t * toSurpassTime)
{
	bool surpassed = false;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (realTime->year > toSurpassTime->year)				surpassed = true;																
		else if (realTime->year		< toSurpassTime->year)		surpassed = false;
		else if (realTime->month	> toSurpassTime->month)		surpassed = true;
		else if (realTime->month	< toSurpassTime->month)		surpassed = false;
		else if (realTime->day		> toSurpassTime->day)		surpassed = true;
		else if (realTime->day		< toSurpassTime->day)		surpassed = false;
		else if (realTime->hour		> toSurpassTime->hour)		surpassed = true;
		else if (realTime->hour		< toSurpassTime->hour)		surpassed = false;
		else if (realTime->minute	> toSurpassTime->minute)	surpassed = true;
		else if (realTime->minute	> toSurpassTime->minute)	surpassed = false;
		else if (realTime->second	> toSurpassTime->second)	surpassed = true;
		else													surpassed = false;
	}
	return surpassed;
}

static void RTCProcessTime(time_t * time)
{
	time->second += addSecond;
	
	RTCProcessOverflow(time);
}


static void RTCProcessOverflow(time_t * time)
{
	while (time->second > 59)
	{
		time->second -= 60;
		time->minute += 1;
	}
	while (time->minute > 59)
	{
		time->minute -= 60;
		time->hour += 1;
	}
	while (time->hour > 23)
	{
		time->hour -= 24;
		time->day += 1;
	}
	while (time->day > RTCGetDaysInMonth(time->month, time->year))
	{
		time->day -= RTCGetDaysInMonth(time->month, time->year);
		time->month += 1;
	}
	while (time->month > 12)
	{
		time->month -= 12;
		time->year += 1;
	}
}


uint8_t RTCGetDaysInMonth(uint8_t month, uint8_t year)
{
	switch (month)
	{
		case 1: return 31;
		case 2: return (RTCGetIsLeapYear(year))?(29):(28);
		case 3: return 31;
		case 4: return 30;
		case 5: return 31;
		case 6: return 30;
		case 7: return 31;
		case 8: return 31;
		case 9: return 30;
		case 10: return 31;
		case 11: return 30;
		case 12: return 31;
		default: return 30;
	}
}


bool RTCGetIsLeapYear(uint8_t year) // year starting from 2000
{
	if ((year & 0x03) != 0) {
		return false;
	}
// 	else if ((year % 100) != 0) // only needed for > year 2099
// 	{
// 		return true;
// 	}
// 	else if ((year % 400) != 0)
// 	{
// 		return false;
// 	}
	return true;
}

void RTCInit(time_t * time, CLK_RTCSRC_t clkSrc, RTC_OVFINTLVL_t ovfIntLevel, RTC_PRESCALER_t prescaler, uint16_t period, uint8_t seconds)
{
	addSecond = seconds;
	timeInt = time;
	
	if (!RTCIsValid(time)) {
		RTCUpdateAll(time, RTC_MIN_VALID_YEAR, 1, 1, 0, 0, 0); //Reset date and time
	}
	
	CLK.RTCCTRL = ( CLK.RTCCTRL & ~CLK_RTCSRC_gm ) | clkSrc | CLK_RTCEN_bm;
	
	while (RTC.STATUS & RTC_SYNCBUSY_bm);
	RTC.INTCTRL = (RTC.INTCTRL & ~(RTC_COMPINTLVL_gm | RTC_OVFINTLVL_gm ) ) | ovfIntLevel;
	
	while (RTC.STATUS & RTC_SYNCBUSY_bm);
	RTC.CTRL = RTC_PRESCALER_OFF_gc;
	
	while (RTC.STATUS & RTC_SYNCBUSY_bm);
	RTC.PER = period;
	
	while (RTC.STATUS & RTC_SYNCBUSY_bm);
	RTC.COMP = 0;
	
	while (RTC.STATUS & RTC_SYNCBUSY_bm);
	RTC.CNT = 0;
	
	while (RTC.STATUS & RTC_SYNCBUSY_bm);
	RTC.CTRL = (RTC.CTRL & ~RTC_PRESCALER_gm ) | prescaler;
}

