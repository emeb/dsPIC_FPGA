/***********************************************************************
 * cmd.c - Command parsing                                             *
 *    Project:        fpga1                                            * 
 *    Author:         Eric Brombaugh                                   *
 *    Company:        KBADC                                            * 
 *    Date:           01/16/2010                                       *
 *    File Version:   0.0                                              *
 **********************************************************************/
#include <p33FJ128GP204.h>
#include <stdio.h>
#include <string.h>
#include "FSIO.h"
#include "rprintf.h"
#include "fpga.h"
#include "uart.h"
#include "cmd.h"
#include "adc.h"
#include "flash.h"

/* globals we use here */
extern int scan_stat;

/* locals we use here */
char cmd_buffer[256];
char *cmd_wptr;
const char *cmd_commands[] = 
{
	"help",
	"dir",
	"cfg_fpga",
	"spi_read",
	"spi_write",
	"get_cv",
	"get_flash_status",
	"get_flash_security",
	"dump_flash",
	"flash_file_write",
	"set_scan_stat",
	"get_scan_stat",
	"flash_file_verify",
	""
};

/* reset buffer & display the prompt */
void cmd_prompt(void)
{
	/* reset input buffer */
	cmd_wptr = &cmd_buffer[0];

	/* prompt user */
	U1printstring("\r\n\nCommand>");
}


/* SD filesystem directory */
void cmd_dir(char *name)
{
	int result;
	SearchRec rec;

	if((result = FindFirst(name, ATTR_MASK, &rec)))
	{
		U1printstring("File Not Found\r\n");
	}
	else
	{
		while(!result)
		{
			rprintf("%s\r\n", rec.filename);
			result = FindNext(&rec);
		}
	}
}

/* dump a page of flash memory */
void cmd_dump_flash(int page, int count)
{
	char buffer[528];
	int stop, i, j;

	/* loop over pages */
	stop = page + count;
	while(page<stop)
	{
		/* Read buffer from flash */
		flash_cont_read(page, 0, 528, buffer);

		/* Hex Dump in Canonical form */
		for(i=0;i<528;i+=16)
		{
			/* Address */
			rprintf("%04X:%04X ", page, i);

			/* Hex Bytes */
			for(j=0;j<16;j++)
				rprintf("%02X ", buffer[i+j]&0xff);
			U1printstring(" ");

			/* ASCII chars */
			for(j=0;j<16;j++)
				if(buffer[i+j]>=32)
					rprintf("%c", buffer[i+j]&0xff);
				else
					U1printstring(".");
			U1printstring("\r\n");
		}

		/* update page */
		page++;
	}
}

/* Flash Write File command */
int cmd_flash_write_file(char *fname, int page)
{
	char buffer[528];
	int read, count=0;
    FSFILE * fd;
	
	/* open file or return error*/
	if(!(fd = FSfopen(fname, "r")))
	{
		rprintf("cmd_flash_write_file: open file %s failed\n\r", fname);
		return 1;
	}
	else
	{
		rprintf("cmd_flash_write_file: found file %s\n\r", fname);
	}
	
	/* Loop on file read */
	while( (read=FSfread((char*)buffer, sizeof(char), 528, fd)) > 0 )
	{
		/* write to SRAM buffer in SPI Flash */
		flash_buffer_write(0, 528, buffer);
		
		/* Erase Flash page & write SRAM buffer to Flash */
		flash_buffer_page_write_erase(page++);

		/* inc counter */
		count++;

		/* progress indicator */
		U1printstring(".");
	}

	rprintf("\n\rcmd_flash_write_file: closing file\n\r");
	FSfclose(fd);

	/* done */
	return count;
}

