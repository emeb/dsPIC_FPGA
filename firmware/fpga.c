/***********************************************************************
 * fpga.c - configuration routines                                     *
 *    Project:        fpga0 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           12/27/2009                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/

#include <p33FJ128GP204.h>
#include <stdlib.h>
#include "FSIO.h"
#include "spi.h"
#include "rprintf.h"
#include "fpga.h"

/* Uncomment this to test in Linux */
//#define TEST

/* Uncomment this to skip HW handshake on PROG/INIT */
//#define SKIP

/* Uncomment this to dump bitstream to UART */
//#define DUMP

#define READBUFSIZE 512

/* readbuf MUST be on a word boundary */
char readbuf[READBUFSIZE];

/* .bit file header */
const char bit_hdr[] =
{
	0x00, 0x09, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x00, 0x00, 0x01
};

const char *bit_hdr_strings[] =
{
	"filename",
	"device",
	"date",
	"time"
};

const char *deviceid = "3s200avq100";

/* delay routine used to time FPGA operations. Time in ns */
void delay_fpga(int time)
{
	int i;
	
	/* scale time for 100 ns/tick */
	time = time / 100;
	
	/* minimum 1 tick */
	time = time < 1 ? 1 : time;
	
	for (i = 0; i < time; i++)	//We are going to count to 10000 "time" number of times
		asm volatile ("nop");	//"nop" means no-operation.  We don't want to do anything during the delay
}

/* set up the FPGA control bits for a 'benign' state */
void init_fpga(void)
{
	/* Leave all GPIO as inputs for now */
	FPGA_PROG_DIR = 1;

	/* Set as output & de-assert SPI CSL */
	TRISBbits.TRISB15 = 0;
    LATBbits.LATB15 = 1;

	/* Set as output & de-assert SPI MUX */
	TRISBbits.TRISB13 = 0;
    LATBbits.LATB13 = 0;	// for v5 0=>flash, 1=>fpga
}

/* load a bitstream file into the FPGA */
int load_fpga_bs(char *bs_fname)
{
#ifdef TEST
	FILE * fd;
#else
    FSFILE * fd;
#endif
    int read;
    int j, d, header;
	long ct, n;
	char *cp, byte;
	
	/* open file or return error*/
	if(!(fd = FSfopen(bs_fname, "r")))
	{
		rprintf("load_fpga_bs: open file %s failed\n\r", bs_fname);
		return 1;
	}
	else
	{
		rprintf("load_fpga_bs: found bitstream file %s\n\r", bs_fname);
	}

	/* Read file & send bitstream via SPI1 */
	ct = 0;
	header = 1;
	rprintf("load_fpga_bs: parsing bitstream\n\r");
	while( (read=FSfread((char*)readbuf, sizeof(char), READBUFSIZE,fd)) > 0 )
	{
		/* init pointer to keep track */
		cp = readbuf;
		
		/* are we parsing the header? */
		if(header)
		{
			/* check / skip .bit header */
			for(j=0;j<13;j++)
			{
				if(bit_hdr[j] != *cp++)
				{
					rprintf("load_fpga_bs: .bit header mismatch\n\r");
					FSfclose(fd);
					return 1;
				}
			}
			rprintf("load_fpga_bs: found header\n\r");
		
			/* Skip File header chunks */
			for(j=0;j<4;j++)
			{
				/* get 1 byte chunk desginator (a,b,c,d) */
				d = *cp++;
				
				/* compute chunksize */
				n = *cp++;
				n <<= 8;
				n += *cp++;
			
				/* print chunk */
				rprintf("load_fpga_bs: chunk %c length %ld %s %s\n\r", d, n, bit_hdr_strings[j], cp);
			
				/* Check device type */
				if(j==1 && strcmp(cp, deviceid))
				{
					rprintf("load_fpga_bs: Device != %s\n\r", deviceid);
				}
			
				/* skip chunk */
				cp += n;
			}
	
			/* Skip final chunk designator */
			cp++;
		
			/* compute config data size - modified for 16-bit int & char */
			n = *cp++;
			n <<= 8;
			n += *cp++;
			n <<= 8;
			n += *cp++;
			n <<= 8;
			n += *cp++;
			rprintf("load_fpga_bs: config size = %ld\n\r", n);
			
			/* no longer processing header */
			header = 0;
			
#ifndef TEST
			/* pulse PROG_B low min 500 ns */
			FPGA_PROG_DIR = 0;			// set as output
			FPGA_PROG_BIT = 0;			// drive low
			delay_fpga(1000);			// wait a bit
	
#ifndef SKIP
			/* Wait for INIT low */
			rprintf("load_fpga_bs: PROG low, Waiting for INIT low\n\r");
			while(FPGA_INIT_BIT==1)
			{
				asm volatile ("nop");	//"nop" means no-operation.  We don't want to do anything during the delay
			}
	
			/* Release PROG */
			FPGA_PROG_DIR = 1;			// set as input (hi-z)
	
			/* Wait for INIT high */
			rprintf("load_fpga_bs: PROG high, Waiting for INIT high\n\r");
			while(FPGA_INIT_BIT==0)
			{
				asm volatile ("nop");	//"nop" means no-operation.  We don't want to do anything during the delay
			}
#else
			rprintf("load_fpga_bs: skip waits for INIT low->high\n\r");	
#endif

			/* wait 5us */
			delay_fpga(5000);
#endif
			rprintf("load_fpga_bs: Sending bitstream\n\r");
		}

		/* Send bitstream */
		while(cp < (readbuf + read))
		{
			/* get next data byte */
			byte = *cp++;
			
			/* Send byte via SPI1 */
			SPI2txrx(byte);

#ifdef DUMP
			/* diagnostic - dump bitstream as hex */
			if(ct < 512)
			{
				if(ct%16 == 0)
				{
					rprintf("\n\r%06lx : ", ct);
				}
				rprintf("%02x ", byte&0xff);
			}
#endif			
			/* inc byte counter */
			ct++;
		}
		
		/* diagnostic to track buffers */
		rprintf(".");
		
#ifndef SKIP
#ifndef TEST
		/* Check INIT - if low then fail */
		if(FPGA_INIT_BIT==0)
		{
			rprintf("\n\rload_fpga_bs: INIT low during bitstream send\n\r");
			FSfclose(fd);
			return 1;
		}
#endif
#endif
	}
	
	/* close file */
	rprintf("\n\rload_fpga_bs: sent %ld of %ld bytes\n\r", ct, n);
	rprintf("load_fpga_bs: bitstream sent, closing file\n\r");
	FSfclose(fd);
	
	/* send dummy data while waiting for DONE or !INIT */
 	rprintf("load_fpga_bs: sending dummy clocks, waiting for DONE or fail\n\r");
	while((FPGA_DONE_BIT==0) && (FPGA_INIT_BIT==1))
	{
		/* Dummy - all ones */
		SPI2txrx(0xff);
	}
	
	/* return status */
	if(FPGA_DONE_BIT==0)
	{
		rprintf("load_fpga_bs: cfg failed - DONE not high\n\r");
		return 1;	// Done = 0 - error
	}
	else	
	{
		rprintf("load_fpga_bs: success\n\r");
		return 0;	// Done = 1 - OK
	}
}

