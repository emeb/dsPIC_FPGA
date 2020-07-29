/***********************************************************************
 * fpga.c - configuration routines                                     *
 *    Project:        fpga0 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           12/27/2009                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/

#ifndef __fpga__
#define __fpga__

#define FPGA_CSL_BIT LATBbits.LATB15	// FPGA SPI CSL output bit on RB15
#define FPGA_CSL_DIR TRISBbits.TRISB15	// FPGA SPI CSL direction
#define FPGA_MOSI_BIT LATBbits.LATB12	// FPGA SPI MOSI output bit on RB12
#define FPGA_MOSI_DIR TRISBbits.TRISB12	// FPGA SPI MOSI direction
#define FPGA_MISO_BIT PORTBbits.RB11	// FPGA SPI MISO input bit on RB11
#define FPGA_SCLK_BIT LATBbits.LATB14	// FPGA SPI SCLK output bit on RB14
#define FPGA_SCLK_DIR TRISBbits.TRISB14	// FPGA SPI SCLK direction
#define FPGA_PROG_BIT LATAbits.LATA3	// FPGA PROG_B output bit on RA3
#define FPGA_PROG_DIR TRISAbits.TRISA3	// FPGA PROG_B direction
#define FPGA_IRQ_BIT PORTAbits.RA4		// FPGA IRQ input bit on RA4
#define FPGA_INIT_BIT PORTAbits.RA2		// FPGA INIT input bit on RA2
#define FPGA_DONE_BIT PORTAbits.RA8		// FPGA DONE input bit on RA8

void init_fpga(void);
int load_fpga_bs(char *bs_fname);
unsigned long fpga_spi_rw(int addr, int rd, unsigned long data);
void fpga_spi_write(int addr, unsigned long data);
unsigned long fpga_spi_read(int addr);

#endif
