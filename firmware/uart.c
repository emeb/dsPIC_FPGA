/***********************************************************************
 * uart.c - UART I/O routines                                          *
 *    Project:        fpga0 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           12/27/2009                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#include <p33FJ128GP204.h>
#include <stdio.h>
#include "uart.h"

char U1RX_buffer[256];
char *U1RX_wptr, *U1RX_rptr;

void U1init(void)
{
	/* init RX buffer write/read pointers*/
	U1RX_wptr = &U1RX_buffer[0];
	U1RX_rptr = &U1RX_buffer[0];

	/* Setup UART1 for 9600 bps */
	U1BRG = 153;	// Buad Rate: Fcy / (16 * 9600bps) - 1
	U1MODE = 0;	// 8bit, 1stop, 16xbaud, 1idle, rxtx ena, no auto, etc.
	U1STA = 0;	// noaddr, int on RX, Tx disable, nosync, noirda, etc.
	U1MODEbits.UARTEN = 1;	// enable UART1
	U1STAbits.UTXEN = 1;	// enable Transmit
    IFS0bits.U1RXIF = 0;	// clear any pending RX IRQ
	IEC0bits.U1RXIE = 1;	// Enable UART1 RX interrupt
}

void U1putc(char ch)
{
	U1TXREG = ch;	/* send char */
	while(U1STAbits.TRMT == 0); /* wait for char sent */
}

void U1printstring( char *str)
{
    unsigned char c;

    while( (c = *str++) )
        U1putc(c);
}

int U1getc(void)
{
	int retval;
	
	/* check if there's data in the buffer */
	if(U1RX_rptr != U1RX_wptr)
	{
		/* get the data */
		retval = (int)*U1RX_rptr++;
		
		/* wrap the pointer */
		if((U1RX_rptr - &U1RX_buffer[0])>=256)
			U1RX_rptr = &U1RX_buffer[0];
	}
	else
		retval = EOF;

	return retval;
}

/* UART1 ISR just receives data & queues it */
void __attribute__((interrupt, auto_psv)) _U1RXInterrupt(void)
{
	char rxchar;

	IFS0bits.U1RXIF = 0;	// Clear the U1 RX interrupt flag

	/* Check for errors & receive byte */
	while(U1STAbits.URXDA)
	{
		if(U1STAbits.FERR)
		{
			/* Frame error */
			rxchar =  U1RXREG;
			continue;				// get next
		}
 
		if(U1STAbits.OERR)
		{
			/* Overrun error */
			U1STAbits.OERR = 0;		// clear overrun
			continue;				// get next
		}
		
		/* get the character */
		rxchar =  U1RXREG;

		/* check if there's room in the buffer */
		if((U1RX_wptr != U1RX_rptr-1) &&
           (U1RX_wptr - U1RX_rptr != 255))
		{
			/* Yes - Queue the new char */
			*U1RX_wptr++ = rxchar;
	
			/* Wrap pointer */
			if((U1RX_wptr - &U1RX_buffer[0])>=256)
				U1RX_wptr = &U1RX_buffer[0];
		}
	}
}
