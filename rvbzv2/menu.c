/*
 * menu.c
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#include "menu.h"
#include "peripherals.h"
#include "timeEvent.h"
#include "rtcDriver.h"
#include "twiDriver.h"
#include "remoteDriver.h"
#include "adcDriver.h"
#include "dmaDriver.h"


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

const __flash char strNoone[]					= "noone";
const __flash char strMenuMain[]				= "main";
const __flash char strMenuSettings[]			= "settings";
const __flash char strMenuRemote[]				= "remote";
const __flash char strDeviceHumax[]				= "humax";
const __flash char strDeviceBein[]				= "bein";
const __flash char strDeviceMbc[]				= "mbc";
const __flash char * const __flash strMenu[]	= { strNoone, strMenuMain, strMenuSettings, strMenuRemote };
const __flash char * const __flash strDevice[]	= { strNoone, strDeviceHumax, strDeviceBein, strDeviceMbc };

typedef struct
{
	char buf[32];
	uint8_t charNumber;
} textInput_t;


typedef enum ROOMS_enum
{
	ROOM_NULL,
	ROOM_MAIN,
	ROOM_SETTINGS,
	ROOM_REMOTE,
} ROOMS_t;


bool EEMEM alarmEnabledEEPROM = 0;
uint8_t EEMEM deviceTypeEEPROM;
time_t EEMEM alarmEEPROM, timeEEPROM;
static ROOMS_t room;
static textInput_t textInput;

static everythingData_t everythingData;

static void roomSet(ROOMS_t newRoom);

static void roomKeysMenuMain(uint8_t key);
static void roomKeysMenuSettings(uint8_t key);
static void roomKeysMenuRemote(uint8_t key);

static void roomPrintMenuMain();
static void roomPrintMenuSettings();
static void roomPrintMenuRemote();

static bool roomReadInputText(char key);
static void alarmPowerOnCallback();

void roomInit()
{
	roomReset();
	
	hardware.alarmEnabled = false;
	
	EEPROM_DO_CLAMP(&hardware.device.type, &deviceTypeEEPROM, 1, 1, 2, EEPROM_LOAD);
	EEPROM_DO_CLAMP(&hardware.alarmEnabled, &alarmEnabledEEPROM, hardware.alarmEnabled, 0, 1, EEPROM_LOAD);
	EEPROM_DO(&hardware.alarm, &alarmEEPROM, EEPROM_LOAD);
	EEPROM_DO(&hardware.time, &timeEEPROM, EEPROM_LOAD);
}


void roomClean()
{
	if (room != ROOM_SETTINGS) {
		memset(&textInput, 0, sizeof textInput);
	}
	
//	Clean Buffers
	while (CDC_Device_BytesReceived(&USBSerialClass)) 
	{
		CDC_Device_ReceiveByte(&USBSerialClass); 
	}
// 	RingBuffer_InitBuffer(&input, inputData, sizeof inputData);
// 	memset(&inputData, 0, sizeof inputData);
	
//	Clean & refresh screen
	appUIClean();
	roomPrintDeviceStuff();
	roomPrintMenus();
	psuVoltageController();
}


static void roomSet(ROOMS_t newRoom)
{
	room = newRoom;
	
	roomClean();
}

// Executed after few minutes of inactivity
void roomReset()
{
	roomSet(ROOM_MAIN);
}


/////////////////////////////// Room USB Key Presses ///////////////////////////////


void roomKeys()
{
//	Check for USB input
	if (!CDC_Device_BytesReceived(&USBSerialClass))	return;

//	Read USB input
	char key = tolower(CDC_Device_ReceiveByte(&USBSerialClass));
	
//	Clear buffer
	while (CDC_Device_BytesReceived(&USBSerialClass))
	{
		CDC_Device_ReceiveByte(&USBSerialClass);
// 		if (!RingBuffer_IsFull(&input)) RingBuffer_Insert(&input, key);
	}
	
//	Check if byte is available
// 	if (RingBuffer_IsEmpty(&input))	return;

//	Read byte
//	key = RingBuffer_Remove(&input);
	
	eventTimerTrigger(&roomPrintDeviceStuff);
	eventTimerTrigger(&roomPrintMenus);
	eventTimerTrigger(&psuVoltageController);
	eventTimerRestart(&roomReset);
	eventTimerRestart(&roomClean);
	
	eventSetTimeleft(&alarmCancelButton, MAX2((uint32_t)1000 * 300, eventGetInterval(&alarmCancelButton)));
	//eventSetTimeleft(&alarmSetChannel, MAX2((uint32_t)1000 * 300, eventGetInterval()));
	
	switch (key)
	{
		case 27:	roomSet(ROOM_MAIN); return; //Escape
		//case 8: 
		//case 127:	roomClean(); break;
	}
	
	switch(room)
	{
		case ROOM_MAIN:			roomKeysMenuMain(key);		break;
		case ROOM_SETTINGS:		roomKeysMenuSettings(key);	break;
		case ROOM_REMOTE:		roomKeysMenuRemote(key);	break;
		default: break;
	}
}



static void roomKeysMenuMain(uint8_t key)
{
	switch (key)
	{
		case 's':	roomSet(ROOM_SETTINGS); return;
		case ' ':	roomSet(ROOM_REMOTE);	return;
	}
}


static void roomKeysMenuSettings(uint8_t key)
{
	uint8_t newIP[4];
	time_t newTime;
	memset(&newIP, 0, sizeof newIP);
	memset(&newTime, 0, sizeof newTime);
	
	if (roomReadInputText(key) == true)
	{
		if (strstr_P(textInput.buf, PSTR("bein")) != NULL)
		{
			hardware.device.type = device_bein;
			appUIPrintln("Device type set to Bein");
		}
		
		else if (strstr_P(textInput.buf, PSTR("humax")) != NULL)
		{
			hardware.device.type = device_humax;
			appUIPrintln("Device type set to Humax");
		}
		
		else if (strstr_P(textInput.buf, PSTR("mbc")) != NULL)
		{
			hardware.device.type = device_mbc;
			appUIPrintln("Device type set to MBC");
		}
		
		else if (strstr_P(textInput.buf, PSTR("reset")) != NULL || strstr_P(textInput.buf, PSTR("restart")) != NULL || strstr_P(textInput.buf, PSTR("reboot")) != NULL)
		{
			softwareReset();
		}
		
		else if (strstr_P(textInput.buf, PSTR("alarm")) != NULL)
		{
			if (strstr_P(textInput.buf, PSTR("on")) != NULL)
			{
				hardware.alarmEnabled = true;
				eeprom_update_byte((uint8_t *) &alarmEnabledEEPROM, (uint8_t) hardware.alarmEnabled);
				appUIPrintln("Alarm enabled");
			
				if (RTCIsSurpassed(&hardware.time, &hardware.alarm))
				{
					RTCUpdateDate(&hardware.alarm, hardware.time.year, hardware.time.month, hardware.time.day);
					RTCAddDate(&hardware.alarm, 0, 0, 1);
				}
			}
			else if (strstr_P(textInput.buf, PSTR("off")) != NULL)
			{
				hardware.alarmEnabled = false;
				eeprom_update_byte((uint8_t *) &alarmEnabledEEPROM, (uint8_t) hardware.alarmEnabled);
				appUIPrintln("Alarm disabled");
			}
		}
		
		
		// Update Date
		else if (sscanf_P(textInput.buf, PSTR("d%*[^0-9]%hhu:%hhu:%hhu"), (uint8_t *) &newTime.year, (uint8_t *) &newTime.month, (uint8_t *) &newTime.day) == 3)
		{
			if (RTCUpdateDate(&hardware.time, newTime.year, newTime.month, newTime.day) == true)
			{
				writeTimeToEE();
				RTCUpdateDate(&hardware.alarm, newTime.year, newTime.month, newTime.day);
				appUIPrintln("Succesfully updated date to: ");
				RTCPrintDate(&newTime);
				
				if (RTCIsSurpassed(&hardware.time, &hardware.alarm))
				{
					RTCUpdateDate(&hardware.alarm, hardware.time.year, hardware.time.month, hardware.time.day);
					RTCAddDate(&hardware.alarm, 0, 0, 1);
				} // */
			}
			else appUIPrintln("Failed updating date: invalid date!");
		}
		
		// Update Time
		else if (sscanf_P(textInput.buf, PSTR("t%*[^0-9]%hhu:%hhu:%hhu"), (uint8_t *) &newTime.hour, (uint8_t *) &newTime.minute, (uint8_t *) &newTime.second) == 3)
		{
			if (RTCUpdateTime(&hardware.time, newTime.hour, newTime.minute, newTime.second) == true)
			{
				writeTimeToEE();
				appUIPrintln("Succesfully updated time to: ");
				RTCPrintTime(&newTime);
				
				if (RTCIsSurpassed(&hardware.time, &hardware.alarm))
				{
					RTCUpdateDate(&hardware.alarm, hardware.time.year, hardware.time.month, hardware.time.day);
					RTCAddDate(&hardware.alarm, 0, 0, 1);
				} // */
			}
			else appUIPrintln("Failed updating time: invalid time!");
		} 
		
		// Update Alarm
		else if (sscanf_P(textInput.buf, PSTR("a%*[^0-9]%hhu:%hhu:%hhu"), (uint8_t *) &newTime.hour, (uint8_t *) &newTime.minute, (uint8_t *) &newTime.second) == 3)
		{
			if (RTCUpdateAll(&hardware.alarm, hardware.time.year, hardware.time.month, hardware.time.day, newTime.hour, newTime.minute, newTime.second) == true)
			{
				writeTimeToEE();
				appUIPrintln("Succesfully updated alarm to: ");
				RTCPrintAll(&newTime);
				
				if (RTCIsSurpassed(&hardware.time, &hardware.alarm))
				{
					RTCUpdateDate(&hardware.alarm, hardware.time.year, hardware.time.month, hardware.time.day);
					RTCAddDate(&hardware.alarm, 0, 0, 1);
				}
			}
			else appUIPrintln("Failed updating alarm: invalid time!");
		}
		
		else appUIPrintln("Failed reading text input: invalid input format!");
		
		memset(&textInput, 0, sizeof textInput);
	}
}



