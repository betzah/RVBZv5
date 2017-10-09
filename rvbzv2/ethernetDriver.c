/*
 * ethernetDriver.cpp
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */

#include "ethernetDriver.h"
#include "timeEvent.h"
#include "peripherals.h"

#include "enc28j60/ip_arp_udp_tcp.h"
#include "enc28j60/enc28j60.h"
#include "enc28j60/net.h"
#include "LUFA/Drivers/USB/USB.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#define ETH_INTERRUPT_ENABLE()			do { PORTB.INTCTRL = (PORTB.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc; PORTB.INT0MASK = (1 << 3); PORTB.PIN3CTRL = PORT_ISC_BOTHEDGES_gc | PORT_SRLEN_bm | PORT_OPC_PULLUP_gc; } while (0);
#define ETH_INTERRUPT_DISABLE()			do { PORTB.INTCTRL = (PORTB.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_OFF_gc; } while (0);
#define ETH_PORT_WWW					80


static ethernetConfig_t * eth;


void ethernetUpdate()
{
	if (!eth->enableEthernet) return;
	
	uint8_t packetCounter = 0;  
	
	while (++packetCounter < 10)
	{	
		// get the next new packet:
		uint16_t dat_p = 0, plen = enc28j60PacketReceive(ETH_BUF_SIZE-1, eth->buf);

		// exit if no packet received
		if(!plen) return;
		
		// update state: we know we are connected now
		eth->state = ethernetState_Connected;
		eth->connected_temp = true;
		
		// arp is broadcast if unknown but a host may also
		// verify the mac address by sending it to a unicast address.
		if(eth_type_is_arp_and_my_ip(eth->buf,plen)) {
			make_arp_answer_from_request(eth->buf);
			//appUIPrintln("Ethernet: ARP request");
			continue;
		}
		// if ip packet not for us: exit
		if(eth_type_is_ip_and_my_ip(eth->buf,plen) == 0) {
			continue;
		}
		
		// if ICMP packet: send answer
		if(eth->buf[IP_PROTO_P]==IP_PROTO_ICMP_V && eth->buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V) {
			make_echo_reply_from_request(eth->buf,plen);
			//appUIPrintln("Ethernet: ICMP request");
			continue;
		}
	
		// if UDP packet
		/*if (eth->buf[IP_PROTO_P]==IP_PROTO_UDP_V){
			payloadlen=eth->buf[UDP_LEN_L_P]-UDP_HEADER_LEN;
			// the received command has to start with t or be 4 char long
			// e.g "test\0"
			if (eth->buf[UDP_DATA_P]=='t' || payloadlen==5){
				make_udp_reply_from_request(eth->buf,(char *)"hello",6,ETH_PORT_UDP); ////////// remove char *
				appUIPrintln("Ethernet: UDP packet");
			}
		} // */
	
		// if TCP packet and port 80
		if (eth->buf[IP_PROTO_P] == IP_PROTO_TCP_V && eth->buf[TCP_DST_PORT_H_P] == 0 && eth->buf[TCP_DST_PORT_L_P] == ETH_PORT_WWW)
		{
			// if SYN flag: first handshake packet
			if (eth->buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V)
			{
				make_tcp_synack_from_syn(eth->buf); // sends syn,ack
				//appUIPrintln("Ethernet: Syn Packet Acknowledged");
				continue;
			}
			
			// if ACK flag
			if (eth->buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V)
			{
				init_len_info(eth->buf); // init some data structures
				// we can possibly have no data, just ack:
				dat_p = get_tcp_data_pointer();
				if (dat_p==0)
				{
					if (eth->buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V)
					{
						//appUIPrintln("Ethernet: FIN Packet Acknowledged");
						make_tcp_ack_from_any(eth->buf); // finack, answer with ack
					} 
					//else appUIPrintln("Ethernet: ACK Packet without data");
					// just an ack with no data, wait for next packet
					continue;
				}
				//appUIPrintln("Ethernet: ACK Packet with data");
			
  				uint8_t * website = (uint8_t *) appUIGetBufWebsite();
			 
				memcpy(&website[0], &eth->buf[0], TCP_CHECKSUM_L_P + 3);
				plen = appUIGetBufWebsiteLength();
				
				make_tcp_ack_from_any(website);			// send ack for http get
				make_tcp_ack_with_data(website, plen);	// send data
				eth->pageviews++;
				continue;
			}
		}
	}
}



void ethernetInit(ethernetConfig_t * const ethernet)
{
	eth = ethernet;
	
	ethernetRestart();
}


void ethernetRestart()
{
	if (eth == NULL) return;
	
	ETH_INTERRUPT_DISABLE();
	enc28j60Init(eth->mac);
	enc28j60PhyWrite(PHLCON, 0x21A);	//0x21: Green: RX, Yellow: TX
	//enc28j60PhyWrite(PHLCON, 0xAA4);	//Blink both fast, max brightness
	
	init_ip_arp_udp_tcp(eth->mac, eth->ip, ETH_PORT_WWW);
	memset(&eth->buf, 0, ETH_BUF_SIZE);
	
	if (eth->enableEthernet)
	{
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON2, ECON2_PWRSV);
		_delay_ms(1); //Increased from 300us; polling CLKRDY is not safe
		//while(!enc28j60Read(ESTAT) & ESTAT_CLKRDY);
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
		
		if (enc28j60getrev() == 0)
		{
			eth->state = ethernetState_NoModule;
		}
		else
		{
			if (eth->enableInterrupt)
			{
				ETH_INTERRUPT_ENABLE();
			}
			if (eth->connected_temp)
			{
				// To Do: Send a ICMP package to someone ...
				eth->connected_temp = false;
			}
			else
			{
				eth->state = ethernetState_NoInternet;
			}
		}
	}
	else // If disabled
	{
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_RXEN);
		while(enc28j60Read(ESTAT) & ESTAT_RXBUSY);
		while(enc28j60Read(ECON1) & ECON1_TXRTS);
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_VRPS);
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PWRSV);
		
		eth->state = ethernetState_Disabled;
	}
}


ISR(PORTB_INT0_vect)
{
	eventTimerTrigger(&ethernetUpdate);
}



