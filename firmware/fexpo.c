/***********************************************************************
 * fexpo.c - 1V/Octave Expo conversion                                 *
 *    Project:        fpga1 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           02/27/2010                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#include <p33FJ128GP204.h>
#include "fexpo.h"

/* Expo Conversion */
unsigned long fexpo(int in, int scale)
{
	unsigned long result;

	result = in;
	result = result << 19;
	return result;
}