static void roomKeysMenuRemote(uint8_t key)
{
	remoteCommand_t cmd =
	{
		.devices_bm = 1 << hardware.device.number,
		.channelNumber = 0,
		.device_type = hardware.device.type,
		.key = noone,
	};

//	Check for key press to fill in command struct
	switch (key)
	{
		case 'q':	if (hardware.device.number > 0)									hardware.device.number--;		return;
		case 'e':	if (hardware.device.number < hardware.device.numberTotal - 1)	hardware.device.number++;		return;
		
		case 'x':	cmd.key = power_switch;		cmd.devices_bm = hardware.device.numberTotal_bm;		break;
		case 'f':	cmd.key = power_switch;		break;
		
		case 'z':	cmd.key = channel_number;	cmd.devices_bm = hardware.device.numberTotal_bm;		break;
		case 'g':	cmd.key = channel_number;	break;
		
		case 'v':	cmd.key = volume_up;		cmd.devices_bm = hardware.device.numberTotal_bm;		break;
		case 'p':	cmd.key = power;			break;
		case 'o':	cmd.key = confirm;			break;
		case 'b':	cmd.key = back;				break;
		case 'c':	cmd.key = cancel;			break;
		case 'h':	cmd.key = home;				break;
		case 'm':	cmd.key = menu;				break;
		case ',':	cmd.key = channel_down;		break;
		case '.':	cmd.key = channel_up;		break;
		case 'w':	cmd.key = arrow_up;			break;
		case 'a':	cmd.key = arrow_left;		break;
		case 's':	cmd.key = arrow_down;		break;
		case 'd':	cmd.key = arrow_right;		break;
		
		case 'y':	cmd.key = favorite;			break;
		case 'n':	cmd.key = option;			break;
		case 'l':	cmd.key = schedule;			break;
		
		case 'u':	cmd.key = red;				break;
		case 'i':	cmd.key = green;			break;
		case 'j':	cmd.key = yellow;			break;
		case 'k':	cmd.key = blue;				break;
		
		case '0':	cmd.key = zero;				break;
		case '1':	cmd.key = one;				break;
		case '2':	cmd.key = two;				break;
		case '3':	cmd.key = three;			break;
		case '4':	cmd.key = four;				break;
		case '5':	cmd.key = five;				break;
		case '6':	cmd.key = six;				break;
		case '7':	cmd.key = seven;			break;
		case '8':	cmd.key = eight;			break;
		case '9':	cmd.key = nine;				break;
		
		default:	return;						break;
	}

	remoteSendCommand(&cmd);
}


