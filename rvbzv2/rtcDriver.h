/*
 * rtcDriver.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */ 

#pragma once


#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>


typedef struct
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} time_t;


typedef struct
{
	uint8_t year; //year starts from 2000
	uint8_t month;
	uint8_t day;
} date_t;


typedef struct
{
	time_t time;
	date_t date;
} datetime_t;


#define RTC_MIN_VALID_YEAR		10
#define RTC_MAX_VALID_YEAR		99

void RTCPrintTime(time_t * time);
void RTCPrintDate(date_t * date);
void RTCPrintDatetime(datetime_t * datetime);

bool RTCUpdateTime(time_t * time, uint8_t hour, uint8_t minute, uint8_t second);
bool RTCUpdateDate(date_t * date, uint8_t year, uint8_t month, uint8_t day);
bool RTCUpdateDatetime(datetime_t * datetime, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

void RTCAddTime(datetime_t * datetime, uint8_t hour, uint8_t minute, uint8_t second); // need date in case of overflow
void RTCAddDate(date_t * date, uint8_t year, uint8_t month, uint8_t day);
void RTCAddDatetime(datetime_t * datetime, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

bool RTCIsValidTime(time_t * time);
bool RTCIsValidDate(date_t * date);
bool RTCIsValidDatetime(datetime_t * datetime);

bool RTCIsSurpassedTime(time_t * time, time_t * alarmTime);
bool RTCIsSurpassedDate(date_t * date, date_t * alarmDate);
bool RTCIsSurpassedDatetime(datetime_t * realTime, datetime_t * toSurpassTime);

void RTCProcessTime(datetime_t * datetime);

void RTCProcessOverflowTime(datetime_t * time);
void RTCProcessOverflowDate(date_t * date);
void RTCProcessOverflowDatetime(datetime_t * datetime);

uint8_t RTCGetDaysInMonth(uint8_t month, uint8_t year);
bool RTCGetIsLeapYear(uint8_t year); // year starting from 2000

void RTCInit(datetime_t * time, CLK_RTCSRC_t clkSrc, RTC_OVFINTLVL_t ovfIntLevel, RTC_PRESCALER_t prescaler, uint16_t period);

