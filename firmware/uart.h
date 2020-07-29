/***********************************************************************
 * uart.h - UART I/O routines                                          *
 *    Project:        fpga1 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           01/16/2010                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#ifndef __uart__
#define __uart__

extern void U1init(void);
extern void U1putc(char ch);
extern void U1printstring( char *str);
extern int U1getc(void);

#endif
