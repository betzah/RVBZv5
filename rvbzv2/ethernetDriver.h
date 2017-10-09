/*
 * ethernetDriver.h
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#pragma once

#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>


typedef enum ethernetState
{
	ethernetState_Disabled,
	ethernetState_NoModule,
	ethernetState_NoInternet,
	ethernetState_Connected,
} ethernetState_t;


#define ETH_BUF_SIZE					(700+64)

typedef struct
{
	bool enableInterrupt;
	bool enableEthernet;
	bool connected_temp;
	ethernetState_t state;
	uint8_t ip[4];
	uint8_t mac[6];
	uint32_t pageviews;
	uint8_t buf[ETH_BUF_SIZE];
} ethernetConfig_t;



void ethernetInit(ethernetConfig_t * const ethernet);
void ethernetUpdate();
void ethernetRestart();

