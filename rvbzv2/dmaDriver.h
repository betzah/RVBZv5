/*
 * adcDriver.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#pragma once

#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>



void dmaInit();
void dmaCopyToBufferRepeating(void * src, void * dst);
void dmaCopyToBufferOnce(void * src, void * dst);
void dmaCopy(void * src, void * dst, DMA_CH_TRIGSRC_t trigSrc, DMA_CH_BURSTLEN_t burstlen, uint16_t trnCnt, uint16_t repCnt, DMA_CH_TRNINTLVL_t intLvl);