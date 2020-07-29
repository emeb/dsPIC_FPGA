/***********************************************************************
 * spi.c - SPI I/O routines                                            *
 *    Project:        fpga0 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           12/27/2009                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#include <p33FJ128GP204.h>
#include "spi.h"

void SPI2init(void)
{
	/* original SPI setup */
	SPI2CON1 = 0x013a;			// 8-bit, data chg fall, ck act high, Master, 2:1, 4:1
								// 2.94MHz SPI clock
	SPI2CON2 = 0;				// no framing	
	SPI2STATbits.SPIROV = 0;	// Clear rx ovfl
	SPI2STATbits.SPIEN = 1;		// Enable SPI2 port
}

unsigned char SPI2txrx(unsigned char data)
{
    SPI2BUF = data;					// Data Out - Logic ones
    while(!SPI2STATbits.SPIRBF);	// Wait until cycle complete
    return(SPI2BUF);				// Return with byte read
}
