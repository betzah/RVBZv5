/*
 * adcDriver.cpp
 *
 * Created: 1/1/2015 0:0:0 AM
 *  Author: Mfadl
 */


extern "C"
{
	#include "peripherals.h"
	#include "adcDriver.h"
	//#include "dmaDriver.h"
}

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stddef.h>

static adc_packet_t * packet = NULL;


void adcInit(adc_packet_t * adc_packet) 
{
//  A0-7: PWR_SENSE 0-7
// 	B0: 2.048 V reference
// 	B1: 12V_DIV11_SENSE
// 	B4-7: PWR_SENSE 8-11
// 	K4: PWR_SELECT group A/B
	
	packet = adc_packet;

	NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
	ADCA.CALL = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0));
	ADCA.CALH = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL1));
	ADCB.CALL = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCBCAL0));
	ADCB.CALH = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCBCAL1));
	NVM.CMD = 0;
	
	PORTA.OUTCLR = 0xFF;
	PORTA.DIRCLR = 0xFF;
	
	PORTB.OUTCLR = 0xF3;
	PORTB.DIRCLR = 0xF3;
	
	PORTK.OUTCLR = 0x10;
	PORTK.DIRSET = 0x10;
	
	PORTA.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTA.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTA.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTA.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTA.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTA.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
	
	PORTB.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTB.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTB.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTB.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTB.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTB.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
	
	PORTK.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
	
	ADCA.CH0.INTCTRL = ADC_CH_INTLVL_OFF_gc;
	ADCA.CH1.INTCTRL = ADC_CH_INTLVL_OFF_gc;
	ADCA.CH2.INTCTRL = ADC_CH_INTLVL_OFF_gc;
	ADCA.CH3.INTCTRL = ADC_CH_INTLVL_OFF_gc;
	
	ADCB.CH0.INTCTRL = ADC_CH_INTLVL_OFF_gc;
	ADCB.CH1.INTCTRL = ADC_CH_INTLVL_OFF_gc;
	ADCB.CH2.INTCTRL = ADC_CH_INTLVL_OFF_gc;
	ADCB.CH3.INTCTRL = ADC_CH_INTLVL_OFF_gc;
	
	ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFF_gc;
	ADCA.CH1.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFF_gc;
	ADCA.CH2.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFF_gc;
	ADCA.CH3.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFF_gc;
	
	ADCB.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFF_gc;
	ADCB.CH1.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFF_gc;
	ADCB.CH2.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFF_gc;
	ADCB.CH3.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFF_gc;
	
	ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
	ADCA.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
	ADCA.CH2.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
	ADCA.CH3.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
	
	ADCB.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN8_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
	ADCB.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN8_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
	ADCB.CH2.MUXCTRL = ADC_CH_MUXPOS_PIN8_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
	ADCB.CH3.MUXCTRL = ADC_CH_MUXPOS_PIN8_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
	
	ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc | ADC_CONMODE_bm | ADC_CURRLIMIT_NO_gc| !ADC_FREERUN_bm;
	ADCA.REFCTRL = ADC_REFSEL_AREFB_gc;
	ADCA.EVCTRL = 0; // ADC_SWEEP_0123_gc | ADC_EVSEL_0123_gc | ADC_EVACT_SYNCSWEEP_gc; /* Sweep channels 0-1, trigger using event channel 0, sync sweep on event */
	ADCA.PRESCALER = ADC_PRESCALER_DIV16_gc; // 24 MHz / 16 = 1.5 MSPS
	
	ADCB.CTRLB = ADC_RESOLUTION_12BIT_gc | ADC_CONMODE_bm | ADC_CURRLIMIT_NO_gc | !ADC_FREERUN_bm;
	ADCB.REFCTRL = ADC_REFSEL_AREFB_gc;
	ADCB.EVCTRL = 0; // ADC_SWEEP_0123_gc | ADC_EVSEL_0123_gc | ADC_EVACT_SYNCSWEEP_gc; /* Sweep channels 0-1, trigger using event channel 0, sync sweep on event */
	ADCB.PRESCALER = ADC_PRESCALER_DIV16_gc; // 24 MHz / 16 = 1.5 MSPS
	
	ADCA.CTRLA = ADC_ENABLE_bm;// | ADC_FLUSH_bm;
	ADCB.CTRLA = ADC_ENABLE_bm;// | ADC_FLUSH_bm;
	
