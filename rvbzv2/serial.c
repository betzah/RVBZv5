/*
 * serial.cpp
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#include "serial.h"
#include "peripherals.h"

#include <stdio.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#define PD_UART_C0_RXD_CTRL			PIN2_bm
#define PD_UART_C0_TXD_CTRL			PIN3_bm

#define TX_BUF_SIZE	250
#define RX_BUF_SIZE	250

static int Ctrl_putchar(char c, FILE *stream);

FILE serialFileIO = FDEV_SETUP_STREAM(Ctrl_putchar, NULL, _FDEV_SETUP_RW);

static volatile uint8_t tx_write_index, tx_read_index, tx_buffer[TX_BUF_SIZE];
static volatile uint8_t rx_write_index, rx_read_index, rx_buffer[RX_BUF_SIZE];

void serialInit(PORT_t * port, USART_t * usart, bool isHighNibble, const uint32_t cpu, const uint32_t baud)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (isHighNibble) 
		{
			// 			port.PIN6CTRL = PORT_OPC_PULLDOWN_gc; // ATMEGA-TXD
			// 			port.PIN7CTRL = PORT_OPC_PULLDOWN_gc; // ATMEGA-RXD
			port->DIRCLR = PIN6_bm;
			port->DIRSET = PIN7_bm;
			port->OUTSET = PIN6_bm | PIN7_bm;
		}
		else
		{
			// 			port.PIN2CTRL = PORT_OPC_PULLDOWN_gc; // ATMEGA-TXD
			// 			port.PIN3CTRL = PORT_OPC_PULLDOWN_gc; // ATMEGA-RXD
			port->DIRCLR = PIN2_bm;
			port->DIRSET = PIN3_bm;
			port->OUTSET = PIN2_bm | PIN3_bm;
		}
		
 		usart->CTRLB = USART_RXEN_bm | USART_TXEN_bm; // | USART_CLK2X_bm;
		usart->CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_OFF_gc;
	
		PMIC.CTRL |= PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
		
		serialSetBaud(usart, cpu, baud);
	}
}



void serialWaitForFinish() 
{
	while (tx_write_index != tx_read_index && rx_write_index != rx_read_index);
}


void serialSetBaud(USART_t * usart, const uint32_t cpu, const uint32_t baud) 
{
	uint32_t div1k;
	uint16_t bsel;
	uint8_t bscale = 0;

	//if (baud > (cpu>>4)) return 0;

	div1k = ((cpu<<6) / baud) - 1024;
	while ((div1k < 2096640) && (bscale < 7)) {
		bscale++;
		div1k <<= 1;
	}
	
	bsel = div1k >> 10;
	
	usart->BAUDCTRLA = bsel & 0xff;
	usart->BAUDCTRLB = (bsel>>8) | ((16-bscale) << 4);

	//return 1;
}


uint8_t serialCanRead() 
{
	uint8_t wridx = rx_write_index, rdidx = rx_read_index;
	
	if(wridx >= rdidx)
		return wridx - rdidx;
	else
		return wridx - rdidx + RX_BUF_SIZE;
	
}


uint8_t serialReadByte() 
{
	uint8_t res;
	
	if (serialCanRead()) 
	{
		res = rx_buffer[rx_read_index];
		
		if(rx_read_index + 1 >= RX_BUF_SIZE) {
			rx_read_index = 0;
		} else {
			rx_read_index = rx_read_index + 1;
		}
	} else {
		res = '?'; //Error sign
	}
	
	return res;
}


uint8_t serialCanWrite() 
{
	uint8_t wridx1 = tx_write_index + 1, rdidx = tx_read_index;
	
	if(wridx1 >= TX_BUF_SIZE)
		wridx1 -= TX_BUF_SIZE;
	if(rdidx >= wridx1)
		return rdidx - wridx1;
	else
		return rdidx - wridx1 + TX_BUF_SIZE;
	
}


void serialWriteString(const uint8_t *data) 
{
	uint8_t cur, i;
	
	//length = strlen(data);
	cur = tx_write_index;
	
	for (i=0; data[i] != '\0'; i++) {
		tx_buffer[cur] = data[i];
		if (++cur >= TX_BUF_SIZE) {
			cur = 0;
		}
	}
	
	tx_write_index = cur;
}

void serialWriteByte(uint8_t data) {
	uint8_t curSlot, nextSlot, savePMIC;
	
	/* Busy-wait for a byte to be available. Should not be necessary if the caller calls CanWrite_xxx() first */
	while(!serialCanWrite()) {
		USARTC0.CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_LO_gc;
	}
	
	curSlot = tx_write_index;
	tx_buffer[curSlot] = data;
	
	nextSlot = curSlot + 1;
	if(nextSlot >= TX_BUF_SIZE)
		nextSlot = 0;

	savePMIC = PMIC.CTRL;
	PMIC.CTRL = savePMIC & ~PMIC_LOLVLEN_bm;
	tx_write_index = nextSlot;
	USARTC0.CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_LO_gc;
	PMIC.CTRL = savePMIC;

}


static int Ctrl_putchar(char c, FILE *stream) {
	
	serialWriteByte((uint8_t) c);
	
	return 0;
	
}


/* resets buffer by resetting the index */
void serialReadClearBuffer(void)
{
	rx_read_index = 0;
	rx_write_index = 0;
}


/* resets buffer by resetting the index */
void serialWriteClearBuffer(void) {
	tx_read_index = 0;
	tx_write_index = 0;
}

// 
// 
// ISR(USARTC0_RXC_vect) {
// 	
// 	uint8_t curSlot, nextSlot;
// 	
// 	curSlot = rx_write_index;
// 	rx_buffer[curSlot] = USARTC0.DATA;
// 	
// 	nextSlot = curSlot + 1;
// 	if(nextSlot >= RX_BUF_SIZE - 1)
// 		nextSlot = 0;
// 		
// 	if(nextSlot != rx_read_index)
// 		rx_write_index = nextSlot;
// 	
// }
// 
// 
// ISR(USARTC0_DRE_vect) {
// 	
// 	uint8_t curSlot, nextSlot, lastSlot;
// 	
// 	nextSlot = curSlot = tx_read_index;
// 	lastSlot = tx_write_index;
// 	
// 	if(curSlot != lastSlot) {
// 		USARTC0.DATA = tx_buffer[curSlot];
// 		nextSlot = curSlot + 1;
// 		if(nextSlot >= TX_BUF_SIZE)
// 			nextSlot = 0;
// 	}
// 	if(nextSlot == lastSlot)
// 		USARTC0.CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_OFF_gc;
// 	
// 	tx_read_index = nextSlot;
// }


