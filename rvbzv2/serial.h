/*
 * serial.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */


#pragma once

#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>


void serialInit(PORT_t * port, USART_t * usart, bool isHighNibble, const uint32_t cpu, const uint32_t baud);
void serialSetBaud(USART_t * usart, const uint32_t cpu, const uint32_t baud); //800 cpu cycle

uint8_t serialCanRead();
uint8_t serialReadByte();

uint8_t serialCanWrite();
void serialWriteByte(uint8_t data);
void serialWriteString(const uint8_t *data);

void serialWriteClearBuffer();
void serialReadClearBuffer();
void serialWaitForFinish();