//	PMIC.CTRL |=  PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
//	EVSYS.CH0MUX = EVSYS_CHMUX_RTC_OVF_gc; /* Connect RTC overflow to event channel 0, thus triggering an ADC sweep on ADC */
//	EVSYS.CH0MUX = EVSYS_CHMUX_TCE0_OVF_gc; /* Connect TCE0 overflow to event channel 0, thus triggering an ADC sweep on ADC */
}

#define NUMBER_OF_SAMPLES				256
#define NUMBER_OF_ITERATIONS			(NUMBER_OF_SAMPLES / 8)
#define SAMPLE_DIVISION					16
#define CALCULATE_VOLTAGE(INTEGER)		((float)INTEGER * (1 / ((4.096f / 2.048f) * (float) NUMBER_OF_SAMPLES)))


static int16_t adcMeasureSingleChannel(uint8_t adca_pin)
{
	ADCA.CH0.MUXCTRL = adca_pin;
	ADCA.CH1.MUXCTRL = adca_pin;
	ADCA.CH2.MUXCTRL = adca_pin;
	ADCA.CH3.MUXCTRL = adca_pin;
	
	ADCB.CH0.MUXCTRL = adca_pin ^ 0x40;
	ADCB.CH1.MUXCTRL = adca_pin ^ 0x40;
	ADCB.CH2.MUXCTRL = adca_pin ^ 0x40;
	ADCB.CH3.MUXCTRL = adca_pin ^ 0x40;
	
	int32_t sum = 0;
	
	for (uint16_t i = 0; i < NUMBER_OF_ITERATIONS; i++)
	{
		// Start all channels
		ADCA.CTRLA |= ADC_CH0START_bm | ADC_CH1START_bm | ADC_CH2START_bm | ADC_CH3START_bm;
		ADCB.CTRLA |= ADC_CH0START_bm | ADC_CH1START_bm | ADC_CH2START_bm | ADC_CH3START_bm;
		
		// Wait till all channels finish
		while ((ADCB.INTFLAGS & 0x0F) != 0x0F);
		
		// Clear all flags
		ADCA.INTFLAGS = ADC_CH0IF_bm | ADC_CH1IF_bm | ADC_CH2IF_bm | ADC_CH3IF_bm;
		ADCB.INTFLAGS = ADC_CH0IF_bm | ADC_CH1IF_bm | ADC_CH2IF_bm | ADC_CH3IF_bm;
		
		// Read all results
		sum += (int16_t) ADCA.CH0.RES + (int16_t) ADCA.CH1.RES + (int16_t) ADCA.CH2.RES + (int16_t) ADCA.CH3.RES;
		sum += (int16_t) ADCB.CH0.RES + (int16_t) ADCB.CH1.RES + (int16_t) ADCB.CH2.RES + (int16_t) ADCB.CH3.RES;
	}
	
	return (sum / SAMPLE_DIVISION) & 0xFFFF;
}


#define CURRENTSENSE_OFFSET				(9)
#define CURRENTSENSE_GAIN				(1.018f)
#define ADC_MILLIAMPS_MIN				(20)
#define ADC_MILLIAMPS_MAX				(4000)

#define ADC_PSU_DIVIDER						(11.0f)
#define ADC_CURRENTRATIO					(2.250f) // Iklis = 2250 at 0.5A load, 20% accuracy
#define ADC_TO_MILLIVOLT_MULTIPLIER			(2048.0f / 32767.0f)
#define ADC_PSU_TO_MILLIVOLT_MULTIPLIER		(ADC_TO_MILLIVOLT_MULTIPLIER * ADC_PSU_DIVIDER)
#define ADC_TO_MILLIAMPS_MULTIPLIER			(ADC_TO_MILLIVOLT_MULTIPLIER * CURRENTSENSE_GAIN) // * ADC_CURRENTRATIO)