/* Flash Verify command */
void cmd_flash_ver_file(char *fname, int page)
{
	char filebuffer[528], membuffer[528];
	int read, byte, pgerr, toterr = 0;
    FSFILE * fd;
	
	/* open file or return error*/
	if(!(fd = FSfopen(fname, "r")))
	{
		rprintf("cmd_flash_ver_file: open file %s failed\n\r", fname);
		return;
	}
	else
	{
		rprintf("cmd_flash_ver_file: found file %s\n\r", fname);
	}
	
	/* Loop on file read */
	while( (read=FSfread((char*)filebuffer, sizeof(char), 528, fd)) > 0 )
	{
		/* read from SPI Flash */
		flash_cont_read(page, 0, 528, membuffer);
		
		pgerr = 0;
		for(byte=0;byte<528;byte++)
		{
			if(filebuffer[byte] != membuffer[byte])
			{
				rprintf("%04X:%04X ", page, byte);
				rprintf("mismatch: ");
				rprintf("expected 0x%02X, ", filebuffer[byte]);
				rprintf("got 0x%02X\n\r", membuffer[byte]);
				pgerr++;
			}
		}

		/* progress indicator */
		if(pgerr == 0)
			U1printstring(".");
		else
			toterr += pgerr;

		/* inc page */
		page++;
	}

	rprintf("\n\rcmd_flash_ver_file: %d errors\n\r", toterr);
	FSfclose(fd);
}

