/*
 * remoteInterface.c
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */
/*
#include "remoteInterface.h"
#include "remoteDriver.h"
#include "twiDriver.h"
#include "peripherals.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>


static void twiTranceive();


TWI_Driver_t twiDriver;

#define TWI_ADDRESS 0x37


void twiInterfaceInit(const uint32_t cpu, const uint32_t baud)
{
	twiMasterInit(&twiDriver.master, &TWIE, PORTE, TWI_MASTER_INTLVL_LO_gc, cpu, baud);
	println("TWI Master Initialized.");
	
// 	twiSlaveInit(&twiDriver.slave, &TWIE, PORTE, TWI_MASTER_INTLVL_LO_gc, TWI_ADDRESS);
// 	println("TWI Slave Initialized.");
}


void twiInterfaceProcess()
{
	
	remoteCommand_t cmd =
	{
		.devices_bm = 1 << (device.number & 0x07),
		.channelNumber = 0,
		.device_type = device.type,
		.key = noone,
	};
	
	//TWI
	static uint8_t busy_timeout = 10;
	if (board.isMaster) // Master
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
		{
			if (twiDriver.master.result == TWIM_RESULT_OK)
			{
				// Print received data (debugging)
// 				println("Received: %u { ", (uint8_t) twiDriver.master.bytesRead);
// 				for (uint8_t i = 0; i < twiDriver.master.bytesRead; i++) print("%u ", (uint8_t) twiDriver.master.inputData[i]);
// 				print("}");
				
				// Read board id
				board.idOther = twiDriver.master.inputData[0];
				
				// Mark result as already used
				twiDriver.master.result = TWIM_RESULT_UNKNOWN;
				twiDriver.master.bytesRead = 0;
				twiDriver.master.bytesToRead = 0;
				twiDriver.master.bytesWritten = 0;
				twiDriver.master.bytesToWrite = 0;
				busy_timeout = 10;
			}
			else if (!(--busy_timeout))
			{
				// Reset TWI connection
				busy_timeout = 10;
				twiDriver.master.result = TWIM_RESULT_FAIL;
				twiDriver.master.status = TWI_STATUS_READY;
				
				// If board id was previously correct, change id to error
				if (board.idOther >= BOARDID_FIRST && board.idOther <= BOARDID_LAST) board.idOther = BOARDID_ERROR;
			}
			
			//Send packet
			twiTranceive();
		}
	}
	else // Slave
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) 
		{
			if (twiDriver.slave.result == TWIS_RESULT_OK)
			{
				// Print received data (debugging)
// 				println("Received: %u { ", (uint8_t) twiDriver.slave.bytesReceived);
// 				for (uint8_t i = 0; i < twiDriver.slave.bytesReceived; i++) print("%u ", (uint8_t) twiDriver.slave.inputData[i]);
// 				print("}");
				
				// Read board id
				twiDriver.slave.outputData[0] = board.id;				// Transmit only Slave ID to Master
				board.idOther = twiDriver.slave.inputData[0];
				
				// Read RTC data
				memcpy(&hardware.time, (void *)&twiDriver.slave.inputData[1], sizeof (time_t));
				
				//print("Updating hardware.time: %u", RTCUpdateAll(&hardware.time,	twiDriver.slave.inputData[1], twiDriver.slave.inputData[2], twiDriver.slave.inputData[3],
				//						twiDriver.slave.inputData[4], twiDriver.slave.inputData[5], twiDriver.slave.inputData[6], twiDriver.slave.inputData[7]));
				
				// Read command
				if (twiDriver.slave.inputData[9] >= DEVICES_PER_GROUP && twiDriver.slave.inputData[9] < 2 * DEVICES_PER_GROUP) twiDriver.slave.inputData[9] -= DEVICES_PER_GROUP;
				
				if (twiDriver.slave.inputData[9] < DEVICES_PER_GROUP && twiDriver.slave.inputData[11] != noone && twiDriver.slave.inputData[11] < last)
				{
					cmd.devices_bm		= twiDriver.slave.inputData[8];
					cmd.channelNumber	= twiDriver.slave.inputData[9];
					cmd.device_type		= twiDriver.slave.inputData[10];
					cmd.key				= twiDriver.slave.inputData[11];
					
					NONATOMIC_BLOCK(ATOMIC_RESTORESTATE)
					{
						remoteSendCommand(&cmd);
					}
				}
				
				// Mark result as already used
				twiDriver.slave.result = TWIS_RESULT_UNKNOWN;
				twiDriver.slave.bytesReceived = 0;
				twiDriver.slave.bytesSent = 0;
				busy_timeout = 10;
			}
			else if (!(--busy_timeout))
			{
				// Reset TWI connection
				busy_timeout = 10;
				twiDriver.slave.result = TWIM_RESULT_FAIL;
				twiDriver.slave.status = TWI_STATUS_READY;
				
				// If board id was previously correct, change id to error
				if (board.idOther >= BOARDID_FIRST && board.idOther <= BOARDID_LAST) board.idOther = BOARDID_ERROR;
			}
		}
	}
	
	// Change number of devices
	if (board.idOther >= BOARDID_FIRST && board.idOther <= BOARDID_LAST) {
		device.numberTotal = DEVICES_PER_GROUP * 2;
	}
	else {
		device.numberTotal = DEVICES_PER_GROUP * 1;
	}
	clamp(device.number, 0, device.numberTotal - 1);
	
}


void twiInterfaceSendCommandOtherPCB(remoteCommand_t * cmd)
{
	memcpy((void *) &twiDriver.master.outputData[(1 + sizeof (time_t))], cmd, sizeof (remoteCommand_t));
}


static void twiTranceive()
{
	twiDriver.master.bytesToWrite = 1 + sizeof(time_t) + sizeof(remoteCommand_t);
	
	twiDriver.master.outputData[0] = board.id;
	memcpy((void *) &twiDriver.master.outputData[1], &hardware.time, sizeof(time_t));

	TWI_MASTER_SEND(twiDriver, board.id);
	TWI_MASTER_SEND(twiDriver, hardware.time);
	
	twiMasterWriteRead(&twiDriver.master, TWI_ADDRESS, twiDriver.master.bytesToWrite, twiDriver.master.bytesToWrite);
}


// */