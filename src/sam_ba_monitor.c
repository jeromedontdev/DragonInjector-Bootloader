/* ----------------------------------------------------------------------------
 *         SAM Software Package License
 * ----------------------------------------------------------------------------
 * Copyright (c) 2011-2014, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition is met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

#include "uf2.h"
#include <stdarg.h>

uint32_t i;
uint16_t write_blink_interval = 5000;
uint16_t write_blink_timer = 0;
uint8_t data[SIZEBUFMAX + 1];

char cmd[SIZEBUFMAX];
  
char bdelay_argument[2] = {'5', '\0'};	// Default reboot to bootloader delay in seconds.
char slots_argument[2] = {'4', '\0'};	// Default multi-payload slot limit.
char mdelay_argument[2] = {'3', '\0'};	// Default hold delay for mode switch in seconds.
char cslot_argument[2] = {'0', '\0'};	// Default selected payload slot. 0 = Single, 1-9 = Multi.
char rdelay_argument[2] = {'1', '\0'};	// Default hold delay for pre-RCM payload switch in seconds.
char dmode_argument[2] = {'0', '\0'};	// Alternate dual-payload SOP, boot slot 01 normally, boot slot 02 if cap button held.
  
char bdelay_command[] = "bdelay";
char slots_command[] = "slots";
char mdelay_command[] = "mdelay";
char cslot_command[] = "cslot";
char rdelay_command[] = "rdelay";
char dmode_command[] = "dmode";
char reboot_command[] = "reboot";
char readall_command[] = "readall";
char freset_command[] = "freset";
char commands_command[] = "commands";
char read_text[] = "Read ";
char write_text[] = "Wrote ";
  
#define PAGE_00 0xFC00
#define PAGE_01 0xFC40
#define PAGE_02 0xFC80
#define PAGE_03 0xFCC0
#define PAGE_04 0xFD00
#define PAGE_05 0xFD40
#define PAGE_06 0xFD80
#define PAGE_07 0xFDC0
#define PAGE_08 0xFE00
#define PAGE_09 0xFE40
#define PAGE_10 0xFE80
#define PAGE_11 0xFEC0
#define PAGE_12 0xFF00
#define PAGE_13 0xFF40
#define PAGE_14 0xFF80
#define PAGE_15 0xFFC0
#define LAST_PAGE ((usersettings_t*)PAGE_15)
#define PAGE_SIZE 0x40

//Make a struct to hold our settings.
typedef struct __attribute__((__packed__)) usersettings
{
	uint8_t a;		//Location for pagedata
	uint8_t b;		//Location for bdelay
	uint8_t c;		//Location for slots
	uint8_t d;		//Location for mdelay
	uint8_t e;		//Location for cslot
	uint8_t f;		//Location for rdelay
	uint8_t g;		//Location for rdelay
} usersettings_t;

// Serial CLI.

void sam_ba_monitor_run(void) {
	PINOP(PIN_PA08, DIRSET);
	read_settings();
	safe_settings();
    while (1) {
		process_msc();
		
		// Empty USART buffer.
		for (i = 0; i < SIZEBUFMAX; i++)
		{
			data[i] = 0;
		}
		
		if(write_blink_timer > 0)
		{
			write_blink_timer--;
			PINOP(PIN_PA08, OUTSET);
			LED_MSC_OFF();
		}
		else
		{			
			PINOP(PIN_PA08, OUTCLR);
		}
		
		// Read USART to buffer.
		cdc_read_buf(data, SIZEBUFMAX);
		
		//Check if buffer has data. Ignore carriage return.
		if (data[0] && *data != '\r')
		{
			const char *result_str = "";
			if (strncmp(data, bdelay_command, strlen(bdelay_command)) == 0)
			{   
				if (data[strlen(bdelay_command)+1] >= '1' && data[strlen(bdelay_command)+1] <= '9')
				{
					bdelay_argument[0] = data[strlen(bdelay_command)+1];
					result_str = write_text;
				}
				else
				{
					result_str = read_text;
				}
				write_settings();
				serial_print(5, result_str, bdelay_command, " = ", bdelay_argument, "\n\r");
			}
			else if (strncmp(data, slots_command, strlen(slots_command)) == 0)
			{   
				if (data[strlen(slots_command)+1] >= '1' && data[strlen(slots_command)+1] <= '9')
				{
					slots_argument[0] = data[strlen(slots_command)+1];
					result_str = write_text;
				}
				else
				{
					result_str = read_text;
				}
				write_settings();
				serial_print(5, result_str, slots_command, " = ", slots_argument, "\n\r");
			}
			else if (strncmp(data, mdelay_command, strlen(mdelay_command)) == 0)
			{   
				if (data[strlen(mdelay_command)+1] >= '1' && data[strlen(mdelay_command)+1] <= '9')
				{
					mdelay_argument[0] = data[strlen(mdelay_command)+1];
					result_str = write_text;
				}
				else
				{
					result_str = read_text;
				}
				write_settings();
				serial_print(5, result_str, mdelay_command, " = ", mdelay_argument, "\n\r");
			}
			else if (strncmp(data, cslot_command, strlen(cslot_command)) == 0)
			{   
				if (data[strlen(cslot_command)+1] >= '0' && data[strlen(cslot_command)+1] <= '9')
				{
					cslot_argument[0] = data[strlen(cslot_command)+1];
					if(cslot_argument[0] > slots_argument[0])
					{
						cslot_argument[0] = slots_argument[0];
					}
					result_str = write_text;					
				}
				else
				{
					result_str = read_text;
				}
				write_settings();
				serial_print(5, result_str, cslot_command, " = ", cslot_argument, "\n\r");
			}
			else if (strncmp(data, rdelay_command, strlen(rdelay_command)) == 0)
			{   
				if (data[strlen(rdelay_command)+1] >= '1' && data[strlen(rdelay_command)+1] <= '3')
				{
					rdelay_argument[0] = data[strlen(rdelay_command)+1];
					result_str = write_text;
				}
				else
				{
					result_str = read_text;
				}
				write_settings();
				serial_print(5, result_str, rdelay_command, " = ", rdelay_argument, "\n\r");
			}	
			else if (strncmp(data, dmode_command, strlen(dmode_command)) == 0)
			{   
				if (data[strlen(dmode_command)+1] >= '0' && data[strlen(dmode_command)+1] <= '1')
				{
					dmode_argument[0] = data[strlen(dmode_command)+1];
					result_str = write_text;
				}
				else
				{
					result_str = read_text;
				}
				write_settings();
				serial_print(5, result_str, dmode_command, " = ", dmode_argument, "\n\r");
			}		
			else if (strncmp(data, reboot_command, strlen(reboot_command)) == 0)
			{			
				cdc_write_buf("Rebooting...", 12);
				delay(20);
				NVIC_SystemReset();
			}		
			else if (strncmp(data, readall_command, strlen(readall_command)) == 0)
			{			   
				serial_print(5, read_text, bdelay_command, " = ", bdelay_argument, "\n\r");
				serial_print(5, read_text, slots_command, " = ", slots_argument, "\n\r");
				serial_print(5, read_text, mdelay_command, " = ", mdelay_argument, "\n\r");
				serial_print(5, read_text, cslot_command, " = ", cslot_argument, "\n\r");
				serial_print(5, read_text, rdelay_command, " = ", rdelay_argument, "\n\r");
				serial_print(5, read_text, dmode_command, " = ", dmode_argument, "\n\r");
			}		
			else if (strncmp(data, freset_command, strlen(freset_command)) == 0)
			{			
				bdelay_argument[0] = '5';
				slots_argument[0] = '4';
				mdelay_argument[0] = '3';
				cslot_argument[0] = '0';
				rdelay_argument[0] = '1';
				dmode_argument[0] = '0';
				write_settings();
				read_settings();
				serial_print(5, write_text, bdelay_command, " = ", bdelay_argument, "\n\r");
				serial_print(5, write_text, slots_command, " = ", slots_argument, "\n\r");
				serial_print(5, write_text, mdelay_command, " = ", mdelay_argument, "\n\r");
				serial_print(5, write_text, cslot_command, " = ", cslot_argument, "\n\r");
				serial_print(5, write_text, rdelay_command, " = ", rdelay_argument, "\n\r");
				serial_print(5, write_text, dmode_command, " = ", dmode_argument, "\n\r");			   
			}		
			else if (strncmp(data, commands_command, strlen(commands_command)) == 0)
			{		
				serial_print(3, "  ", bdelay_command, "\n\r");
				serial_print(3, "  ", slots_command, "\n\r");
				serial_print(3, "  ", mdelay_command, "\n\r");
				serial_print(3, "  ", cslot_command, "\n\r");
				serial_print(3, "  ", rdelay_command, "\n\r");
				serial_print(3, "  ", dmode_command, "\n\r");
				serial_print(3, "  ", reboot_command, "\n\r");
				serial_print(3, "  ", readall_command, "\n\r");
				serial_print(3, "  ", freset_command, "\n\r");
				serial_print(3, "  ", commands_command, "\n\r");	   
			}				
			else
			{
				serial_print(4, "Invalid command. Send '", commands_command, "' for list.", "\n\r");
			} 
			write_blink_timer = write_blink_interval;
		}
	}
}

void read_settings()
{
	//Read settings stored in flash. 16 pages used for wear-levelling, starts reading from last page.
	for (int y = PAGE_15; y >= (PAGE_00); y -= PAGE_SIZE)
	{
		usersettings_t *config = (usersettings_t *)y;
		if(config->a == 0)
		{
			//If we find a page with user settings, read them and stop looking.
			bdelay_argument[0] = config->b + '0';
			slots_argument[0] = config->c + '0';
			mdelay_argument[0] = config->d + '0';
			cslot_argument[0] = config->e + '0';
			rdelay_argument[0] = config->f + '0';
			dmode_argument[0] = config->g + '0';
			break;
		}
	}  
}

void safe_settings()
{
	//Use known good settings if saved settings somehow out of range.
	int f = 0;
	if(bdelay_argument[0] < '1' || bdelay_argument[0] > '9')
	{
		bdelay_argument[0] = '5';
		f++;
	}
	if(slots_argument[0] < '1' || slots_argument[0] > '9')
	{
		slots_argument[0] = '4';
		f++;
	}
	if(mdelay_argument[0] < '1' || mdelay_argument[0] > '9')
	{
		mdelay_argument[0] = '3';
		f++;
	}
	if(cslot_argument[0] < '0' || cslot_argument[0] > '9')
	{
		cslot_argument[0] = '0';
		f++;
	}
	if(rdelay_argument[0] < '1' || rdelay_argument[0] > '3')
	{
		rdelay_argument[0] = '1';
		f++;
	}
	if(dmode_argument[0] < '0' || dmode_argument[0] > '1')
	{
		dmode_argument[0] = '0';
		f++;
	}
	if(f)
	{
		write_settings();
	}
}

void write_settings()
{
	//Copy settings into an array for easier page writing.   
	usersettings_t config;
	uint32_t usersettingarray[16];
	config.a = 0;
	config.b = bdelay_argument[0] - 0x30;
	config.c = slots_argument[0] - 0x30;
	config.d = mdelay_argument[0] - 0x30;
	config.e = cslot_argument[0] - 0x30;
	config.f = rdelay_argument[0] - 0x30;
	config.g = dmode_argument[0] - 0x30;
	memcpy(usersettingarray, &config, sizeof(usersettings_t));

	//Check if all pages used, erase all if true.
	if(LAST_PAGE->a == 0)
	{
		flash_erase_row((uint32_t *)PAGE_00);
		flash_erase_row((uint32_t *)PAGE_04);
		flash_erase_row((uint32_t *)PAGE_08);
		flash_erase_row((uint32_t *)PAGE_12);
	}

	//Store settings in flash. 16 pages used for wear-levelling, starts reading from first page.
	for (int y = PAGE_00; y <= (PAGE_15); y += PAGE_SIZE)
	{
		usersettings_t *flash_config = (usersettings_t *)y;
		if(flash_config->a == 0xFF)
		{
			//If we find a page with no user settings, write them and stop looking.
			flash_write_words((uint32_t *)y, usersettingarray, 16);
			break;
		}
	}   
}

void serial_print(int count, ...)
{
	va_list strs;

	va_start(strs, count);

	for(int i = 0; i < count; i++)
	{
		char *cur_str = va_arg(strs, char *);
		int len = strlen(cur_str);
		cdc_write_buf(cur_str, len);
	}
	va_end(strs);
}