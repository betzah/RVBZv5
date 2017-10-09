/*
 * remoteDriver.c
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#include "remoteDriver.h"
#include "peripherals.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>


// Static functions
static void remoteSetDataGPIO(bool state, bool hiZ, uint16_t devices_bm);
static void remoteRebootCallback();

static void remoteSend(const remoteCommand_t *cmd);
static void remoteSend_HUMAX(const remoteCommand_t *cmd);
static void remoteSend_BEIN(const remoteCommand_t *cmd);
static void remotePrintCommand(const remoteCommand_t *cmd);
static uint32_t getKeyCode(const remoteCommand_t *cmd);

static uint16_t rebootDevices_bm = 0;


static void remoteSetDataGPIO(bool state, bool hiZ, uint16_t devices_bm)
{
	if (hiZ)
	{
		PORTJ.DIRCLR = devices_bm & 0x00FF;
		PORTK.DIRCLR = (devices_bm & 0x0F00) >> 8;
		
		PORTJ.OUTCLR = devices_bm & 0x00FF;
		PORTK.OUTCLR = (devices_bm & 0x0F00) >> 8;
	}
	else if (state)
	{
		PORTJ.DIRSET = devices_bm & 0x00FF;
		PORTK.DIRSET = (devices_bm & 0x0F00) >> 8;
		
		PORTJ.OUTSET = devices_bm & 0x00FF;
		PORTK.OUTSET = (devices_bm & 0x0F00) >> 8;
	}
	else
	{
		PORTJ.DIRSET = devices_bm & 0x00FF;
		PORTK.DIRSET = (devices_bm & 0x0F00) >> 8;
		
		PORTJ.OUTCLR = devices_bm & 0x00FF;
		PORTK.OUTCLR = (devices_bm & 0x0F00) >> 8;
	}
}


void remoteSetPowerGPIO(bool isPowerOn, deviceGroup group, uint16_t devices_bm)
{
	// Device E0: CH0B, E1: CH0A, E2: CH1B, F0: CH4B, H0: CH8B, etc.

	uint8_t part1, part2, part3;
	
	part1 = ((devices_bm & 0x001) ? (group << 0) : (0)) |
			((devices_bm & 0x002) ? (group << 2) : (0)) |
			((devices_bm & 0x004) ? (group << 4) : (0)) |
			((devices_bm & 0x008) ? (group << 6) : (0));
	
	part2 = ((devices_bm & 0x010) ? (group << 0) : (0)) |
			((devices_bm & 0x020) ? (group << 2) : (0)) |
			((devices_bm & 0x040) ? (group << 4) : (0)) |
			((devices_bm & 0x080) ? (group << 6) : (0));
	
	part3 = ((devices_bm & 0x100) ? (group << 0) : (0)) |
			((devices_bm & 0x200) ? (group << 2) : (0)) |
			((devices_bm & 0x400) ? (group << 4) : (0)) |
			((devices_bm & 0x800) ? (group << 6) : (0));
	
	if (isPowerOn)
	{
		PORTE.OUTSET = part1;
		PORTF.OUTSET = part2;
		PORTH.OUTSET = part3;
	}
	else
	{
		PORTE.OUTCLR = part1;
		PORTF.OUTCLR = part2;
		PORTH.OUTCLR = part3;
	}
	//appUIPrintln("%u, %u, 0x%04x, PORTS: 0x%02x, 0x%02x, 0x%02x", isPowerOn, group, devices_bm, (uint8_t)(PORTE.OUT), (uint8_t)(PORTF.OUT), (uint8_t)(PORTH.OUT));
}



static void remoteRebootCallback()
{
	static uint16_t devices_bm = 0;
	static uint8_t device_number = 0;
	static bool phasePowerOn = false;
	
	if (rebootDevices_bm != 0)
	{
		devices_bm = rebootDevices_bm;
		device_number = 0;
		phasePowerOn = false;
		rebootDevices_bm = 0;
	}
	
	if (devices_bm & (1 << device_number))
	{
		if (phasePowerOn)
		{
			appUIPrintln("Powering On device %u A+B", device_number + 1);
		}
		else
		{
			appUIPrintln("Powering Off device %u A+B", device_number + 1);
		}
		remoteSetPowerGPIO(phasePowerOn, devicegroup_AB, (1 << device_number));
	}
	
	if (++device_number > hardware.device.numberTotal - 1) 
	{
		if (phasePowerOn) 
		{
			eventRemove(&remoteRebootCallback);
			phasePowerOn = false;
		}
		phasePowerOn = true;
		device_number = 0;
	}
}


void remoteSendCommand(const remoteCommand_t *cmd)
{
	remotePrintCommand(cmd);
	
	if (cmd->key == noone) 
	{
		appUIPrintln(" Error: empty command!");
		return;	
	}
	
	if (cmd->key == power_switch)
	{
		//Start callback timer
		if (eventFindCount(&remoteRebootCallback) == 0) 
		{
			if (eventAdd(200, -1, &remoteRebootCallback))
			{
				rebootDevices_bm = cmd->devices_bm;
				return;
			}
		}
		appUIPrintln("Error: reboot is aborted! Already busy rebooting or internal error.");
		return;
	}
	
	if (cmd->key == channel_number)
	{
		remoteCommand_t cmd_temp = *cmd;
		
		for (uint8_t i = 0; i < 8; i++)
		{
			if (cmd->devices_bm & (1 << i))
			{
				cmd_temp.devices_bm = 1 << i;
				
				char digits[5];
				snprintf_P(digits, sizeof digits, PSTR("%u"), cmd_temp.channelNumber);
				
				for (int8_t j = sizeof digits - 1; j >= 0; i--)
				{
					switch (digits[j])
					{
						case 0: cmd_temp.key = zero;	break;
						case 1: cmd_temp.key = one;		break;
						case 2: cmd_temp.key = two;		break;
						case 3: cmd_temp.key = three;	break;
						case 4: cmd_temp.key = four;	break;
						case 5: cmd_temp.key = five;	break;
						case 6: cmd_temp.key = six;		break;
						case 7: cmd_temp.key = seven;	break;
						case 8: cmd_temp.key = eight;	break;
						case 9: cmd_temp.key = nine;	break;
						default: cmd_temp.key = noone;	break;
					}
					
					if (cmd_temp.key != noone) {
						//remoteSend(&cmd_temp);
					}
					
					if (j > 0) {
						_delay_ms(DELAY_BETWEEN_TWO_KEYS);
					}
				}
			}
		}
		return;
	}
	
	appUIPrint(", devices 0x%03x, keycode 0x%08x", cmd->devices_bm, getKeyCode(cmd));
	
	if (cmd->key == power && (cmd->device_type == device_humax || cmd->device_type == device_mbc))
	{
		for (uint8_t i = 0; i < 3; i++)
		{
			remoteSend(cmd);
		}
		return;
	}
	
	remoteSend(cmd);
	appUIPrintln(" done.\r\n")
}


static void remoteSend(const remoteCommand_t *cmd)
{
	remoteSetDataGPIO(1, 0, 0xFFF);
	
	switch (cmd->device_type)
	{
		case device_humax:		remoteSend_HUMAX(cmd); break;
		case device_bein:		remoteSend_BEIN(cmd); break;
		case device_mbc:		remoteSend_HUMAX(cmd); break;
		default:				appUIPrintln(" Error: unknown device type!"); break;
	}
	
	remoteSetDataGPIO(1, 0, 0xFFF);
}


static void remoteSend_HUMAX(const remoteCommand_t *cmd) 
{
	uint32_t keycode = getKeyCode(cmd);
	uint16_t pins = cmd->devices_bm;
	
	/* Transmit code x times */
	//for (uint8_t j = 0; j < BEIN_RETRANSMISSIONS_REDUNDANT; j++)
	//{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			/* Start bit */
			remoteSetDataGPIO(0, 0, pins);
			_delay_us(9000);
			remoteSetDataGPIO(1, 0, pins);
			_delay_us(4500);
		
			/* keycode */
			for (int8_t i = 31; i >= 0; i--) 
			{
				uint8_t bit = (uint8_t)(((uint32_t)keycode >> i) & 1);
			
				if (bit) {
					remoteSetDataGPIO(0, 0, pins);
					_delay_us(562);
					remoteSetDataGPIO(1, 0, pins);
					_delay_us(1687);
				} else {
					remoteSetDataGPIO(0, 0, pins);
					_delay_us(562);
					remoteSetDataGPIO(1, 0, pins);
					_delay_us(562);
				}
			}
		
			/* Stop bit */
			remoteSetDataGPIO(0, 0, pins);
			_delay_us(562);
		
			/* Floating pin */
			remoteSetDataGPIO(1, 1, pins);
		}
		//if (j != BEIN_RETRANSMISSIONS_REDUNDANT - 1) _delay_ms(88);
		_delay_ms(88);
	//}
}