/////////////////////////////// Room Screen Printing ///////////////////////////////

void roomPrintMenus()
{
	appUISetUI(APPUI_MENUS);
	
	switch(room)
	{
		case ROOM_MAIN:			roomPrintMenuMain();			break;
		case ROOM_SETTINGS:		roomPrintMenuSettings();		break;
		case ROOM_REMOTE:		roomPrintMenuRemote();			break;
		default:				appUIPrintln("Rooms Error!");	break;
	}
	
	appUISetUI(APPUI_INFO);
}



static void roomPrintMenuMain()
{
	appUIPrintPos(LINE_MENUS + 0, COLUMN_1,			"Press Space to send remote command");
	appUIPrintPos(LINE_MENUS + 1, COLUMN_1,			"Press S to modify settings");
}


static void roomPrintMenuSettings()
{
	appUIPrintPos(LINE_MENUS + 0, COLUMN_1,			"Build: " __TIMESTAMP__);
	appUIPrintPos(LINE_MENUS + 0, COLUMN_2,			"WDT: %u, RAM: %d, ID: 0x%08lx", hardware.board.wdtCrashes, freeRam(), hardware.board.xmegaID);
	
	appUIPrintPos(LINE_MENUS + 2, COLUMN_1,			"date y:m:d");
	appUIPrintPos(LINE_MENUS + 3, COLUMN_1,			"time h:m:s");
	appUIPrintPos(LINE_MENUS + 4, COLUMN_1,			"alarm h:m:s");
	appUIPrintPos(LINE_MENUS + 5, COLUMN_1,			"ip a.b.c.d");

	appUIPrintPos(LINE_MENUS + 2, COLUMN_2,			"reset / restart / reboot");
	appUIPrintPos(LINE_MENUS + 3, COLUMN_2,			"set bein, set humax, set mbc");
	appUIPrintPos(LINE_MENUS + 4, COLUMN_2,			"alarm on, alarm off");
	
	appUIPrintPos(LINE_MENUS + 7, COLUMN_1,			"Data: '%s'", textInput.buf);
}