/* set spi mux to flash & wait */
void fpga_set_mux(void)
{
	unsigned int timeout=0xffff;

	/* request FPGA SPI Slave port mux */
    LATBbits.LATB13 = 1;

	/* wait for Ack */
	while((PORTCbits.RC3==0) && timeout)
	{
		timeout--;
	}

	if(timeout==0)
		rprintf("fpga_set_mux: timeout waiting for ACK\n");
}

/* Common SPI access routine */
unsigned long fpga_spi_rw(int addr, int rd, unsigned long data)
{
	unsigned char spi_data[5];
	int i;
	
	/* setup transaction */
	spi_data[0] = addr & 0x7f;			// 7 bits address
	spi_data[0] |= ((rd & 1) << 7);		// msb is read
	spi_data[1] = (data >> 24) & 0xff;	// top data byte
	spi_data[2] = (data >> 16) & 0xff;	// mid-high data byte
	spi_data[3] = (data >> 8) & 0xff;	// mid-low data byte
	spi_data[4] = data & 0xff;			// low data byte

	/* set mux for fpga access */	
	fpga_set_mux();

	/* assert CSL */
    LATBbits.LATB15 = 0;
	
	/* loop across 5 bytes */
	for(i=0;i<5;i++)
	{
		/* send/receive data */
		spi_data[i] = SPI2txrx(spi_data[i]);
	}
	
	/* deassert CSL */
    LATBbits.LATB15 = 1;
	
	/* reassemble data */
	data = spi_data[1];
	data <<= 8;
	data |= spi_data[2];
	data <<= 8;
	data |= spi_data[3];
	data <<= 8;
	data |= spi_data[4];
	
	return data;
}

/* SPI write to FPGA design */
void fpga_spi_write(int addr, unsigned long data)
{
	fpga_spi_rw(addr, 0, data);
}

/* SPI read from FPGA design */
unsigned long fpga_spi_read(int addr)
{
	return fpga_spi_rw(addr, 1, 0);
}


