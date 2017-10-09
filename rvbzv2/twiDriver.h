/*
 * twiDriver.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#pragma once

#include "avr_compiler.h"

#include <stdio.h>
#include <stdbool.h>

/*! Baud register setting calculation. Formula described in datasheet. */
#define TWI_BAUD(F_SYS, F_TWI)		( (F_SYS / (2 * F_TWI)) - 5)
//#define TWI_BAUD(F_SYS, F_TWI)			( (TWI_BAUD_FORMULA(F_SYS, F_TWI) > 255) ? 255 : (TWI_BAUD_FORMULA(F_SYS, F_TWI) < 0) ? 0 : (TWI_BAUD_FORMULA(F_SYS, F_TWI))))

/*! Transaction status defines. */
#define TWI_STATUS_READY              0
#define TWI_STATUS_BUSY               1

/*! Transaction result enumeration. */
typedef enum TWIM_RESULT_enum {
	TWIM_RESULT_UNKNOWN          = (0x00<<0),
	TWIM_RESULT_OK               = (0x01<<0),
	TWIM_RESULT_BUFFER_OVERFLOW  = (0x02<<0),
	TWIM_RESULT_ARBITRATION_LOST = (0x03<<0),
	TWIM_RESULT_BUS_ERROR        = (0x04<<0),
	TWIM_RESULT_NACK_RECEIVED    = (0x05<<0),
	TWIM_RESULT_FAIL             = (0x06<<0),
} TWIM_RESULT_t;


/* Transaction result enumeration */
typedef enum TWIS_RESULT_enum {
	TWIS_RESULT_UNKNOWN            = (0x00<<0),
	TWIS_RESULT_OK                 = (0x01<<0),
	TWIS_RESULT_BUFFER_OVERFLOW    = (0x02<<0),
	TWIS_RESULT_TRANSMIT_COLLISION = (0x03<<0),
	TWIS_RESULT_BUS_ERROR          = (0x04<<0),
	TWIS_RESULT_FAIL               = (0x05<<0),
	TWIS_RESULT_ABORTED            = (0x06<<0),
} TWIS_RESULT_t;


/*! Buffer size defines */
#define TWI_OUTPUT_BUFSIZE		32
#define TWI_INPUT_BUFSIZE		32


typedef struct // 22 bytes + bufsize
{
	TWI_t * twi;								/*!< Pointer to what interface to use */
	PORT_t * port;
	bool isMaster;
	
	void (*transmissionFinished) (void);            /*!< Pointer to process data function*/
	register8_t outputData[TWI_OUTPUT_BUFSIZE];		/*!< Data to write */
	register8_t inputData[TWI_INPUT_BUFSIZE];			/*!< Read data */
	register8_t status;                             /*!< Status of transaction*/
	register8_t result;                             /*!< Result of transaction*/
	
	// Master only
	register8_t address;                            /*!< Slave address */
	register8_t bytesToWrite;                       /*!< Number of bytes to write */
	register8_t bytesToRead;                        /*!< Number of bytes to read */
	register8_t bytesWritten;                       /*!< Number of bytes written */
	register8_t bytesRead;                          /*!< Number of bytes read */
	
	// Slave only
	register8_t bytesReceived;                      /*!< Number of bytes received*/
	register8_t bytesSent;                          /*!< Number of bytes sent*/
} twi_driver_t;


// #define TWI_MASTER_SEND(_twiDriver, _data)			do { memcpy(&(_twiDriver).master.outputData[_twiDriver.master.bytesToWrite], (&_data), sizeof (typeof (_data))); (_twiDriver).master.bytesToWrite += sizeof (typeof (_data)); } while (0);
// #define TWI_MASTER_RECEIVE(_twiDriver, _data)		do { } while (0);

void twiInit(twi_driver_t * twi, TWI_t * twiModule, PORT_t * port, bool isMaster, uint8_t address, void (*transmissionFinished)(), TWI_MASTER_INTLVL_t intlvl, const uint32_t cpu, const uint32_t baud);
void twiMasterWriteRead(uint8_t address, uint8_t bytesToWrite, uint8_t bytesToRead);