static void roomPrintMenuRemote()
{
	appUIPrintPos(LINE_MENUS + 0, COLUMN_1,			"WASD: arrow keys");
	appUIPrintPos(LINE_MENUS + 0, COLUMN_2,			"0-9: digits");
	
	appUIPrintPos(LINE_MENUS + 1, COLUMN_1,			"',' / '.': channel prev/next");
	appUIPrintPos(LINE_MENUS + 1, COLUMN_2,			"P: power button");
	
	appUIPrintPos(LINE_MENUS + 2, COLUMN_1,			"O: ok");
	appUIPrintPos(LINE_MENUS + 2, COLUMN_2,			"B: back");
	
	appUIPrintPos(LINE_MENUS + 3, COLUMN_1,			"C: cancel");
	appUIPrintPos(LINE_MENUS + 3, COLUMN_2,			"H: home");
	
	appUIPrintPos(LINE_MENUS + 4, COLUMN_1,			"M: menu");
	appUIPrintPos(LINE_MENUS + 4, COLUMN_2,			"V: volume");
	
	appUIPrintPos(LINE_MENUS + 5, COLUMN_1,			"U,I,J,K: red, green, yellow, blue");
	appUIPrintPos(LINE_MENUS + 5, COLUMN_2,			"Y,N,L: favorite, option, schedule");
	
	appUIPrintPos(LINE_MENUS + 6, COLUMN_1,			"G: set channel");
	appUIPrintPos(LINE_MENUS + 6, COLUMN_2,			"F: force reboot");
	
	appUIPrintPos(LINE_MENUS + 7, COLUMN_1,			"Z: set channel all devices");
	appUIPrintPos(LINE_MENUS + 7, COLUMN_2,			"X: force reboot all devices");
}


