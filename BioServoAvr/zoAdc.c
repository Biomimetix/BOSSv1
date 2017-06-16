#include <avr/io.h>
#include <avr/interrupt.h>
#include "zoAdc.h"
#include "zoMcu.h"

#define ADC_PRESCALE_MASK		0x07
#define ADC_REFERENCE_MASK		0xC0
#define ADC_TRIGGER_MASK		0x07
#define ADC_CHANNEL_MASK		0x0F

//storage for adc results
static volatile u08 AdcCurrentChannel = 0;
static volatile u16 AdcResult[11] = {0,0,0,0,0,0,0,0,0,0,0};
static bool AdcChannelOn[11] = {FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,
								FALSE,FALSE,FALSE,FALSE,FALSE};
static volatile u16 NewCurrentAverage = 0;
static volatile u16 OldCurrentAverage = 0;


// initialize a2d converter
void zoAdcInit(void)
{
	DDRC = 0x00;
	DIDR0 = 0x00;
	PRR &= ~_BV(PRADC);								// enable ADC power
	ADCSRA |= _BV(ADEN);							// enable ADC (turn on ADC power)
	
	//ADCSRA |= _BV(ADATE);							// enable auto triggering- doesnt work right for some reason
	
	zoAdcSetPrescaler(ZO_ADC_PRESCALE_128);			// set default prescaler
	zoAdcSetReference(ZO_ADC_REFERENCE_AREF);		// set default reference
	zoAdcSetTrigger(ZO_ADC_TRIGGER_FREE_RUNNING);	// set free running mode
	zoAdcChannelEnable(ZO_ADC_CHANNEL_7);			// enable at least on channel
	ADMUX &= ~_BV(ADLAR);							// set to right-adjusted result
	ADCSRA |= _BV(ADIE);							// enable ADC interrupts
	ADCSRA |= _BV(ADSC)|_BV(ADIF);					// start free running conversions
	
	//ADMUX |=7;  //sets reading of channel 7 on admux ** comment out for more ADC channels
 	
	sei();											// turn on interrupts (if not already on)
}

inline void zoAdcSetPrescaler(ZO_ADC_PRESCALE prescale)
{
	ADCSRA = (ADCSRA & ~ADC_PRESCALE_MASK)|prescale;
}

inline void zoAdcSetReference(ZO_ADC_REFERENCE ref)
{
	ADMUX = (ADMUX & ~ADC_REFERENCE_MASK)|ref;
}

inline void zoAdcSetTrigger(ZO_ADC_TRIGGER trig)
{
	ADCSRB = (ADCSRB & ~ADC_TRIGGER_MASK)|trig;
}

inline void zoAdcOff(void)
{
	ADCSRA &= ~_BV(ADIE);	//disable interrupts
	ADCSRA &= ~_BV(ADEN);	//turn off adc power
}

inline void zoAdcChannelEnable(ZO_ADC_CHANNEL channel)
{
	u08 ch;
	
	ch= (u08)channel; 
	
	enterCritical();
	AdcChannelOn[ch] = TRUE;
	exitCritical();
}

void zoAdcChannelDisable(ZO_ADC_CHANNEL channel)
{
	u08 ch;
	
	ch= (u08)channel; 
	
	enterCritical();
	AdcChannelOn[ch] = FALSE;
	exitCritical();
}

u16 zoAdcRead(ZO_ADC_CHANNEL channel)
{
	u16 result;
	u08 ch;
	
	ch = (u08)channel;

	//enterCritical();
	result = AdcResult[ch];
	//exitCritical();

	return result;
}

ISR(ADC_vect)
{
	
	//Comment this for more ADC channels reading
	
	AdcResult[AdcCurrentChannel] = ADCL;			// read in the result first ADCL
	AdcResult[AdcCurrentChannel] |= (ADCH<<8);		// then ADCH


		if (AdcCurrentChannel==ZO_ADC_CHANNEL_7)
		{	
			OldCurrentAverage=NewCurrentAverage;
			NewCurrentAverage = (0.125*AdcResult[AdcCurrentChannel])+(0.875*OldCurrentAverage);
			AdcResult[AdcCurrentChannel]=NewCurrentAverage;
			
		}


	//scan next active channel
	do {
		AdcCurrentChannel++;
		if(AdcCurrentChannel > 10)
		AdcCurrentChannel = 0;
	}while( AdcChannelOn[AdcCurrentChannel] == FALSE );

	//configure mux for next conversion
	if(AdcCurrentChannel <= ZO_ADC_CHANNEL_TEMPERATURE)
	ADMUX = (ADMUX & ~ADC_CHANNEL_MASK)|AdcCurrentChannel;
	else
	ADMUX = (ADMUX & ~ADC_CHANNEL_MASK)|(AdcCurrentChannel+5);

	//start the next conversion
	ADCSRA |= _BV(ADSC)|_BV(ADIF);
		
/*	//Used if you only need Motor Current FB

	//This only reads the FB current from the Motor controller and smooths the average
	AdcResult[7] = ADCL;			// read in the result first ADCL
	AdcResult[7] |= (ADCH<<8);		// then ADCH

	OldCurrentAverage=NewCurrentAverage;
	NewCurrentAverage = (0.05*AdcResult[7])+(0.95*OldCurrentAverage);
	AdcResult[7]=NewCurrentAverage;
	
	//start the next conversion
	ADCSRA |= _BV(ADSC)|_BV(ADIF);
*/
}

