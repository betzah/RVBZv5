/*
 * twiDriver.c
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#include "twiDriver.h"
//#include "peripherals.h"

#include <stdio.h>


#include <stdio.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>

static twi_driver_t * twi_driver = NULL;

static void twiInterruptHandler();
static void twiMasterWriteHandler();
static void twiMasterReadHandler();
static void twiSlaveWriteHandler();
static void twiSlaveReadHandler();



void twiInit(twi_driver_t * twi, TWI_t * twiModule, PORT_t * port, bool isMaster, uint8_t address, void (*transmissionFinished)(), TWI_MASTER_INTLVL_t intlvl, const uint32_t cpu, const uint32_t baud)
{
	port->PIN0CTRL = /*PORT_SRLEN_bm |*/ PORT_OPC_WIREDANDPULL_gc;
	port->PIN1CTRL = /*PORT_SRLEN_bm |*/ PORT_OPC_WIREDANDPULL_gc;
	port->OUTSET = PIN0_bm | PIN1_bm;
	port->DIRSET = PIN0_bm | PIN1_bm;
	
	//memset(twi_driver, 0, sizeof (twi_driver_t));
	twi_driver = twi;
	twi_driver->port = port;
	twi_driver->isMaster = isMaster;
	twi_driver->transmissionFinished = transmissionFinished;
	twi_driver->twi = twiModule;
	
	if (twi_driver->isMaster)
	{
		twi_driver->twi->CTRL = 0x00;
		twi_driver->twi->MASTER.BAUD = (uint16_t)(TWI_BAUD(cpu, baud));
		twi_driver->twi->MASTER.CTRLA = intlvl | TWI_MASTER_RIEN_bm | TWI_MASTER_WIEN_bm | TWI_MASTER_ENABLE_bm;
		twi_driver->twi->MASTER.CTRLB = 0;
		twi_driver->twi->MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
	}
	else
	{
		twi_driver->bytesReceived = 0;
		twi_driver->bytesSent = 0;
		twi_driver->status = TWI_STATUS_READY;
		twi_driver->result = TWIS_RESULT_UNKNOWN;
		twi_driver->twi->SLAVE.CTRLA = intlvl | TWI_SLAVE_DIEN_bm | TWI_SLAVE_APIEN_bm | TWI_SLAVE_ENABLE_bm;
		twi_driver->twi->SLAVE.ADDR = (address << 1);
	}
}


static void twiInterruptHandler()
{
	if (twi_driver->isMaster)
	{
		uint8_t status = twi_driver->twi->MASTER.STATUS;
	
		if (status & TWI_MASTER_BUSERR_bm) {
			twi_driver->result = TWIM_RESULT_BUS_ERROR;
			twi_driver->status = TWI_STATUS_READY;
			twi_driver->twi->MASTER.STATUS = status | TWI_MASTER_BUSERR_bm;
		}
	
		else if (status & TWI_MASTER_ARBLOST_bm) {
			twi_driver->result = TWIM_RESULT_ARBITRATION_LOST;
			twi_driver->status = TWI_STATUS_READY;
			twi_driver->twi->MASTER.STATUS = status | TWI_MASTER_ARBLOST_bm;
		}

		/* If master write interrupt. */
		else if (status & TWI_MASTER_WIF_bm) {
			twiMasterWriteHandler();
		}

		/* If master read interrupt. */
		else if (status & TWI_MASTER_RIF_bm) {
			twiMasterReadHandler();
		}
	
		/* If unexpected state. */
		else {
			twi_driver->result = TWIM_RESULT_FAIL;
			twi_driver->status = TWI_STATUS_READY;
		}
	}
	else
	{
		uint8_t status = twi_driver->twi->SLAVE.STATUS;

		/* If bus error. */
		if (status & TWI_SLAVE_BUSERR_bm) {
			twi_driver->bytesReceived = 0;
			twi_driver->bytesSent = 0;
			twi_driver->result = TWIS_RESULT_BUS_ERROR;
			twi_driver->status = TWI_STATUS_READY;
		}

		/* If transmit collision. */
		else if (status & TWI_SLAVE_COLL_bm) {
			twi_driver->bytesReceived = 0;
			twi_driver->bytesSent = 0;
			twi_driver->result = TWIS_RESULT_TRANSMIT_COLLISION;
			twi_driver->status = TWI_STATUS_READY;
		}

		/* If address match. */
		else if ((status & TWI_SLAVE_APIF_bm) && (status & TWI_SLAVE_AP_bm)) {
			twi_driver->status = TWI_STATUS_BUSY;
			twi_driver->result = TWIS_RESULT_UNKNOWN;

			/* Disable stop interrupt. */
			uint8_t currentCtrlA = twi_driver->twi->SLAVE.CTRLA;
			twi_driver->twi->SLAVE.CTRLA = currentCtrlA & ~TWI_SLAVE_PIEN_bm;

			//twi_driver->bytesReceived = 0;
			twi_driver->bytesSent = 0;

			/* Send ACK, wait for data interrupt. */
			twi_driver->twi->SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
		}

		/* If stop (only enabled through slave read transaction). */
		else if (status & TWI_SLAVE_APIF_bm) {
			uint8_t currentCtrlA = twi_driver->twi->SLAVE.CTRLA;
			twi_driver->twi->SLAVE.CTRLA = currentCtrlA & ~TWI_SLAVE_PIEN_bm;
			
			/* Clear APIF, according to flowchart don't ACK or NACK */
			uint8_t status = twi_driver->twi->SLAVE.STATUS;
			twi_driver->twi->SLAVE.STATUS = status | TWI_SLAVE_APIF_bm;
			
			twi_driver->result = TWIS_RESULT_OK;
			twi_driver->status = TWI_STATUS_READY;
			
			if (twi_driver->transmissionFinished != NULL) {
				twi_driver->transmissionFinished();
			}
		}

		/* If data interrupt. */
		else if (status & TWI_SLAVE_DIF_bm) {
			if (twi_driver->twi->SLAVE.STATUS & TWI_SLAVE_DIR_bm) {
				twiSlaveWriteHandler();
			} else {
				twiSlaveReadHandler();
			}
		}

		/* If unexpected state. */
		else {
			twi_driver->result = TWIS_RESULT_FAIL;
			twi_driver->status = TWI_STATUS_READY;
		}
	}
}


