/*
 * peripherals.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */ 

#pragma once

/* Include Atmel Libraries */
#include <avr/io.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <util/crc16.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <inttypes.h>
#include <stdarg.h>

/* Include USB LUFA Library */
#include "LUFA/Descriptors.h"
#include "LUFA/Drivers/USB/USB.h"
#include "LUFA/Platform/Platform.h"

/* Include Custom Libraries */
#include "LUFA/Misc/TerminalCodes.h"
#include "clksys_driver.h"
#include "remoteDriver.h"
#include "twiDriver.h"
#include "rtcDriver.h"
#include "appUI.h"
#include "timeEvent.h"
#include "adcDriver.h"
#include "menu.h"

/* Variable Clamping and limiting macro's */
#define MAX2(a,b) 						({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define MIN2(a,b) 						({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _b : _a; })
#define isClamped(a,b,c) 				({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); __typeof__ (c) _c = (c); _a < _b ? 0 : (_a > _c ? 0 : 1); })
#define clamp(a,b,c) 					({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); __typeof__ (c) _c = (c); a = _a < _b ? _b : (_a > _c ? _c : _a); })

#define JTAG_FORCE_DISABLE()		CCPWrite(&MCU.MCUCR, MCU_JTAGD_bm)
#define WDT_RESET()					__asm__ __volatile__("wdr");

//#define USB_UPDATE()				do { CDC_Device_USBTask(&USBSerialClass); USB_USBTask(); } while (0);
//#define USB_CLEAR_BUF()				while (CDC_Device_BytesReceived(&USBSerialClass)) { CDC_Device_ReceiveByte(&USBSerialClass); }

/* EEPROM constants */
#define EEPROM_LOAD				0
#define EEPROM_SAVE				1
#define EEPROM_FACTORYRESET		2

#define EEPROM_DO(ramPtr, eepPtr, action) \
	do { \
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { \
			__typeof__ (ramPtr) _ramPtr = (ramPtr); \
			__typeof__ (eepPtr) _eepPtr = (eepPtr); \
			__typeof__ (action) _action = (action); \
			if ((_action) == EEPROM_LOAD) { \
				eeprom_read_block((void *) _ramPtr, (const void *) _eepPtr, sizeof(*_eepPtr)); \
			} \
			else if ((_action) == EEPROM_SAVE) { \
				eeprom_update_block((const void *) _ramPtr, (void *) _eepPtr, sizeof(*_eepPtr)); \
			} \
		} \
	} while(0); // EEPROM_DO()
	
	
#define EEPROM_DO_CLAMP(ramPtr, eepPtr, defVal, minVal, maxVal, action) \
	do { \
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { \
			__typeof__ (ramPtr) _ramPtr = (ramPtr); \
			__typeof__ (eepPtr) _eepPtr = (eepPtr); \
			__typeof__ (defVal) _defVal = (defVal); \
			__typeof__ (minVal) _minVal = (minVal); \
			__typeof__ (maxVal) _maxVal = (maxVal); \
			__typeof__ (action) _action = (action); \
			if ((action) == EEPROM_FACTORYRESET) { \
				*_ramPtr = _defVal; \
				eeprom_update_block((const void *) _ramPtr, (void *) _eepPtr, sizeof(*_eepPtr)); \
			} else { \
				if ((_action) == EEPROM_LOAD) { \
					eeprom_read_block((void *) _ramPtr, (const void *) _eepPtr, sizeof(*_eepPtr)); \
					if (!isClamped(*_ramPtr, _minVal, _maxVal)) { \
						*_ramPtr = _defVal; \
					} \
				} \
				else if ((_action) == EEPROM_SAVE) { \
					if (!isClamped(*_ramPtr, _minVal, _maxVal)) { \
						*_ramPtr = _defVal; \
					} \
					eeprom_update_block((const void *) _ramPtr, (void *) _eepPtr, sizeof(*_eepPtr)); \
				} \
			} \
		} \
	} while(0); // EEPROM_DO_CLAMP()


typedef enum
{
	BOARDID_UNINITIALIZED = 0,
	BOARDID_FIRST = 1,
	BOARDID_LAST = 253,
	BOARDID_ERROR = 254,
	BOARDID_UNKNOWN = 255,
}board_id_t;


#define DEVICENAME_MAXDEVICES			12
#define DEVICENAME_MAXCHARACTERS		8

typedef struct 
{
	uint32_t xmegaID;
	board_id_t id;
	uint8_t wdtCrashes;
	float temperature;
} board_t;


typedef struct
{
	char name[10];
	deviceTypes type;
	uint16_t ch;
} deviceConfig_t;


extern FILE USBStream;
extern USB_ClassInfo_CDC_Device_t USBSerialClass;
extern uint8_t EEMEM wdtCrashesEEPROM;

typedef struct 
{
	datetime_t datetime, alarm;
	board_t board;
	adc_packet_t adc;
	cpu_t cpu;
	twi_driver_t twi;
	device_t device;
	deviceConfig_t devConfig[12];
	bool alarmEnabled;
} hardware_t;

extern hardware_t hardware;

void TMP112Read();
void RX8900Read();

void ledController();

void ledRedEnable();
void ledRedDisable();
bool ledRedGet();

void ledGreenEnable();
void ledGreenDisable();
bool ledGreenGet();

void ledBlueEnable();
void ledBlueDisable();
bool ledBlueGet();

char * deviceNameGet(uint8_t deviceNumber);

void EVENT_USB_Device_Connect();
void EVENT_USB_Device_Disconnect();
void EVENT_USB_Device_Suspend();
void EVENT_USB_Device_WakeUp();
void EVENT_USB_Device_Reset();
void EVENT_USB_Device_ConfigurationChanged();
void EVENT_USB_Device_ControlRequest();

/* Declare header functions */
void initHardware();
void softwareReset();
int16_t freeRam ();

