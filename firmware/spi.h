/***********************************************************************
 * spi.h - SPI I/O routines                                            *
 *    Project:        fpga0 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           12/27/2009                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#ifndef __spi__
#define __spi__

extern void SPI2init(void);
extern unsigned char SPI2txrx(unsigned char data);

#endif