void roomPrintDeviceStuff()
{
	char strBuf[20];
	
	appUICleanWebsite();
	appUISetUI(APPUI_DEVICES);
	
	strcpy_P(strBuf, strMenu[room]);
	appUIPrintPos(LINE_DEVICES + 0, COLUMN_1,	"Menu: %s", strBuf);
	
	strcpy_P(strBuf, strDevice[hardware.device.type]);
	appUIPrintPos(LINE_DEVICES + 1, COLUMN_1,	"Device: %s, %2u / %2u", strBuf, hardware.device.number + 1, hardware.device.numberTotal);
	appUIPrint(" (%s)", deviceNameGet(hardware.device.number));
	
	appUIPrintPos(LINE_DEVICES + 2, COLUMN_1,	"Board: %u, CPU: %.1f%%, Temp: %.1f C", hardware.board.id, hardware.cpu.usage, hardware.temperature);
	
	appUIPrintPos(LINE_DEVICES + 3, COLUMN_1,	"PSU: %5d mV, Load: %5d mA", hardware.adc.milliVolts_psu, hardware.adc.milliAmps_total);
	
	appUIPrintPos(LINE_DEVICES + 4, COLUMN_1,	"Time:  ");
	RTCPrintAll(&hardware.time);
	
	appUIPrintPos(LINE_DEVICES + 5, COLUMN_1,	"Alarm: ");
	RTCPrintTime(&hardware.alarm);
	if (hardware.alarmEnabled) {	appUIPrint("(on)"); }
	else { 							appUIPrint("(off)"); }
	
	for (uint8_t i = 0; i < 3; i++)
	{
//		appUIPrintPos(LINE_DEVICES + i, COLUMN_2,	"Dev %2u-%2u:%5d,%5d,%5u,%5d mA", i * 2 + 1, i * 2 + 2, hardware.adc.milliAmps_devA[i * 2], hardware.adc.milliAmps_devB[i * 2], hardware.adc.milliAmps_devA[i * 2 + 1], hardware.adc.milliAmps_devB[i * 2 + 1]);
		appUIPrintPos(LINE_DEVICES + i, COLUMN_2,		"Dev%2uA-%2uA:%5d,%5d,%5u,%5d mA", i * 4 + 1, i * 4 + 4, hardware.adc.milliAmps_devA[i * 4], hardware.adc.milliAmps_devA[i * 4 + 1], hardware.adc.milliAmps_devA[i * 4 + 2], hardware.adc.milliAmps_devA[i * 4 + 3]);
		appUIPrintPos(LINE_DEVICES + i + 3, COLUMN_2,	"Dev%2uB-%2uB:%5d,%5d,%5u,%5d mA", i * 4 + 1, i * 4 + 4, hardware.adc.milliAmps_devB[i * 4], hardware.adc.milliAmps_devB[i * 4 + 1], hardware.adc.milliAmps_devB[i * 4 + 2], hardware.adc.milliAmps_devB[i * 4 + 3]);
	}
	
	appUISetUI(APPUI_INFO);
}

#define LENGTH_REMAINING(len)			(EVERYTHINGDATA_MAXLENGTH - (len))

void printAllDataRawCommaSeperated(FILE * stream) // 105 bytes
{
	if (everythingData.busy) {
		return;
	}
	
	uint16_t len = 0;
	// Timestamp must always be the first one
	len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%s,"), __TIMESTAMP__);
	
	// Hardware Struct, 81 bytes binary
	{
		// Time
		len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%u,%u,%u,%u,%u,%u,"), hardware.time.year, hardware.time.month, hardware.time.day, hardware.time.hour, hardware.time.minute, hardware.time.second);
	
		// Alarm
		len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%u,%u,%u,%u,%u,%u,"), hardware.alarm.year, hardware.alarm.month, hardware.alarm.day, hardware.alarm.hour, hardware.alarm.minute, hardware.alarm.second);
	
		// Board
		len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%lu,%u,%u"), hardware.board.xmegaID, hardware.board.id, hardware.board.wdtCrashes);
	
		// ADC
		for (uint8_t i = 0; i < 12; i++)
			len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%d,"), hardware.adc.milliAmps_devA[i]);
		for (uint8_t i = 0; i < 12; i++)
			len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%d,"), hardware.adc.milliAmps_devB[i]);
		len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%d,%d,"), hardware.adc.milliAmps_total, hardware.adc.milliVolts_psu);
	
		// CPU
		len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%u,%u,%u,%.2f,"), hardware.cpu.total, hardware.cpu.on, hardware.cpu.off, hardware.cpu.usage);
	
		// TWI driver data is completely useless
	
		// Temperature
		len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%.2f,"), hardware.temperature);
	}
	len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%d,"), freeRam());
	
	uint8_t portsData[] = { PORTA.DIR, PORTB.DIR, PORTC.DIR, PORTD.DIR, PORTE.DIR, PORTF.DIR, PORTH.DIR, PORTJ.DIR, PORTK.DIR, PORTQ.DIR, PORTR.DIR, 
							PORTA.OUT, PORTB.OUT, PORTC.OUT, PORTD.OUT, PORTE.OUT, PORTF.OUT, PORTH.OUT, PORTJ.OUT, PORTK.OUT, PORTQ.OUT, PORTR.OUT, };
	
	for (uint8_t i = 0; i < sizeof (portsData); i++) {
		len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("%u,"), portsData[i]);
	}
	len += snprintf_P(&everythingData.str[len], LENGTH_REMAINING(len), PSTR("\r\n"));
	
	everythingData.busy = true;
	everythingData.len = len;
	
	// Start DMA controller
	//dmaCopy(everythingData.str, USARTC0.DATA, DMA_CH_TRIGSRC_USARTC0_DRE_gc, DMA_CH_BURSTLEN_1BYTE_gc, 16, 1, DMA_CH_TRNINTLVL_OFF_gc);
}