void twiMasterWriteRead(uint8_t address, /*uint8_t *outputData,*/ uint8_t bytesToWrite, uint8_t bytesToRead)
{
	if (twi_driver->status != TWI_STATUS_READY) {
		return;
	}

	twi_driver->status = TWI_STATUS_BUSY;
	twi_driver->result = TWIM_RESULT_UNKNOWN;
	twi_driver->address = address<<1;
	
	twi_driver->bytesToWrite = bytesToWrite;
	twi_driver->bytesToRead = bytesToRead;
	twi_driver->bytesWritten = 0;
	twi_driver->bytesRead = 0;
	
	/* Fill write data buffer. */
// 	for (uint8_t i=0; i < bytesToWrite; i++) {
// 		twi_driver->outputData[i] = outputData[i];
// 	}
	
	/* If write command, send the START condition + Address + 'R/_W = 0' */
	if (bytesToWrite) {
		uint8_t writeAddress = twi_driver->address & ~0x01;
		twi_driver->twi->MASTER.ADDR = writeAddress;
	}

	/* If read command, send the START condition + Address + 'R/_W = 1' */
	else if (bytesToRead) {
		uint8_t readAddress = twi_driver->address | 0x01;
		twi_driver->twi->MASTER.ADDR = readAddress;
	}
}


static void twiMasterWriteHandler()
{
	/* Local variables used in if tests to avoid compiler warning. */
	uint8_t bytesToWrite  = twi_driver->bytesToWrite;
	uint8_t bytesToRead   = twi_driver->bytesToRead;

	/* If NOT acknowledged (NACK) by slave cancel the transaction. */
	if (twi_driver->twi->MASTER.STATUS & TWI_MASTER_RXACK_bm) {
		twi_driver->twi->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
		twi_driver->result = TWIM_RESULT_NACK_RECEIVED;
		twi_driver->status = TWI_STATUS_READY;
	}

	/* If more bytes to write, send data. */
	else if (twi_driver->bytesWritten < bytesToWrite) {
		twi_driver->twi->MASTER.DATA = twi_driver->outputData[twi_driver->bytesWritten++];
		//twi_driver->outputData[twi_driver->bytesWritten] = 0;	// clear data that is already sent
	}

	/* If bytes to read, send repeated START condition + Address + 'R/_W = 1' */
	else if (twi_driver->bytesRead < bytesToRead) {
		uint8_t readAddress = twi_driver->address | 0x01;
		twi_driver->twi->MASTER.ADDR = readAddress;
	}

	/* If transaction finished, send STOP condition and set RESULT OK. */
	else {
		twi_driver->twi->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
		twi_driver->result = TWIM_RESULT_OK;
		twi_driver->status = TWI_STATUS_READY;
		
		if (twi_driver->transmissionFinished != NULL) {
			twi_driver->transmissionFinished();
		}
	}
}


