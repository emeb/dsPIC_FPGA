/***********************************************************************
 * rprintf.c - print to debug UART                                     *
 *    Project:        fpga0 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           12/27/2009                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#include <p33FJ128GP204.h>
#include <stdio.h>
#include <stdarg.h>
#include "uart.h"
#include "rprintf.h"

/* var args printf */
void rprintf(char const *format, ...)
{
    va_list arg;
	char buffer[256];

	/* special code to pass var args to sprintf */
    va_start (arg, format);
	vsprintf(buffer, format, arg);

	/* send to UART */
	U1printstring(buffer);
}