/////////////////////////////// Boring Functions ///////////////////////////////


void writeTimeToEE()
{
	EEPROM_DO(&hardware.alarm, &alarmEEPROM, EEPROM_SAVE);
	EEPROM_DO(&hardware.time, &timeEEPROM, EEPROM_SAVE);
	appUIPrintln("Alarm & Time Saved to EEPROM.");
}



/*
void alarmSetChannel()
{
	// Function only valid for DRN bein 7 and 9
	if (hardware.board.id != 2) {
		//eventRemove(&alarmSetChannel);
		return;
	}
	
	remoteCommand_t cmd =
	{
		.device_type = device_bein,
		.key = cancel,
	};
	
	cmd.devices_bm = 0x08;
	cmd.key = nine;
	remoteSendCommand(&cmd);
	
	cmd.devices_bm = 0x10;
	cmd.key = seven;
	remoteSendCommand(&cmd);
} //  */


void alarmCancelButton()
{
	remoteCommand_t cmd =
	{
		.devices_bm = hardware.device.numberTotal_bm,
		.channelNumber = 0,
		//.device_type = device_humax,
		.key = cancel,
	};
	
	// Ugly Hack: Dont send any button to DRN fourth (bein 9) and fifth device (bein 7).
	//if (hardware.board.id == 2) { // if location = DRN
	//	cmd.devices_bm &= ~0x18;
	//}
	
	cmd.device_type = device_humax;
	remoteSendCommand(&cmd);
//	remoteSendCommand(&cmd);
	cmd.device_type = device_bein;
	remoteSendCommand(&cmd);
//	remoteSendCommand(&cmd);
	cmd.device_type = device_mbc;
	remoteSendCommand(&cmd);
//	remoteSendCommand(&cmd);
	
	
	// Ugly Hack: send both cancel and back button for Humax; Cancel-only is untested
	/*if (cmd.device_type == device_humax)
	{
		cmd.key = back;
		cmd.device_type = device_bein;
		remoteSendCommand(&cmd);
		cmd.device_type = device_humax;
		remoteSendCommand(&cmd);
	} //*/
}


void alarmCheck()
{
	// Check for alarm
	if (hardware.alarmEnabled && RTCIsSurpassed(&hardware.time, &hardware.alarm))
	{
		RTCUpdateDate(&hardware.alarm, hardware.time.year, hardware.time.month, hardware.time.day);
		RTCAddAll(&hardware.alarm, 0, 0, 0, 12, 0, 0);
	
		//Start callback timer
		eventRemove(&alarmPowerOnCallback);
		if (eventAdd(2000, 2 * hardware.device.numberTotal, &alarmPowerOnCallback) == false) {
			appUIPrintln("Error: cannot add reboot callback! Reboot aborted.");
		}
	}
	
	// Hardcoded channel up down alarm: 1x per day
	/*
	static time_t timePrev;
	time_t alarmTime = hardware.time; 
	RTCUpdateTime(&alarmTime, 3, 0, 0, 0);
		
	if (RTCIsSurpassed(hardware.time, alarmTime)
	*/
}