static void twiMasterReadHandler()
{
	/* Fetch data if bytes to be read. */
	if (twi_driver->bytesRead < TWI_INPUT_BUFSIZE) {
		twi_driver->inputData[twi_driver->bytesRead++] = twi_driver->twi->MASTER.DATA;
	}

	/* If buffer overflow, issue STOP and BUFFER_OVERFLOW condition. */
	else {
		twi_driver->twi->MASTER.CTRLC = TWI_MASTER_CMD_STOP_gc;
		twi_driver->result = TWIM_RESULT_BUFFER_OVERFLOW;
		twi_driver->status = TWI_STATUS_READY;
	}

	/* Local variable used in if test to avoid compiler warning. */
	uint8_t bytesToRead = twi_driver->bytesToRead;

	/* If more bytes to read, issue ACK and start a byte read. */
	if (twi_driver->bytesRead < bytesToRead) {
		twi_driver->twi->MASTER.CTRLC = TWI_MASTER_CMD_RECVTRANS_gc;
	}

	/* If transaction finished, issue NACK and STOP condition. */
	else {
		twi_driver->twi->MASTER.CTRLC = TWI_MASTER_ACKACT_bm | TWI_MASTER_CMD_STOP_gc;
		twi_driver->result = TWIM_RESULT_OK;
		twi_driver->status = TWI_STATUS_READY;
		
		if (twi_driver->transmissionFinished != NULL) {
			twi_driver->transmissionFinished();
		}
	}
}


static void twiSlaveReadHandler()
{
	/* Enable stop interrupt. */
	uint8_t currentCtrlA = twi_driver->twi->SLAVE.CTRLA;
	twi_driver->twi->SLAVE.CTRLA = currentCtrlA | TWI_SLAVE_PIEN_bm;

	/* If free space in buffer. */
	if (twi_driver->bytesReceived < TWI_INPUT_BUFSIZE) {
		/* Fetch data */
		twi_driver->inputData[twi_driver->bytesReceived++] = twi_driver->twi->SLAVE.DATA;
		twi_driver->twi->SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
	}
	/* If buffer overflow, send NACK and wait for next START. Set
	 * result buffer overflow.
	 */
	else {
		twi_driver->twi->SLAVE.CTRLB = TWI_SLAVE_ACKACT_bm |
		                              TWI_SLAVE_CMD_COMPTRANS_gc;
		twi_driver->result = TWIS_RESULT_BUFFER_OVERFLOW;
		twi_driver->status = TWI_STATUS_READY;
	}
}


/*! \brief TWI slave write interrupt handler.
 *
 *  Handles TWI slave write transactions and responses.
 *
 *  \param twi The TWI_Slave_t struct instance.
 */
static void twiSlaveWriteHandler()
{
	/* If NACK, slave write transaction finished. */
	if ((twi_driver->bytesSent > 0) && (twi_driver->twi->SLAVE.STATUS & TWI_SLAVE_RXACK_bm)) {
		twi_driver->twi->SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
		twi_driver->result = TWIS_RESULT_OK;
		twi_driver->status = TWI_STATUS_READY;
		
		if (twi_driver->transmissionFinished != NULL) {
			twi_driver->transmissionFinished();
		}
	}
	/* If ACK, master expects more data. */
	else {
		if (twi_driver->bytesSent < TWI_OUTPUT_BUFSIZE) {
			twi_driver->twi->SLAVE.DATA = twi_driver->outputData[twi_driver->bytesSent++];

			/* Send data, wait for data interrupt. */
			twi_driver->twi->SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
		}
		/* If buffer overflow. */
		else {
			twi_driver->twi->SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
			twi_driver->result = TWIS_RESULT_BUFFER_OVERFLOW;
			twi_driver->status = TWI_STATUS_READY;
		}
	}
}


#ifdef TWIC_TWIM_vect
ISR(TWIC_TWIM_vect) {
	twiInterruptHandler();
}
#endif

#ifdef TWIC_TWIS_vect
ISR(TWIC_TWIS_vect) {
	twiInterruptHandler();
}
#endif



#ifdef TWID_TWIM_vect
ISR(TWID_TWIM_vect) {
	twiInterruptHandler();
}
#endif

#ifdef TWID_TWIS_vect
ISR(TWID_TWIS_vect) {
	twiInterruptHandler();
}
#endif


#ifdef TWIE_TWIM_vect
ISR(TWIE_TWIM_vect) {
	twiInterruptHandler();
}
#endif


#ifdef TWIE_TWIS_vect
ISR(TWIE_TWIS_vect) {
	twiInterruptHandler();
}
#endif