/*	Each button consists of 14 bits, the first two bits are ignored. 
	Transmission starts at MSB. Fifth bit is ignored and inverted every *new* key press. */

static void remoteSend_BEIN(const remoteCommand_t *cmd)
{
	static bool invert = false;
	uint16_t keycode = getKeyCode(cmd) & 0x37FF;
	uint8_t pins = cmd->devices_bm;
	uint8_t transmitTotal = 3;
			
	invert = !invert;
	if (invert) {
		keycode |= 0x0800;
	}
	
	if (cmd->key == volume_up) 
	{
		transmitTotal = 20;
	}
	
	/* Transmit code x times */
	for (uint8_t j = 0; j < transmitTotal; j++)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			/* Transmit bit by bit */
			for (int8_t i = 13; i >= 0; i--) 
			{
				uint8_t bit = ((keycode & (1 << i)) >> i);
		
				if (bit) {
					remoteSetDataGPIO(1, 0, pins);
					_delay_us(887);
					remoteSetDataGPIO(0, 0, pins);
					_delay_us(887);
				} else {
					remoteSetDataGPIO(0, 0, pins);
					_delay_us(887);
					remoteSetDataGPIO(1, 0, pins);
					_delay_us(887);
				}
			}
			
			remoteSetDataGPIO(1, 1, pins);
		}
		if (j != transmitTotal - 1) _delay_ms(88);
	}
}


