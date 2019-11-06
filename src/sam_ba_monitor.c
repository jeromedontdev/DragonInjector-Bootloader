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

//static const char fullVersion[] = "v" SAM_BA_VERSION " [Arduino:XYZ] " __DATE__ " " __TIME__ "\n\r";

/* b_terminal_mode mode (ascii) or hex mode */
#if USE_CDC_TERMINAL
volatile bool b_terminal_mode = false;
#endif
volatile bool b_sam_ba_interface_usart = false;

void sam_ba_monitor_init(uint8_t com_interface) {
#if USE_UART
    // Selects the requested interface for future actions
    if (com_interface == SAM_BA_INTERFACE_USART) {
        b_sam_ba_interface_usart = true;
    }
#endif
}

/**
 * \brief This function allows data rx by USART
 *
 * \param *data  Data pointer
 * \param length Length of the data
 */
void sam_ba_putdata_term(uint8_t *data, uint32_t length) {
#if USE_CDC_TERMINAL
    uint8_t temp, buf[12], *data_ascii;
    uint32_t i, int_value;

    if (b_terminal_mode) {
        if (length == 4)
            int_value = *(uint32_t *)(void *)data;
        else if (length == 2)
            int_value = *(uint16_t *)(void *)data;
        else
            int_value = *(uint8_t *)(void *)data;

        data_ascii = buf + 2;
        data_ascii += length * 2 - 1;

        for (i = 0; i < length * 2; i++) {
            temp = (uint8_t)(int_value & 0xf);

            if (temp <= 0x9)
                *data_ascii = temp | 0x30;
            else
                *data_ascii = temp + 0x37;

            int_value >>= 4;
            data_ascii--;
        }
        buf[0] = '0';
        buf[1] = 'x';
        buf[length * 2 + 2] = '\n';
        buf[length * 2 + 3] = '\r';
        cdc_write_buf(buf, length * 2 + 4);
    } else
#endif
        cdc_write_buf(data, length);
    return;
}

volatile uint32_t sp;
void call_applet(uint32_t address) {
    uint32_t app_start_address;

    /* Save current Stack Pointer */
    sp = __get_MSP();

    /* Rebase the Stack Pointer */
    __set_MSP(*(uint32_t *)address);

    /* Load the Reset Handler address of the application */
    app_start_address = *(uint32_t *)(address + 4);

    /* Jump to application Reset Handler in the application */
    asm("blx %0" ::"r"(app_start_address):"r0","r1","r2","r3","lr");

    /* Rebase the Stack Pointer */
    __set_MSP(sp);
}

uint32_t current_number;
uint32_t i, length;
uint8_t command, *ptr_data, *ptr, data[SIZEBUFMAX + 1];
uint8_t j;
uint32_t u32tmp;

// Prints a 32-bit integer in hex.
void put_uint32(uint32_t n) {
    char buff[8];
    writeNum(buff, n, true);
    cdc_write_buf(buff, 8);
}

char cmd[SIZEBUFMAX];

  uint8_t bdelay = 5;            // bl   Default reboot to bootloader delay in seconds.
  uint8_t slots = 4;             // se   Default multi-payload slot limit.
  uint8_t mdelay = 3;            // bd   Default hold delay for mode switch in seconds.
  uint8_t cslot = 0;             // cs   Default selected payload slot. 0 = Single, 1-8 = Multi.
  uint8_t rdelay = 1;            // rd   Default hold delay for pre-RCM payload switch in seconds.
  uint8_t dmode = 0;             // dm   Alternate dual-payload SOP, boot slot 01 normally, boot slot 02 if cap button held.
  
  char bdelay_argument[1];
  char slots_argument[1];
  char mdelay_argument[1];
  char cslot_argument[1];
  char rdelay_argument[1];
  char dmode_argument[1];
  
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
  
  //Defines
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
    uint8_t a;       //Location for pagedata
    uint8_t b;       //Location for bdelay
    uint8_t c;       //Location for slots
    uint8_t d;       //Location for mdelay
    uint8_t e;       //Location for cslot
    uint8_t f;       //Location for rdelay
    uint8_t g;       //Location for rdelay
  } usersettings_t;

// Serial CLI.