#define ADC_TO_MILLIVOLT(ADC)				((int16_t) ((float) (ADC) * (ADC_TO_MILLIVOLT_MULTIPLIER)))
#define ADC_PSU_TO_MILLIVOLT(ADC)			((int16_t) ((float) (ADC) * (ADC_PSU_TO_MILLIVOLT_MULTIPLIER)))
#define ADC_TO_MILLIAMPS(ADC)				((int16_t) ((float) (ADC) * (ADC_TO_MILLIAMPS_MULTIPLIER)) + (int16_t) CURRENTSENSE_OFFSET)

void adcMeasure()
{
	static int16_t sum = 0;
	static bool isFirstGroup = true;
	int16_t * device;
	uint8_t i = 0;
	
	if (packet == NULL) {
		return;
	}
	
	uint8_t adcPin[13] = {
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN0_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN1_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN2_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN3_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN4_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN5_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN6_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN7_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN12_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN13_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN14_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN15_gc,
		ADC_CH_MUXNEG_GND_MODE3_gc | ADC_CH_MUXPOS_PIN9_gc
	};
	
	if (isFirstGroup)
	{
		device = packet->milliAmps_devB;
	}
	else
	{
		device = packet->milliAmps_devA;
	}
	
	for (i = 0; i < 12; i++)
	{
		int16_t sample = ADC_TO_MILLIAMPS(adcMeasureSingleChannel(adcPin[i]));
		
		if (sample < ADC_MILLIAMPS_MIN && sample > -ADC_MILLIAMPS_MIN) {
			sample = 0;
		}
		else if (sample > ADC_MILLIAMPS_MAX) {
			sample = ADC_MILLIAMPS_MAX;
		}
		else if (sample < -ADC_MILLIAMPS_MAX) {
			sample = -ADC_MILLIAMPS_MAX;
		}
		
		device[i] = sample;
		sum += sample;
	}
	
	packet->milliVolts_psu = ADC_PSU_TO_MILLIVOLT(adcMeasureSingleChannel(adcPin[i]));
	
	if (!isFirstGroup)
	{
		packet->milliAmps_total = sum;
		sum = 0;
	}
	isFirstGroup = !isFirstGroup;
	
	// Settling time after switching sense pin is 20 us max, but RC filter time constant = 215 us => must wait 1-2 ms minimum
	PORTK.DIRSET = 0x10;
	PORTK.OUTTGL = 0x10;
	
	//adcMeasureSingleChannel(&(sample[0][i]), adcPin[i]);
}


void adcPrint()
{
	if (packet == NULL) {
		return;
	}
	
	appUIPrintln("\r\nPSU: %5d mV", packet->milliVolts_psu);
	for (uint8_t i = 0; i < 12; i++)
	{
		appUIPrintln("CH%2dA: %5d mA \tCH%2dB: %5d mA", i, packet->milliAmps_devA[i], i, packet->milliAmps_devB[i]);
		//appUIPrintln("CH%uA: %5u, %.4f V", i, (adc_sample_t)(packet->deviceA).integer, (adc_sample_t)(packet->deviceA).floating);
	}
}

/*
static void adcTransferChannelDMA(uint8_t channel)
{
	DMA_CH_TRIGSRC_t trigSrc;
	void * adcChResPtr;
	
	switch (channel)
	{
		case 0:		adcChResPtr = (void *) &ADCA.CH0RES;	trigSrc = DMA_CH_TRIGSRC_ADCA_CH0_gc;	break;
		case 1:		adcChResPtr = (void *) &ADCA.CH1RES;	trigSrc = DMA_CH_TRIGSRC_ADCA_CH1_gc;	break;
		case 2:		adcChResPtr = (void *) &ADCA.CH2RES;	trigSrc = DMA_CH_TRIGSRC_ADCA_CH2_gc;	break;
		case 3:		adcChResPtr = (void *) &ADCA.CH3RES;	trigSrc = DMA_CH_TRIGSRC_ADCA_CH3_gc;	break;
		default:	adcChResPtr = (void *) 0;				trigSrc = DMA_CH_TRIGSRC_OFF_gc;		break;
	}
	
	dmaCopy(adcChResPtr,
			adc.buf,
			trigSrc,
			DMA_CH_BURSTLEN_2BYTE_gc,
			ADC_BUF_BYTES,
			1,
			DMA_CH_TRNINTLVL_OFF_gc);
}
//*/