static void alarmPowerOnCallback()
{
	if (eventFindCount(&alarmPowerOnCallback) != 1) 
	{
		eventRemove(&alarmPowerOnCallback);
		appUIPrintln("Error detected in Alarm Power Callback!");
		return;
	}
	
	
	int8_t count = eventGetRepeation(&alarmPowerOnCallback);
	if (count == hardware.device.numberTotal * 2) appUIPrintln("Alarm reboot started!");
	
	
	remoteCommand_t cmd =
	{
		.devices_bm = 1 << ((2 * hardware.device.numberTotal - count) % hardware.device.numberTotal), 
		.channelNumber = 0,
		.device_type = hardware.device.type,
		.key = power,
	};
	
	cmd.device_type = device_humax;
	remoteSendCommand(&cmd);
	
	cmd.device_type = device_bein;
	remoteSendCommand(&cmd);
	
	cmd.device_type = device_mbc;
	remoteSendCommand(&cmd);
	
	if (count == 1) appUIPrintln("Alarm reboot finished!");
}


#define COUNTER_THRESHOLD					6

#define POWER_ON_VOLTAGE_NOMINAL			12.0f

#define POWER_OFF_VOLTAGE_THRESHOLD			1.0f
#define UNDER_VOLTAGE_THRESHOLD				9.0f
#define OVER_VOLTAGE_THRESHOLD				15.0f
#define OVER_TEMP_THRESHOLD					50.0f


void psuVoltageController()
{
	/*
	static uint16_t overTempCounterCritical = 0;
	static uint16_t overVoltageCounterCritical = 0;
	static uint16_t underVoltageCounterCritical = 0;
	
	//bool error = false;
	
	appUISetUI(APPUI_PSU);
	printpos(LINE_PSU, COLUMN_1, "");
	
	// Critical errors
	if (fTempSensor > OVER_TEMP_THRESHOLD)
	{
		if (++overTempCounterCritical > COUNTER_THRESHOLD)
		{
			appUIPrintln("!!! WARNING: temperature is dangerously high !!!");
			//error = true;
		}
	} else if (overTempCounterCritical) overTempCounterCritical--;
	
	
	if (fPsuVoltage > OVER_VOLTAGE_THRESHOLD)
	{
		if (++overVoltageCounterCritical > COUNTER_THRESHOLD)
		{
			appUIPrintln("!!! WARNING: PSU voltage is dangerously high !!!");
			//error = true;
		}
	} else if (overVoltageCounterCritical) overVoltageCounterCritical--;
	

	if (fPsuVoltage < UNDER_VOLTAGE_THRESHOLD)
	{
		if (fPsuVoltage > POWER_OFF_VOLTAGE_THRESHOLD)
		{
			if (++underVoltageCounterCritical > COUNTER_THRESHOLD)
			{
				appUIPrintln("!!! WARNING: PSU voltage is dangerously low !!!");
				//error = true;
			}
		}
		else
		{
			appUIPrintln("!!! WARNING: PSU seems to be turned off !!!");
			//error = true;
			
			if (underVoltageCounterCritical) underVoltageCounterCritical--;
		}
	} else if (underVoltageCounterCritical) underVoltageCounterCritical--;
	
	//if (error == true) print("\a"); //Bell sound
	
	appUISetUI(APPUI_INFO);
	//return error;
	*/
}


/////////////////////////////// Even More Boring Abstraction Functionality ///////////////////////////////


static bool roomReadInputText(char key)
{
	//while (CDC_Device_BytesReceived(&USBSerialClass))
	//{
	//	key = CDC_Device_ReceiveByte(&USBSerialClass);

	if (key == 8 || key == 127)
	{
		if (textInput.charNumber > 0)
		{
			textInput.buf[--textInput.charNumber] = 0;
		}
	}
	else if (key == 13) // newline
	{
		textInput.charNumber = 0;
		return true;
	}
	else if (key >= 32 && key <= 127) { 
		if (textInput.charNumber < sizeof textInput.buf - 1) {
			textInput.buf[textInput.charNumber++] = key;
		}
		else {
			memset(&textInput, 0, sizeof textInput);
			appUIPrintln("Error: text input overflow!");
		}
	}
	return false;
}
