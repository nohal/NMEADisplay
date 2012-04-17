/*! \file NMEADisplay.c \NMEA repeater - displays data received from NMEA GPS. */
//*****************************************************************************
//
// File Name	: 'NMEADisplay.c'
// Title	: NMEA repeater - displays data received from NMEA GPS.
// Author	: Pavel Kalian - Copyright (C) 2012
// Created	: 2012.04.17
// Revised	: 2012.04.17
// Version	: 0.1
// Target MCU	: Atmel AVR Series
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include "global.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>
#include "hd44780.h"
#include "buffer.h"
#include "timerx8.h"
#include "uart.h"
#include "gps.h"
#include "nmea.h"
#include "rprintf.h"

#define USART_BAUDRATE 4800
#define BAUD_PRESCALE ((( F_CPU / ( USART_BAUDRATE * 16UL))) - 1)
#define NO_INPUT 0x00
#define ROLLER "-/|\\"

GpsInfoType *gps;
int counter;

void lcdprintc(u08 c)
{
	lcd_putc(c);
}

static void print_latlon(float val, float invalid, u08 islatitude)
{
  if (val == invalid)
    lcd_puts("**********");
  else
  {
    int degs = (int)floor(abs(val));
    int degsmath = (int)floor(val);
    float mins = (val - degsmath) * 60;
	int minprint = (int)floor(mins);
	int minfracprint = (mins - minprint) * 1000;
    if (islatitude)
      lcd_puts(" ");
    else
      if (degs < 100)
        lcd_puts("0");
    rprintf("%d", degs);
    lcd_putc((char)223);
    if (mins < 10)
      lcd_puts("0");
    rprintf("%d", minprint);
	lcd_putc('.');
	if (minfracprint < 10)
      lcd_puts("0");
	if (minfracprint < 100)
      lcd_puts("0");
    rprintf("%d", minfracprint);
    if (islatitude)
      if (val < 0)
        lcd_puts("S");
      else
        lcd_puts("N");
    else
      if (val < 0)
        lcd_puts("W");
      else
        lcd_puts("E");
  }
}

void do_timer(void)
{
	counter++;
}

int main ( void )
{
	counter = 0;
	lcd_init();
    
    lcd_clrscr();
    lcd_puts("Howdy navigator?");
	_delay_ms(1000);
	lcd_clrscr();

	uartInit();                 // initialize UART (serial port)
    uartSetBaudRate(4800);      // set UART speed to 4800 baud
	
	cBuffer *rxb = uartGetRxBuffer();
	
	nmeaInit();
	rprintfInit(lcdprintc);
	
	timerInit();
	timer0SetPrescaler(TIMER_CLK_DIV1024);
	timerAttach(TIMER0OVERFLOW_INT, do_timer);

	while (1) {
		if(!uartReceiveBufferIsEmpty())
		{
			nmeaProcess(rxb);
			gps = gpsGetInfo();
		}
		if (counter < 400)
		{
			//page1
			lcd_goto(0);
			lcd_putc(ROLLER[gps->PosLLA.updates & 3]);
			lcd_goto(1);
			lcd_puts("LA: ");
			print_latlon(gps->PosLLA.lat.f, 0.0, 1);
			lcd_goto(40);
			lcd_puts(" LO: ");
			print_latlon(gps->PosLLA.lon.f, 0.0, 0);
		}
		else if (counter < 600)
		{
			//page2
			lcd_goto(0);
			lcd_puts("C:");
			rprintf("%d", (int)gps->VelHS.heading.f);
			lcd_putc((char)223);
			lcd_puts(" S:");
			rprintfFloat(3, gps->VelHS.speed.f);
			lcd_puts("kt         ");
			lcd_goto(40);
			//hhmmss.sss
			int hh =  (int)(gps->PosLLA.TimeOfFix.f / 10000);
			int mm =  (int)((gps->PosLLA.TimeOfFix.f - (float)hh * 10000) / 100);
			int ss =  (int)(gps->PosLLA.TimeOfFix.f - (float)hh * 10000 - mm * 100);
			lcd_puts("UTC: ");
			if(hh < 10)
				lcd_putc('0');
			rprintf("%d:", hh);
			if(mm < 10)
				lcd_putc('0');
			rprintf("%d:", mm);
			if(ss < 10)
				lcd_putc('0');
			rprintf("%d", ss);
			lcd_puts("          ");
		}
		else
			counter = 0;
	}
	return 0;
}
