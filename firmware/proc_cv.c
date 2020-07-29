/***********************************************************************
 * proc_cv.c - CV Handling                                             *
 *    Project:        fpga1 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           02/27/2010                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#include <p33FJ128GP204.h>
#include <stdio.h>
#include "proc_cv.h"
#include "fpga.h"
#include "adc.h"
#include "fexpo.h"

void proc_cv(void)
{
	unsigned long spi_data;

	/* Check if we've got new CVs to deal with */
	if(cv_new == 0)
		return;

	/* reset new flag */
	cv_new = 0;

	/* CV 0 is frequency - convert to expo 32-bit */
	spi_data = fexpo(cv_dat[0]<<1, 0);
	fpga_spi_write(1, spi_data);

	/* CV 1 is left wave - quantize to 128 */
	spi_data = cv_dat[1];
	spi_data = spi_data >> 5;
	fpga_spi_write(2, spi_data);
	fpga_spi_write(3, spi_data+1);

	/* CV 2 is left interp - convert to 16-bit */
	spi_data = cv_dat[2];
	spi_data = spi_data << 4;
	fpga_spi_write(5, spi_data);

	/* CV 3 is right wave  - quantize to 128 */
	spi_data = cv_dat[3];
	spi_data = spi_data >> 5;
	fpga_spi_write(4, spi_data);
}
