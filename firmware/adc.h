/***********************************************************************
 * adc.c - ADC routines                                                *
 *    Project:        fpga1 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           01/19/2010                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#ifndef __adc__
#define __adc__

extern int cv_dat[4];	// Copy of ADC data buf
extern int cv_new;		// flag for new data

extern void ADCinit(void);

#endif
