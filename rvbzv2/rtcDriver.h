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
	uint8_t year; //year starts from 2000
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} time_t;

#define RTC_MIN_VALID_YEAR		10
#define RTC_MAX_VALID_YEAR		99

void RTCPrintAll(time_t * time);
void RTCPrintDate(time_t * time);
void RTCPrintTime(time_t * time);

bool RTCUpdateAll(time_t * time, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
bool RTCUpdateDate(time_t * time, uint8_t year, uint8_t month, uint8_t day);
bool RTCUpdateTime(time_t * time, uint8_t hour, uint8_t minute, uint8_t second);

void RTCAddAll(time_t * time, uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
void RTCAddDate(time_t * time, uint8_t year, uint8_t month, uint8_t day);
void RTCAddTime(time_t * time, uint8_t hour, uint8_t minute, uint8_t second);

bool RTCIsValid(time_t * time);
bool RTCIsSurpassed(time_t * realTime, time_t * toSurpassTime);

uint8_t RTCGetDaysInMonth(uint8_t month, uint8_t year);
bool RTCGetIsLeapYear(uint8_t year); // year starting from 2000

void RTCInit(time_t * time, CLK_RTCSRC_t clkSrc, RTC_OVFINTLVL_t ovfIntLevel, RTC_PRESCALER_t prescaler, uint16_t period, uint8_t seconds);

