/*
 * remoteDriver.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#pragma once

#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>

typedef enum  
{
	noone,
	
	power_switch,
	poweron_switch,
	poweroff_switch,
	channel_number,
	
	power,
	arrow_up,
	arrow_down,
	arrow_right,
	arrow_left,
	confirm,
	back,
	cancel,
	home,				// No Humax
	menu,
	channel_up,
	channel_down,
	volume_up,
	
	audio,				// No Bein
	subs,				// No Bein
	
	option,
	favorite,			// No Humax
	schedule,			// No Humax
	
	red,
	green,
	yellow,
	blue,

	zero,
	one,
	two,
	three,
	four,
	five,
	six,
	seven,
	eight,
	nine,
	
	last
} keys;


typedef enum
{
	devicegroup_noone = 0x00,
	devicegroup_B = 0x01,
	devicegroup_A = 0x02,
	devicegroup_AB = 0x03,
} deviceGroup;


typedef enum
{
	device_noone,
	device_humax,
	device_bein,
	device_mbc	
} deviceTypes;


typedef struct
{
	deviceTypes device_type; /////
	uint16_t devices_bm;
	uint16_t channelNumber;
	keys key;
} remoteCommand_t;


typedef struct
{
	deviceTypes type;
	uint8_t number;
	uint8_t numberTotal;
	uint16_t numberTotal_bm;
} device_t;


// HUMAX: 9ms pulse, 4.5ms space. 1 = 562.5us pulse, 1687.5us space. 0 = 562.5us pulse, 562.5us space. stopbit: 562.5 space.
// first 16 bits are address. 8 bits data, followed by inverse of the same 8 bits data
// Repeat code: 9ms pulse, 2.25 space, 562.5 pulse.


// Constants / variables
//#define DEVICES_PER_GROUP 8

// Delays. Example for channel 10: press btn 1, wait 300ms, press btn 0
#define DELAY_BETWEEN_TWO_KEYS				300

//Reboot delays
#define DELAY_BETWEEN_POWEROFF_2DEVICES		250
#define DELAY_AFTER_POWEROFF				30000
#define DELAY_BETWEEN_POWERON_2DEVICES		250


/* This MACRO convert ASCII upper case (65~90) characters to lower case (97~122) characters. */
#define characterConvertLower(key)		do { if (key >= 'A' && key <= 'Z') { key += 'a' - 'A'; } } while (0);

void remoteSendCommand(const remoteCommand_t *cmd);

void remoteSetPowerGPIO(bool isHigh, deviceGroup group, uint16_t devices_bm);