static void remotePrintCommand(const remoteCommand_t *cmd)
{
	appUIPrint("Remote to %s, 0x%03x, ", (cmd->device_type == device_bein) ? ("bein") : ((cmd->device_type == device_humax) ? ("humax") : ((cmd->device_type == device_mbc) ? ("mbc") : ("unknown"))), cmd->devices_bm);
	
	switch (cmd->key)
	{
		case channel_down:		appUIPrint("channel down");		break;
		case channel_up:		appUIPrint("channel up");		break;
		
		case arrow_up:			appUIPrint("arrow up");			break;
		case arrow_left:		appUIPrint("arrow left");		break;
		case arrow_down:		appUIPrint("arrow down");		break;
		case arrow_right:		appUIPrint("arrow right");		break;
		case confirm:			appUIPrint("confirm");			break;
		case back:				appUIPrint("back");				break;
		case cancel:			appUIPrint("cancel");			break;
		case home:				appUIPrint("home");				break;
		case menu:				appUIPrint("menu");				break;
		case power:				appUIPrint("power");			break;
		case volume_up:			appUIPrint("volume up");		break;
		
		case power_switch:		appUIPrint("force reboot");		break;
		case channel_number:	appUIPrint("set channel %u", cmd->channelNumber);		break;
		//case auto_solver:		appUIPrint("fix everything");	break;
		
		
		
		case red:				appUIPrint("red");			break;
		case green:				appUIPrint("green");		break;
		case yellow:			appUIPrint("yellow");		break;
		case blue:				appUIPrint("blue");			break;
		
		case audio:				appUIPrint("audio");		break;
		case subs:				appUIPrint("subs");			break;
		case favorite:			appUIPrint("favorite");			break;
		case option:			appUIPrint("option");			break;
		case schedule:			appUIPrint("schedule");			break;
		
		case zero:				appUIPrint("0");					break;
		case one:				appUIPrint("1");					break;
		case two:				appUIPrint("2");					break;
		case three:				appUIPrint("3");					break;
		case four:				appUIPrint("4");					break;
		case five:				appUIPrint("5");					break;
		case six:				appUIPrint("6");					break;
		case seven:				appUIPrint("7");					break;
		case eight:				appUIPrint("8");					break;
		case nine:				appUIPrint("9");					break;
		
		default:				appUIPrint("Error: unknown key!");	return;
	}
}


