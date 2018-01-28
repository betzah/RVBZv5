/*
 * remoteDriver.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#pragma once

/*
#include <avr/io.h>
#include <stdio.h>

#include "remoteDriver.h"
#include "rtcDriver.h"
#include "twiDriver.h"


typedef struct
{
	uint8_t board_id;
	remoteCommand_t remoteCommand;
	datetime_t time;
} twiCom_t;


void twiInterfaceInit(const uint32_t cpu, const uint32_t baud);
void twiInterfaceProcess();
void twiInterfaceSendCommandOtherPCB(remoteCommand_t * cmd);

// */