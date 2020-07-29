/***********************************************************************
 *    Project:        fpga1 - FPGA config from SD file & control       * 
 *                    via SPI. Also includes UART console              * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Filename:       main.c                                           *
 *    Date:           02/06/2010                                       *
 *    File Version:   0.3                                              *
 *    Tools Used: MPLAB IDE -> 8.10                                    *
 *                Compiler  -> 3.10                                    *
 *                                                                     *
 *    Devices Supported:                                               *
 *                dsPIC33FJ128GP204                                    *
 *                                                                     *
 ***********************************************************************
 * History:                                                            *
 *                                                                     *
 * V0.0  01/16/2010                                                    *
 *   - Start                                                           *
 * V0.1  01/19/2010                                                    *
 *   - Added ADC input stuff                                           *
 * V0.2  01/21/2010                                                    *
 *   - Added flash access stuff                                        *
 * V0.3  02/06/2010                                                    *
 *   - Added flash file write cmd, cleanup others                      *
 *                                                                     *
 **********************************************************************/
#include <p33FJ128GP204.h>
#include <stdio.h>
#include "FSIO.h"
#include "SD-SPI.h"
#include "uart.h"
#include "rprintf.h"
#include "spi.h"
#include "fpga.h"
#include "cmd.h"
#include "adc.h"
#include "proc_cv.h"

/************* Configuration Bits **********/
_FBS(BWRP_WRPROTECT_OFF)		// No Boot Protect
_FGS(GSS_OFF)					// No Code Protect
_FOSCSEL(FNOSC_FRCPLL)			// Fast RC and PLL
_FOSC(FCKSM_CSDCMD & OSCIOFNC_ON & IOL1WAY_OFF)
//Turn off clock switch & monitor, OSC2 is GPIO, allow multiple IOLOCKs
_FWDT(FWDTEN_OFF)				// Turn off Watchdog Timer
_FPOR(FPWRT_PWR16)				// Power-up Timer to 16msecs
_FICD(ICS_PGD1 & JTAGEN_OFF)	// Use PGC/D 1, no JTAG

/************* START OF GLOBAL DEFINITIONS **********/
const char *bitfilename = "FPGA.BIT";
int scan_stat;

/************** END OF GLOBAL DEFINITIONS ***********/

/* setup hardware */
void init_hw(void)
{
	/* set up port A9 for LED control */
	TRISAbits.TRISA9=0;			// LED
	LATAbits.LATA9=1;			// on

	/* setup FPGA control bits */
	init_fpga();

	/* Setup UART1 for debug output */
	U1init();

	/* Setup SPI2 for FPGA cfg */
	SPI2init();

	/* Setup ADC1 for CV inputs */
	ADCinit();

	/* Setup Peripheral Pin Select for UART & SPI I/O */
	__builtin_write_OSCCONL(OSCCON & ~(1<<6));	// Unlock access to RP registers

	RPOR6 = 0x000A;		// RP out code for SPI2TX->RB12
	RPOR7 = 0x000B;		// RP out code for SPI2CLK->RB14
	RPOR8 = 0x0800;		// RP out code for SPI1CLK->RC1
	RPOR9 = 0x0007;		// RP out code for SPI1TX->RC2
	RPOR10 = 0x0003;	// RP out code for U1TX->RC4
	RPINR18 = 0x0019;	// RP in code for RC9->U1RX
	RPINR20 = 0x0010;	// RP in code for RC0->SPI1RX
	RPINR22 = 0x000B;	// RP in code for RB11->SPI2RX
	
	__builtin_write_OSCCONL(OSCCON | (1<<6));	// Lock access to RP registers
}

/************* START OF MAIN FUNCTION ***************/

int main ( void )
{
	unsigned int cntL=0;
	int result, timeout=10;
	char rxchar;

	/* Initialize hardware */
	init_hw();

	/* Welcome msg */
	rprintf("\r\n\nWelcome to the dsPIC_fpga system!\r\n");
	rprintf("Built on __TIMESTAMP__\r\n\n");

	/* Loop on FSInit */
	while(!FSInit() && timeout)
	{
		timeout--;
	}

	if(timeout)
	{
		rprintf("FS Initialized in %d tries\n\r", 11-timeout);
		
		/* init the FPGA */
		if(load_fpga_bs((char *)bitfilename))
		{
			rprintf("FPGA Config Failed\n\r");
		}
		else
		{
			rprintf("FPGA Config Successful\n\r");
		}
	}
	else
	{
		rprintf("FS Initialize Failed!!!\n\r");
		result = FSerror();
		rprintf("result = %d\n\r", result);
	}

	/* start command processing */
	scan_stat = 1;
	init_cmd();

	/* Loop forever */
	while(1)
	{
		/* Blink timer */
		if(++cntL == 50000)
		{
			cntL = 0;
			LATAbits.LATA9 = ~LATAbits.LATA9;
		}

		/* UART command processing */
		if((rxchar = U1getc())!= EOF)
		{
			/* Parse commands */
			cmd_parse(rxchar);
		}

		/* Process CVs */
		if(scan_stat)
			proc_cv();
	}

	return 0;
}