void sam_ba_monitor_run(void) {
	read_settings();
	safesettings();
    while (1) {
        process_msc();
		
		// Empty USART buffer.
		for (i = 0; i < SIZEBUFMAX; i++)
		{
		  data[i] = 0;
		}
		
		// Read USART to buffer.
        cdc_read_buf(data, SIZEBUFMAX);
		
		//Check if buffer has data. Ignore carriage return.
		if (data[0] && *data != '\r')
		{
           if (strncmp(data, bdelay_command, sizeof(strlen(bdelay_command))) == 0)
	   	   { 
 			   if (data[strlen(bdelay_command)+1] >= '1' && data[strlen(bdelay_command)+1] <= '9')
		  	   {
				  bdelay = data[strlen(bdelay_command)+1] - 0x30;
				  refresh_values();				  
				  save_settings();
				  space();
				  cdc_write_buf(bdelay_command, sizeof(bdelay_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(bdelay_argument, sizeof(bdelay_argument));
				  newline();
			   }
			   else
			   {				   
				  refresh_values();
				  read_text();
				  space();
				  cdc_write_buf(bdelay_command, sizeof(bdelay_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(bdelay_argument, sizeof(bdelay_argument));
				  newline();
			   }
		   }			   
		   else if (strncmp(data, slots_command, sizeof(strlen(slots_command))) == 0)
		   {			   
			   if (data[strlen(slots_command)+1] >= '1' && data[strlen(slots_command)+1] <= '9')
		  	   {
				  slots = data[strlen(slots_command)+1] - 0x30;
				  refresh_values();				  
				  save_settings();
				  space();
				  cdc_write_buf(slots_command, sizeof(slots_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(slots_argument, sizeof(slots_argument));
				  newline();
			   }
			   else
			   {
				  refresh_values();
				  read_text();
				  space();
				  cdc_write_buf(slots_command, sizeof(slots_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(slots_argument, sizeof(slots_argument));
				  newline();
			   }
		   }
           else if (strncmp(data, mdelay_command, sizeof(strlen(mdelay_command))) == 0)
		   {			   
			   if (data[strlen(mdelay_command)+1] >= '1' && data[strlen(mdelay_command)+1] <= '9')
		  	   {
				  mdelay = data[strlen(mdelay_command)+1] - 0x30;
				  refresh_values();				  
				  save_settings();
				  space();
				  cdc_write_buf(mdelay_command, sizeof(mdelay_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(mdelay_argument, sizeof(mdelay_argument));
				  newline();
			   }
			   else
			   {
				  refresh_values();
				  read_text();
				  space();
				  cdc_write_buf(mdelay_command, sizeof(mdelay_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(mdelay_argument, sizeof(mdelay_argument));
				  newline();
			   }
		   }		
           else if (strncmp(data, cslot_command, sizeof(strlen(cslot_command))) == 0)
		   {			   
			   if (data[strlen(cslot_command)+1] >= '0' && data[strlen(cslot_command)+1] <= '9')
		  	   {
				  cslot = data[strlen(cslot_command)+1] - 0x30;
				  refresh_values();				  
				  save_settings();
				  space();
				  cdc_write_buf(cslot_command, sizeof(cslot_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(cslot_argument, sizeof(cslot_argument));
				  newline();
			   }
			   else
			   {
				  refresh_values();
				  read_text();
				  space();
				  cdc_write_buf(cslot_command, sizeof(cslot_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(cslot_argument, sizeof(cslot_argument));
				  newline();
			   }
		   }	
           else if (strncmp(data, rdelay_command, sizeof(strlen(rdelay_command))) == 0)
		   {			   
			   if (data[strlen(rdelay_command)+1] >= '1' && data[strlen(rdelay_command)+1] <= '3')
		  	   {
				  rdelay = data[strlen(rdelay_command)+1] - 0x30;
				  refresh_values();				  
				  save_settings();
				  space();
				  cdc_write_buf(rdelay_command, sizeof(rdelay_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(rdelay_argument, sizeof(rdelay_argument));
				  newline();
			   }
			   else
			   {
				  refresh_values();
				  read_text();
				  space();
				  cdc_write_buf(rdelay_command, sizeof(rdelay_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(rdelay_argument, sizeof(rdelay_argument));
				  newline();
			   }
		   }		
           else if (strncmp(data, dmode_command, sizeof(strlen(dmode_command))) == 0)
		   {			   
			   if (data[strlen(dmode_command)+1] >= '0' && data[strlen(dmode_command)+1] <= '1')
		  	   {
				  dmode = data[strlen(dmode_command)+1] - 0x30;
				  refresh_values();				  
				  save_settings();
				  space();
				  cdc_write_buf(dmode_command, sizeof(dmode_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(dmode_argument, sizeof(dmode_argument));
				  newline();
			   }
			   else
			   {
				  refresh_values();
				  read_text();
				  space();
				  cdc_write_buf(dmode_command, sizeof(dmode_command));
				  space();
				  equals();
				  space();
				  cdc_write_buf(dmode_argument, sizeof(dmode_argument));
				  newline();
			   }
		   }		
           else if (strncmp(data, reboot_command, sizeof(strlen(reboot_command))) == 0)
		   {			
               cdc_write_buf("Rebooting...", 12);
			   delay(20);
			   NVIC_SystemReset();
		   }		
           else if (strncmp(data, readall_command, sizeof(strlen(readall_command))) == 0)
		   {			   
			   refresh_values();
			   read_text();
			   space();
			   cdc_write_buf(bdelay_command, sizeof(bdelay_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(bdelay_argument, sizeof(bdelay_argument));
			   newline();
			   read_text();
			   space();
			   cdc_write_buf(slots_command, sizeof(slots_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(slots_argument, sizeof(slots_argument));
			   newline();
			   read_text();
			   space();
			   cdc_write_buf(mdelay_command, sizeof(mdelay_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(mdelay_argument, sizeof(mdelay_argument));
			   newline();
			   read_text();
			   space();
			   cdc_write_buf(cslot_command, sizeof(cslot_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(cslot_argument, sizeof(cslot_argument));
			   newline();
			   read_text();
			   space();
			   cdc_write_buf(rdelay_command, sizeof(rdelay_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(rdelay_argument, sizeof(rdelay_argument));
			   newline();
			   read_text();
			   space();
			   cdc_write_buf(dmode_command, sizeof(dmode_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(dmode_argument, sizeof(dmode_argument));
			   newline();
		   }		
           else if (strncmp(data, freset_command, sizeof(strlen(freset_command))) == 0)
		   {			   
               bdelay = 5;
               slots = 4; 
               mdelay = 3;
               cslot = 0;
               rdelay = 1;
               dmode = 0;
			   refresh_values();				  
			   save_settings();
			   space();
			   cdc_write_buf(bdelay_command, sizeof(bdelay_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(bdelay_argument, sizeof(bdelay_argument));
			   newline();
			   write_text();
			   space();
			   cdc_write_buf(slots_command, sizeof(slots_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(slots_argument, sizeof(slots_argument));
			   newline();
			   write_text();
			   space();
			   cdc_write_buf(mdelay_command, sizeof(mdelay_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(mdelay_argument, sizeof(mdelay_argument));
			   newline();
			   write_text();
			   space();
			   cdc_write_buf(cslot_command, sizeof(cslot_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(cslot_argument, sizeof(cslot_argument));
			   newline();
			   write_text();
			   space();
			   cdc_write_buf(rdelay_command, sizeof(rdelay_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(rdelay_argument, sizeof(rdelay_argument));
			   newline();
			   write_text();
			   space();
			   cdc_write_buf(dmode_command, sizeof(dmode_command));
			   space();
			   equals();
			   space();
			   cdc_write_buf(dmode_argument, sizeof(dmode_argument));
			   newline();			   
		   }		
           else if (strncmp(data, commands_command, sizeof(strlen(commands_command))) == 0)
		   {		
               space();	   
			   cdc_write_buf(bdelay_command, sizeof(bdelay_command));
			   newline();
			   space();	   
			   cdc_write_buf(slots_command, sizeof(slots_command));
			   newline();
			   space();	   
			   cdc_write_buf(mdelay_command, sizeof(mdelay_command));
			   newline();
			   space();	   
			   cdc_write_buf(cslot_command, sizeof(cslot_command));
			   newline();
			   space();	  
			   cdc_write_buf(rdelay_command, sizeof(rdelay_command));
			   newline();
			   space();	  
			   cdc_write_buf(dmode_command, sizeof(dmode_command));
			   newline();	
			   space();	 
			   cdc_write_buf(reboot_command, sizeof(reboot_command));
			   newline();
			   space();	   
			   cdc_write_buf(readall_command, sizeof(readall_command));
			   newline();
			   space();	  
			   cdc_write_buf(freset_command, sizeof(freset_command));
			   newline();
			   space();	  
			   cdc_write_buf(commands_command, sizeof(commands_command));
			   newline();				   
		   }			   
		   else
		   {
		       invalid_command();
		   } 
		}
	}
}

void invalid_command(void)
{
	char invalid_command_text[] = "Invalid command. Send 'commands' for list.";
	cdc_write_buf(invalid_command_text, sizeof(invalid_command_text));
	newline(); 
}

void newline(void)
{
	cdc_write_buf("\n\r", 2);
}

void space(void)
{
	cdc_write_buf(" ", 1);
}

void equals(void)
{
	cdc_write_buf("=", 1);
}

void read_text(void)
{
	cdc_write_buf("Read", 4);
}

void write_text(void)
{
	cdc_write_buf("Wrote", 5);
}

void period_text(void)
{
	cdc_write_buf(".", 1);
}

void save_settings(void)
{
	if(cslot > slots)
	{
		cslot = slots;
	}
	write_settings();
	write_text();
}

void refresh_values(void)
{
    bdelay_argument[0] = bdelay + '0';
    slots_argument[0] = slots + '0';
    mdelay_argument[0] = mdelay + '0';
    cslot_argument[0] = cslot + '0';
    rdelay_argument[0] = rdelay + '0';
    dmode_argument[0] = dmode + '0';
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
      bdelay = config->b;
      slots = config->c;
      mdelay = config->d;
      cslot = config->e;
      rdelay = config->f;
      dmode = config->g;
      break;
    }
  }  
}

void safesettings()
{
  //Use known good settings if saved settings somehow out of range.
  int f = 0;
  if(bdelay < 1 || bdelay > 9)
  {
    bdelay = 5;
    f++;
  }
  if(slots < 1 || slots > 9)
  {
    slots = 4; 
    f++;
  }
  if(mdelay < 1 || mdelay > 9)
  {
    mdelay = 3;
    f++;
  }
  if(cslot < 0 || cslot > 9)
  {
    cslot = 0;
    f++;
  }
  if(rdelay < 1 || rdelay > 3)
  {
    rdelay = 1;
    f++;
  }
  if(dmode < 0 || dmode > 1)
  {
    dmode = 0;
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
   config.b = bdelay;
   config.c = slots;
   config.d = mdelay;
   config.e = cslot;
   config.f = rdelay;
   config.g = dmode;
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