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


static datetime_t * timeInt;


ISR(RTC_OVF_vect)
{
	RTCProcessTime(timeInt);
}

// https://stackoverflow.com/questions/7960318/math-to-convert-seconds-since-1970-into-date-and-vice-versa
//
// uint32_t RTCConvertToEpoch(datetime_t * time)
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


void RTCPrintTime(time_t * time)
{
	appUIPrint("%02u:%02u:%02u ", time->hour, time->minute, time->second);
}


void RTCPrintDate(date_t * date)
{
	appUIPrint("%04u-%02u-%02u ", 2000 + (uint16_t)date->year, date->month, date->day);
}


void RTCPrintDatetime(datetime_t * datetime)
{
	RTCPrintDate(&datetime->date);
	RTCPrintTime(&datetime->time);
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


bool RTCUpdateDate(date_t * date, uint8_t year, uint8_t month, uint8_t day)
{
	//Error checking
	if (year > 100 || month == 0 || month > 12 || day == 0 || day > RTCGetDaysInMonth(month, year)) return false;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		date->year = year;
		date->month = month;
		date->day = day;
	}
	return true;
}


bool RTCUpdateDatetime(datetime_t * datetime, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
	return (RTCUpdateDate(&datetime->date, year, month, day) && RTCUpdateTime(&datetime->time, hour, minute, second));
}


void RTCAddTime(datetime_t * datetime, uint8_t hour, uint8_t minute, uint8_t second) // need date in case of overflow
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		datetime->time.hour += hour;
		datetime->time.minute += minute;
		datetime->time.second += second;
		
		RTCProcessOverflowDatetime(datetime);
	}
}


void RTCAddDate(date_t * date, uint8_t year, uint8_t month, uint8_t day)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		date->year += year;
		date->month += month;
		date->day += day;
		
		RTCProcessOverflowDate(date);
	}
}


void RTCAddDatetime(datetime_t * datetime, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		datetime->date.year += year;
		datetime->date.month += month;
		datetime->date.day += day;
		datetime->time.hour += hour;
		datetime->time.minute += minute;
		datetime->time.second += second;
		
		RTCProcessOverflowDatetime(datetime);
	}
}


uint32_t RTCtimeUntil(time_t * time, time_t * alarm)
{
	uint32_t secondsTime, secondsAlarm;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		secondsTime = time->hour * 3600 + time->minute * 60 + time->second;
		secondsAlarm = alarm->hour * 3600 + alarm->minute * 60 + alarm->second;
	}
	
	return ( (secondsTime < secondsAlarm) ? (secondsAlarm + 86400) : (secondsAlarm) ) - secondsTime;
}


bool RTCIsValidtime(time_t * time)
{
	if (time->hour > 23)
		return false;
	if (time->minute > 59)
		return false;
	if (time->second > 59)
		return false;
	
	return true;
}


bool RTCIsValidDate(date_t * date)
{
	if (date->year < RTC_MIN_VALID_YEAR || date->year > RTC_MAX_VALID_YEAR)
	return false;
	if (date->month > 12)
	return false;
	if (date->day > RTCGetDaysInMonth(date->month, date->year))
	return false;
	
	return true;
}


bool RTCIsValidDatetime(datetime_t * datetime)
{
	return RTCIsValidDate(&datetime->date) && RTCIsValidtime(&datetime->time);
}


bool RTCIsSurpassedTime(time_t * time, time_t * alarmTime)
{
	bool surpassed = false;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
			 if (time->hour		> alarmTime->hour)		surpassed = true;
		else if (time->hour		< alarmTime->hour)		surpassed = false;
		else if (time->minute	> alarmTime->minute)	surpassed = true;
		else if (time->minute	> alarmTime->minute)	surpassed = false;
		else if (time->second	> alarmTime->second)	surpassed = true;
	}
	return surpassed;
}


bool RTCIsSurpassedDate(date_t * date, date_t * alarmDate)
{
	bool surpassed = false;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
			 if (date->year		> alarmDate->year)		surpassed = true;
		else if (date->year		< alarmDate->year)		surpassed = false;
		else if (date->month	> alarmDate->month)		surpassed = true;
		else if (date->month	< alarmDate->month)		surpassed = false;
		else if (date->day		> alarmDate->day)		surpassed = true;
	}
	return surpassed;
}


bool RTCIsSurpassedDatetime(datetime_t * datetime, datetime_t * alarmDatetime)
{
	return RTCIsSurpassedDate(&datetime->date, &alarmDatetime->date) ? (true) : (RTCIsSurpassedTime(&datetime->time, &alarmDatetime->time));
}


void RTCProcessTime(datetime_t * datetime)
{
	datetime->time.second++;
	
	RTCProcessOverflowDatetime(datetime);
}


void RTCProcessOverflowTime(datetime_t * datetime)
{
	while (datetime->time.second > 59)
	{
		datetime->time.second -= 60;
		datetime->time.minute += 1;
	}
	while (datetime->time.minute > 59)
	{
		datetime->time.minute -= 60;
		datetime->time.hour += 1;
	}
	while (datetime->time.hour > 23)
	{
		datetime->time.hour -= 24;
		datetime->date.day += 1;
	}
}


void RTCProcessOverflowDate(date_t * date)
{
	while (date->day > RTCGetDaysInMonth(date->month, date->year))
	{
		date->day -= RTCGetDaysInMonth(date->month, date->year);
		date->month += 1;
	}
	while (date->month > 12)
	{
		date->month -= 12;
		date->year += 1;
	}
}


void RTCProcessOverflowDatetime(datetime_t * datetime)
{
	RTCProcessOverflowTime(datetime);
	RTCProcessOverflowDate(&datetime->date);
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

void RTCInit(datetime_t * datetime, CLK_RTCSRC_t clkSrc, RTC_OVFINTLVL_t ovfIntLevel, RTC_PRESCALER_t prescaler, uint16_t period)
{
	timeInt = datetime;
	
	if (!RTCIsValidDatetime(datetime)) {
		RTCUpdateDatetime(datetime, RTC_MIN_VALID_YEAR, 1, 1, 0, 0, 0); //Reset date and time
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

