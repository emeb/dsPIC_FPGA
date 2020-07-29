/***********************************************************************
 * flash.c - SPI Flash access routines                                 *
 *    Project:        fpga1 - FPGA config from SD file                 * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           01/21/2010                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/

#ifndef __flash__
#define __flash__

int flash_get_status(void);
void flash_get_security(char *buffer);
void flash_cont_read(int page, int byte, int count, char *buffer);
void flash_buffer_write(int byte, int count, char *buffer);
void flash_buffer_page_write_erase(int page);

#endif