/* process command line after <cr> */
void cmd_proc(void)
{
	char *token, *argv[3];
	int argc, cmd, reg;
	unsigned long data;

	/* parse out three tokens: cmd arg arg */
	argc = 0;
	token = strtok(cmd_buffer, " ");
	while(token != NULL && argc < 3)
	{
		argv[argc++] = token;
		token = strtok(NULL, " ");
	}

	/* figure out which command it is */
	if(argc > 0)
	{
		cmd = 0;
		while(cmd_commands[cmd] != '\0')
		{
			if(strcmp(argv[0], cmd_commands[cmd])==0)
				break;
			cmd++;
		}
	
		/* Can we handle this? */
		if(cmd_commands[cmd] != '\0')
		{
			U1printstring("\r\n");

			/* Handle commands */
			switch(cmd)
			{
				case 0:		/* Help */
					U1printstring("help - this message\r\n");
					U1printstring("dir [<spec>] - SD Card directory\r\n");
					U1printstring("cfg_fpga <file> - Configure FPA with file\r\n");
					U1printstring("spi_read <addr> - FPGA SPI read reg\r\n");
					U1printstring("spi_write <addr> <data> - FPGA SPI write reg, data\r\n");
					U1printstring("get_cv <chl> - Get CV data\r\n");
					U1printstring("get_flash_status - Get Flash Status reg\r\n");
					U1printstring("get_flash_security - Get Flash Security reg\r\n");
					U1printstring("dump_flash <page> <cnt> - Dump Flash \r\n");
					U1printstring("flash_file_write <file> <start page> - Write File to Flash Page\r\n");
					U1printstring("set_scan_stat <stat> - Enable/disable CV scanning\r\n");
					U1printstring("get_scan_stat - report scan status\r\n");
					U1printstring("flash_file_verify <file> <start page> - Verify File to Flash Page\r\n");
					break;
	
				case 1: 	/* dir */
					if(argc < 2)
					{
						U1printstring("dir *.*\r\n");
						cmd_dir("*.*");
					}
					else
					{
						rprintf("dir %s\r\n", argv[1]);
						cmd_dir(argv[1]);
					}
					break;
	
				case 2: 	/* cfg_fpga */
					if(argc < 2)
						U1printstring("cfg_fpga - missing filename\r\n");
					else
					{
						rprintf("cfg_fpga %s\r\n", argv[1]);
						load_fpga_bs(argv[1]);
					}
					break;
	
				case 3: 	/* spi_read */
					if(argc < 2)
						U1printstring("spi_read - missing arg(s)\r\n");
					else
					{
						reg = (int)strtoul(argv[1], NULL, 0) & 0x7f;
						data = fpga_spi_read(reg);
						rprintf("spi_read: 0x%02X = 0x%08lX\r\n", reg, data);
					}
					break;
	
				case 4: 	/* spi_write */
					if(argc < 3)
						U1printstring("spi_write - missing arg(s)\r\n");
					else
					{
						reg = (int)strtoul(argv[1], NULL, 0) & 0x7f;
						data = strtoul(argv[2], NULL, 0);
						fpga_spi_write(reg, data);
						rprintf("spi_write: 0x%02X 0x%08lX\r\n", reg, data);
					}
					break;

				case 5: 	/* get_cv */
					if(argc < 2)
						U1printstring("get_cv - missing arg(s)\r\n");
					else
					{
						reg = (int)strtoul(argv[1], NULL, 0) & 0xf;
						//rprintf("get_cv: 0x%02X = %d\r\n", reg, ADC_buff[reg]);
						rprintf("get_cv: 0x%02X = %d\r\n", reg, cv_dat[reg&3]);
					}
					break;
	
				case 6: 	/* get_flash_status */
					rprintf("get_flash_status: = 0x%02X\r\n", flash_get_status());
					break;
	
				case 7: 	/* get_flash_security */
					U1printstring("get_flash_security:\r\n");
					{
						char buffer[128];
						int i;

						flash_get_security(buffer);
						for(i=0;i<128;i++)
						{
							rprintf("%02X ", buffer[i]&0xff);
							if(i%16 == 15)
								U1printstring("\r\n");
						}
					}
					break;
	
				case 8: 	/* dump_flash */
					if(argc < 3)
						U1printstring("dump_flash - missing arg(s)\r\n");
					else
					{
						int pg, cnt;

						pg = (int)strtoul(argv[1], NULL, 0) & 0x1fff;
						cnt = (int)strtoul(argv[2], NULL, 0) & 0x1fff;
						rprintf("dump_flash: 0x%04X 0x%04X\r\n", pg, cnt);
						cmd_dump_flash(pg, cnt);
						U1printstring("\r\n");
					}
					break;
	
				case 9: 	/* flash_file_write */
					if(argc < 3)
						U1printstring("flash_file_write - missing arg(s)\r\n");
					else
					{
						reg = (int)strtoul(argv[2], NULL, 0) & 0x1fff;
						rprintf("flash_file_write: %s > 0x%04X\r\n", argv[1], reg);
						if((reg = cmd_flash_write_file(argv[1], reg)))
							rprintf("   wrote %d pages.\r\n", reg);
						else
							U1printstring("Error - no pages written.\r\n");
						U1printstring("\r\n");
					}
					break;
	
				case 10: 	/* set_scan_stat */
					if(argc < 2)
						U1printstring("set_scan_stat - missing arg(s)\r\n");
					else
					{
						reg = (int)strtoul(argv[1], NULL, 0) & 0x1;
						rprintf("set_scan_stat: 0x%02X\r\n", reg);
						scan_stat = reg;
					}
					break;
	
				case 11: 	/* get_scan_stat */
					rprintf("get_scan_stat: 0x%02X\r\n", scan_stat);
					break;
	
				case 12: 	/* flash_file_verify */
					if(argc < 3)
						U1printstring("flash_file_verify - missing arg(s)\r\n");
					else
					{
						reg = (int)strtoul(argv[2], NULL, 0) & 0x1fff;
						rprintf("flash_file_verify: %s > 0x%04X\r\n", argv[1], reg);
						cmd_flash_ver_file(argv[1], reg);
						U1printstring("\r\n");
					}
					break;
	
				default:	/* shouldn't get here */
					break;
			}
		}
		else
			U1printstring("Unknown command\r\n");
	}
}
	
void init_cmd(void)
{
	/* just prompts for now */
	cmd_prompt();
}

void cmd_parse(char ch)
{
	/* accumulate chars until cr, handle backspace */
	if(ch == '\b')
	{
		/* check for buffer underflow */
		if(cmd_wptr - &cmd_buffer[0] > 0)
		{
			U1printstring("\b \b");		/* Erase & backspace */
			cmd_wptr--;		/* remove previous char */
		}
	}
	else if(ch == '\r')
	{
		*cmd_wptr = '\0';	/* null terminate, no inc */
		cmd_proc();
		cmd_prompt();
	}
	else
	{
		/* check for buffer full (leave room for null) */
		if(cmd_wptr - &cmd_buffer[0] < 254)
		{
			*cmd_wptr++ = ch;	/* store to buffer */
			U1putc(ch);			/* echo */
			//rprintf("0x%02X ",ch);			/* echo hex */
		}
	}
}
