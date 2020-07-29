/***********************************************************************
 * adc.c - ADC routines                                                *
 *    Project:        fpga1 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           01/19/2010                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#include <p33FJ128GP204.h>
#include <stdio.h>
#include "adc.h"

unsigned int ADC_buff[16] __attribute__((space(dma)));	// ADC data buf
int cv_dat[4];	// Copy of ADC data buf
int cv_new;

void ADCinit(void)
{
	/* ADC1 Setup to read CVs */
	AD1CON1 = 0x04E4;	// Scat/Gath, 12-bit, Integer, Auto, Samp
	AD1CON2 = 0x040C;	// Scan, 4 channels, AVDD & AVSS
	AD1CON3 = 0x0f06;	// Tad=Tcy*(ADCS+1)= (1/23M)*5 = 217ns (4.6Mhz)
	AD1CON4 = 0x0000;	// One sample per buffer
	AD1CHS0 = 0;		// Start CHS0 on AN0
	AD1CHS123 = 0;		// CHS123 unused in 12-bit mode
	AD1PCFGL = 0xFFCC;	// AN 0,1,4,5 used, others disabled
	AD1CSSL = 0x0033;	// Scan AN 0,1,4,5
	IFS0bits.AD1IF = 0;	// Clear the A/D interrupt flag bit
	IEC0bits.AD1IE = 0;	// Do Not Enable A/D interrupt
	AD1CON1bits.ADON = 1;	// Turn on the A/D converter

	/* DMA0 Setup to handle ADC1 in Scatter/Gather mode */
	DMA0CON = 0x0020;		// Peripheral indirect, Continuous
	DMA0PAD = (int)&ADC1BUF0;	// Get data from ADC1BUF0
	DMA0CNT = 3;			// 4 transfers per interrupt
	DMA0REQ = 13;			// IRQ13 (ADC1) as source
	DMA0STA	= __builtin_dmaoffset(ADC_buff);	// Buffer offset
	IFS0bits.DMA0IF	= 0;	// Clear DMA0IF
	IEC0bits.DMA0IE	= 1;	// Enable DMA0 interrupt
	DMA0CONbits.CHEN = 1;	// Enable DMA0

	/* clear new CV flag */
	cv_new = 0; 
}

/****** START OF INTERRUPT SERVICE ROUTINES ********/

/* DMA0 ISR triggered when all 4 CV inputs are digitized */
/* Should happen at 4x ADC period, where ADC period is   */
/* Tcy*(ADCS+1)*(14 + SAMC), or about 5.3us -> 47kHz     */
void __attribute__((interrupt, auto_psv)) _DMA0Interrupt(void)
{
	IFS0bits.DMA0IF = 0;	// Clear IRQ

	/* copy & invert data */
	cv_dat[0] = 4095-ADC_buff[0];
	cv_dat[1] = 4095-ADC_buff[1];
	cv_dat[2] = 4095-ADC_buff[4];
	cv_dat[3] = 4095-ADC_buff[5];

	/* set new CV flag */
	cv_new = 1; 
}
