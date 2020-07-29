/***********************************************************************
 * flash.c - SPI Flash access routines                                 *
 *    Project:        fpga1 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           01/21/2010                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/

#include <p33FJ128GP204.h>
#include "spi.h"
#include "rprintf.h"
#include "flash.h"

/* set spi mux to flash & wait */
void flash_set_mux(void)
{
	unsigned int timeout=0xffff;

	/* Set Mux Req */
    LATBbits.LATB13 = 0;

	/* Wait for Ack */
    while((PORTCbits.RC3==1) && timeout)
	{
		timeout--;
	}

	if(timeout==0)
		rprintf("flash_set_mux: timeout waiting for ~ACK\n");
}

/* read status byte */
int flash_get_status(void)
{
	int status;

	/* Select Flash SPI Slave port */
	flash_set_mux();

	/* assert CSL */
    LATBbits.LATB15 = 0;
	
	/* send read status command */
	SPI2txrx(0xD7);

	/* Get status byte */
	status = SPI2txrx(0);

	/* de-assert CSL */
    LATBbits.LATB15 = 1;
	
	return status;
}

void flash_get_security(char *buffer)
{
	int i;
	char *bptr = buffer;

	/* Select Flash SPI Slave port */
	flash_set_mux();

	/* assert CSL */
    LATBbits.LATB15 = 0;
	
	/* send read security reg command */
	SPI2txrx(0x77);
	SPI2txrx(0);
	SPI2txrx(0);
	SPI2txrx(0);

	/* Load data to buffer */
	for(i=0;i<128;i++)
		*bptr++ = SPI2txrx(0);

	/* de-assert CSL */
    LATBbits.LATB15 = 1;
}


void flash_cont_read(int page, int byte, int count, char *buffer)
{
	int i;
	char add[3], *bptr = buffer;

	/* compute address */
	add[0] = byte & 0xFF;
	add[1] = (byte >> 8) & 0x03;
	add[1] |= (page << 2) & 0xFC;
	add[2] = (page >> 6) & 0x7F;

	/* Select Flash SPI Slave port */
	flash_set_mux();

	/* assert CSL */
    LATBbits.LATB15 = 0;
	
	/* send continuous read command */
	SPI2txrx(0x03);
	SPI2txrx(add[2]);
	SPI2txrx(add[1]);
	SPI2txrx(add[0]);

	/* Load data to buffer */
	for(i=0;i<count;i++)
		*bptr++ = SPI2txrx(0);

	/* de-assert CSL */
    LATBbits.LATB15 = 1;
}

void flash_buffer_write(int byte, int count, char *buffer)
{
	int i;
	char add[3], *bptr = buffer;

	/* compute address */
	add[0] = byte & 0xFF;
	add[1] = (byte >> 8) & 0x03;
	add[2] = 0;

	/* wait for status ready */
	while(!(flash_get_status()&0x80))
	{
	};

	/* assert CSL */
    LATBbits.LATB15 = 0;
	
	/* send continuous read command */
	SPI2txrx(0x84);
	SPI2txrx(add[2]);
	SPI2txrx(add[1]);
	SPI2txrx(add[0]);

	/* Send data from buffer */
	for(i=0;i<count;i++)
		 SPI2txrx(*bptr++);

	/* de-assert CSL */
    LATBbits.LATB15 = 1;
}

void flash_buffer_page_write_erase(int page)
{
	char add[3];

	/* compute address */
	add[0] = 0;
	add[1] = 0;
	add[1] |= (page << 2) & 0xFC;
	add[2] = (page >> 6) & 0x7F;

	/* wait for status ready */
	while(!(flash_get_status()&0x80))
	{
	};

	/* assert CSL */
    LATBbits.LATB15 = 0;
	
	/* send continuous read command */
	SPI2txrx(0x83);
	SPI2txrx(add[2]);
	SPI2txrx(add[1]);
	SPI2txrx(add[0]);

	/* de-assert CSL */
    LATBbits.LATB15 = 1;
}
