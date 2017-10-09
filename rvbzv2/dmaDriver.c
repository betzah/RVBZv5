/*
 * adcDriver.cpp
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#include "peripherals.h"
#include "adcDriver.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stddef.h>

//static DMA_CH_t dmaGetFreeChannel();


void dmaInit()
{
	//Reset whole DMA
	DMA.CTRL = 0;
	DMA.CTRL = DMA_RESET_bm;
	while (DMA.CTRL & DMA_RESET_bm);
	
	DMA.CTRL = DMA_ENABLE_bm | DMA_DBUFMODE_DISABLED_gc | DMA_PRIMODE_RR0123_gc;
}



void dmaCopyToBufferRepeating(void * src, void * dst)
{
	
}


void dmaCopyToBufferOnce(void * src, void * dst)
{
	
}

// DMA_CH_TRIGSRC_ADCA_CH4_gc, DMA_CH_BURSTLEN_2BYTE_gc, DMA_CH_TRNINTLVL_OFF_gc
void dmaCopy(void * src, void * dst, DMA_CH_TRIGSRC_t trigSrc, DMA_CH_BURSTLEN_t burstlen, uint16_t trnCnt, uint16_t repCnt, DMA_CH_TRNINTLVL_t intLvl)
{
	if (!(DMA.CTRL & DMA_ENABLE_bm))
	{
		dmaInit();
	}
	
	//DMA_CH_t * DMA_CH = &DMA.CH0; //dmaGetFreeChannel();
	
	//Reset only channel
	DMA.CH0.CTRLA = 0;
	DMA.CH0.CTRLA = DMA_CH_RESET_bm;
	while (DMA.CH0.CTRLA & DMA_CH_RESET_bm);
	
	DMA.CH0.SRCADDR0 = (((uint16_t) src) >> 0) & 0xFF;
	DMA.CH0.SRCADDR1 = (((uint16_t) src) >> 8) & 0xFF;
	DMA.CH0.SRCADDR2 = 0;
	
	DMA.CH0.DESTADDR0 = (((uint16_t) dst) >> 0) & 0xFF;
	DMA.CH0.DESTADDR1 = (((uint16_t) dst) >> 8) & 0xFF;
	DMA.CH0.DESTADDR2 = 0;
	
	DMA.CH0.CTRLA = (DMA_CH_REPEAT_bm) | (DMA_CH_SINGLE_bm) | burstlen;
	DMA.CH0.CTRLB = intLvl;
	DMA.CH0.ADDRCTRL = DMA_CH_SRCRELOAD_BURST_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTRELOAD_TRANSACTION_gc | DMA_CH_DESTDIR_INC_gc;
	DMA.CH0.TRIGSRC = trigSrc;		
	DMA.CH0.TRFCNT = trnCnt;		// number of bytes per block transfer
	DMA.CH0.REPCNT = repCnt;		// number of block transfers per transaction
	
	DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm;
}




/*
static DMA_CH_t dmaGetFreeChannel()
{
	while (true)
	{
		if (!(DMA.CH0.CTRLA & DMA_CH_ENABLE_bm)) {
			return DMA.CH0;
		}
		if (!(DMA.CH1.CTRLA & DMA_CH_ENABLE_bm)) {
			return DMA.CH1;
		}
		if (!(DMA.CH2.CTRLA & DMA_CH_ENABLE_bm)) {
			return DMA.CH2;
		}
		if (!(DMA.CH3.CTRLA & DMA_CH_ENABLE_bm)) {
			return DMA.CH3;
		}
	}
}// */

