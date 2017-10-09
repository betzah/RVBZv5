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
#include <stddef.h>


#define ADC_FILTER_BITS				0
#define ADC_FILTER_DIVIDER			(1 << ADC_FILTER_BITS)

#define ADC_OFFSET_DEFAULT			190		
#define ADC_RANGE					4096
#define ADC_VREF_1000				1.000f
#define ADC_VREF_1650				1.650f
#define ADC_VREF_2062				2.040f
#define PSU_VOLT_DIVIDER			(1 + (4700 + 4700) / 470)	// 21 V per volt sense	
#define PSU_CURRENT_REF				(2.500f * 1.0f / 3.0f)	
#define PSU_CURRENT_DIVIDER			(1.0f / (0.0323f))	// 33.3 mV at sensor per Amp
#define TEMP_SENSOR_MAXVALUE		358.0f

#define ADC_OVERSAMPLING_GAIN		65536
#define OFFSET_TEMP_SENSOR(_MAXVAL)	((int16_t)((float)_MAXVAL * ((ADC_VREF_1000 / ADC_VREF_2062) * (273.0f / TEMP_SENSOR_MAXVALUE))))
#define GAIN_TEMP_SENSOR(_MAXVAL)	(1.0f / (float)(((float)ADC_OVERSAMPLING_GAIN / ADC_VREF_2062) * ((float)_MAXVAL / TEMP_SENSOR_MAXVALUE)))
#define GAIN_PSU_VOLTAGE			(1.0f / (float)(((float)ADC_OVERSAMPLING_GAIN / ADC_VREF_2062) * ((float)ADC_RANGE / PSU_VOLT_DIVIDER)))
#define GAIN_PSU_CURRENT			(1.0f / (float)(((float)ADC_OVERSAMPLING_GAIN / ADC_VREF_2062) * ((float)ADC_RANGE / PSU_CURRENT_DIVIDER)))

// Simple fast linear congruential RNG, from http://www.cse.yorku.ca/~oz/marsaglia-rng.html
#define DO_LC_RNG(x)							do { x = (uint32_t) 69069 * (x) + (uint32_t) 1234567; } while(0)

// Simple first-order filters
#define FILTER32(newData,filt)					do { filt -= (filt >> ADC_FILTER_BITS); filt += ((int32_t) (newData)); } while(0)
// #define FILTER32(newData,filt)					do { filt -= filt >> (ADC_FILTER_BITS / 2); filt += ((int32_t) (newData)) << (ADC_FILTER_BITS / 2); } while(0)
// #define FILTER32_UNSIGNED(newData,filt)			do { filt -= filt >> (ADC_FILTER_BITS / 2); filt += ((uint32_t) (newData)) << (ADC_FILTER_BITS / 2); } while(0)
// #define FILTER32SQ(newData,filt)				do { filt -= filt >> 8; filt += ((int32_t) (newData)) * ((int32_t) (newData)); } while(0)
// #define FILTER32PWR(newData,filt)				do { filt -= filt >> 8; filt += ((int32_t) (newData)); } while(0)
// #define FILTER32_SHIFT10_DITHER(newData,filt)	do { static uint32_t rng;	DO_LC_RNG(rng);	filt += (((int32_t) (newData)) - (((int32_t) filt) >> 5) + (((int32_t) rng) >> 21)) >> 5; } while(0)


typedef struct  
{
	float		floating;
	uint16_t	integer;
}adc_sample_t;


typedef struct
{
	int16_t milliAmps_devA[12];
	int16_t milliAmps_devB[12];
	int16_t milliAmps_total;
	int16_t milliVolts_psu;
}adc_packet_t;


void adcInit(adc_packet_t * packet);
void adcMeasure();
void adcPrint();