static uint32_t getKeyCode(const remoteCommand_t *cmd)
{
	uint32_t keycode = 0;
	
	if (cmd->device_type == device_humax)
	{
		switch (cmd->key)
		{
			case power:				keycode = 0b10000000000011111111;	break;
			case arrow_up:			keycode = 0b10001000100001110111;	break;
			case arrow_down:		keycode = 0b10001010100001010111;	break;
			case arrow_right:		keycode = 0b10000010100011010111;	break;
			case arrow_left:		keycode = 0b10000100100010110111;	break;
			case confirm:			keycode = 0b10001100100000110111;	break;
			case back:				keycode = 0b10001000001001111101;	break;
			case cancel:			keycode = 0b10000110100010010111;	break;
			//case home:			keycode = 0;						break;
			case menu:				keycode = 0b10000111000010001111;	break;
			case channel_up:		keycode = 0b10000000100011110111;	break;
			case channel_down:		keycode = 0b10001111000000001111;	break;
			case volume_up:			keycode = 0b10001111100000000111;	break;
			
			case audio:				keycode = 0b10001010001001011101;	break;
			case subs:				keycode = 0b10000110001010011101;	break;
			case option:			keycode = 0b10000100001010111101;	break;
			//case favorite:		keycode = 0;	break;
			//case schedule:		keycode = 0;	break;
			
			case red:				keycode = 0b10000011100011000111;	break;
			case green:				keycode = 0b10001011100001000111;	break;
			case yellow:			keycode = 0b10000101100010100111;	break;
			case blue:				keycode = 0b10000111100010000111;	break;
			
			case zero:				keycode = 0b10000011000011001111;	break;
			case one:				keycode = 0b10001100000000111111;	break;
			case two:				keycode = 0b10000010000011011111;	break;
			case three:				keycode = 0b10001010000001011111;	break;
			case four:				keycode = 0b10000110000010011111;	break;
			case five:				keycode = 0b10001110000000011111;	break;
			case six:				keycode = 0b10000001000011101111;	break;
			case seven:				keycode = 0b10001001000001101111;	break;
			case eight:				keycode = 0b10000101000010101111;	break;
			case nine:				keycode = 0b10001101000000101111;	break;
			
			default:				keycode = 0;						break;
		}
	}
	else if (cmd->device_type == device_mbc)
	{
		switch (cmd->key)
		{
			case power:				keycode = 0b10000010111000000100000010111111;	break;
			case arrow_up:			keycode = 0b10000010111000001100100000110111;	break;
			case arrow_down:		keycode = 0b10000010111000001110100000010111;	break;
			case arrow_right:		keycode = 0b10000010111000000110100010010111;	break;
			case arrow_left:		keycode = 0b10000010111000000010100011010111;	break;
			case confirm:			keycode = 0b10000010111000001010100001010111;	break;
			case back:				keycode = 0b10000010111000000100100010110111;	break; // same as cancel
			case cancel:			keycode = 0b10000010111000000100100010110111;	break;
			case home:				keycode = 0b10000010111000001100000000111111;						break;
			case menu:				keycode = 0b10000010111000001111000000001111;	break;
			case channel_up:		keycode = 0b10000010111000001101100000100111;	break;
			case channel_down:		keycode = 0b10000010111000001111100000000111;	break;
			case volume_up:			keycode = 0b10000010111000000001100011100111;	break;
			
			case audio:				keycode = 0;	break;
			case subs:				keycode = 0b10000010111000000000010011111011;	break;
			case option:			keycode = 0;	break;
			case favorite:			keycode = 0b10000010111000000101100010100111;	break;
			//case schedule:		keycode = 0;	break;
			
			case red:				keycode = 0b10000010111000000110010010011011;	break;
			case green:				keycode = 0b10000010111000001110010000011011;	break;
			case yellow:			keycode = 0b10000010111000000001010011101011;	break;
			case blue:				keycode = 0b10000010111000001001010001101011;	break;
			
			case zero:				keycode = 0b10000010111000000111000010001111;	break;
			case one:				keycode = 0b10000010111000000010000011011111;	break;
			case two:				keycode = 0b10000010111000001010000001011111;	break;
			case three:				keycode = 0b10000010111000000110000010011111;	break;
			case four:				keycode = 0b10000010111000001110000000011111;	break;
			case five:				keycode = 0b10000010111000000001000011101111;	break;
			case six:				keycode = 0b10000010111000001001000001101111;	break;
			case seven:				keycode = 0b10000010111000000101000010101111;	break;
			case eight:				keycode = 0b10000010111000001101000000101111;	break;
			case nine:				keycode = 0b10000010111000000011000011001111;	break;
			
			//MUTE 0b10000010111000001000000001111111
			//VOLDOWN 0b10000010111000000011100011000111
			//MEDIA	0b10000010111000001001100001100111
			//INFO	0b10000010111000000000100011110111
			//LIST	0b10000010111000001000100001110111
			default:				keycode = 0;						break;
		}
	}
	else if (cmd->device_type == device_bein)
	{
		switch (cmd->key)
		{
			case power:				keycode = 0b11101010001100;			break; 
			case arrow_up:			keycode = 0b10101010010000;			break; 
			case arrow_down:		keycode = 0b10101010010001;			break; 
			case arrow_right:		keycode = 0b10101010010110;			break; 
			case arrow_left:		keycode = 0b10101010010101;			break; 
			case confirm:			keycode = 0b10101010010111;			break; 
			case back:				keycode = 0b10101010010011;			break; 
			case cancel:			keycode = 0b10101010110101;			break; 
			case home:				keycode = 0b10010101101010;			break;
			case menu:				keycode = 0b10101010010010;			break;
			case channel_up:		keycode = 0b11101010100000;			break;
			case channel_down:		keycode = 0b11101010100001;			break;
			case volume_up:			keycode = 0b11101010010000;			break; // fixed: started with 0b11
			
			//case audio:				keycode = 0;	break;
			//case subs:				keycode = 0;	break;
			case option:			keycode = 0b11101010001110;			break;
			case favorite:			keycode = 0b10101010001100;			break;
			case schedule:			keycode = 0b10101010101000;			break;
			
			case red:				keycode = 0b10101010101011;			break;
			case green:				keycode = 0b10101010101100;			break;
			case yellow:			keycode = 0b10101010101101;			break;
			case blue:				keycode = 0b10101010101110;			break;
			
			case zero:				keycode = 0b11101010000000;			break; 
			case one:				keycode = 0b11101010000001;			break; 
			case two:				keycode = 0b11101010000010;			break; 
			case three:				keycode = 0b11101010000011;			break; 
			case four:				keycode = 0b11101010000100;			break; 
			case five:				keycode = 0b11101010000101;			break; 
			case six:				keycode = 0b11101010000110;			break; 
			case seven:				keycode = 0b11101010000111;			break; 
			case eight:				keycode = 0b11101010001000;			break; 
			case nine:				keycode = 0b11101010001001;			break; 
			
			default:				keycode = 0;						break; 
		}
	}
	return keycode;
}
