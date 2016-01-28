/*! \file ks0108.c \brief Graphic LCD driver for HD61202/KS0108 displays. */
//*****************************************************************************
//
// File Name	: 'ks0108.c'
// Title		: Graphic LCD driver for HD61202/KS0108 displays
// Author		: Pascal Stang - Copyright (C) 2001-2003
// Date			: 10/19/2002
// Revised		: 5/5/2003
// Version		: 0.5
// Target MCU	: Atmel AVR
// Editor Tabs	: 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef WIN32
// AVR specific includes
	#include <avr/io.h>
	#include <avr/interrupt.h>
#include <util/delay.h>
#endif

#include "global.h"
#include "ks0108.h"

#define NOP  asm volatile ("nop")

// global variables
GrLcdStateType GrLcdState;

/*************************************************************/
/********************** LOCAL FUNCTIONS **********************/
/*************************************************************/

void glcdInitHW(void) {
  // initialize I/O ports
  // if I/O interface is in use
  
  //TODO: make setup of chip select lines contingent on how
  // many controllers are actually in the display
  
  // initialize LCD control lines levels
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RS);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RW);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_E);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_CS0);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_CS1);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RESET);
  // initialize LCD control port to output
  sbi(GLCD_CTRL_DDR, GLCD_CTRL_RS);
  sbi(GLCD_CTRL_DDR, GLCD_CTRL_RW);
  sbi(GLCD_CTRL_DDR, GLCD_CTRL_E);
  sbi(GLCD_CTRL_DDR, GLCD_CTRL_CS0);
  sbi(GLCD_CTRL_DDR, GLCD_CTRL_CS1);
  sbi(GLCD_CTRL_DDR, GLCD_CTRL_RESET);
  // initialize LCD data
  outb(GLCD_DATA_PORT, 0x00);
  // initialize LCD data port to output
  outb(GLCD_DATA_DDR, 0xFF);
}

void glcdControllerSelect(u08 controller)
{
  // select requested controller
  if(!controller) {
    sbi(GLCD_CTRL_PORT, GLCD_CTRL_CS0);
    cbi(GLCD_CTRL_PORT, GLCD_CTRL_CS1);
  } else {
    cbi(GLCD_CTRL_PORT, GLCD_CTRL_CS0);
    sbi(GLCD_CTRL_PORT, GLCD_CTRL_CS1);
  }
}

void glcdBusyWait(u08 controller) {
	
  // wait until LCD busy bit goes to zero
  // select the controller chip
  glcdControllerSelect(controller);
  // do a read from control register
  outb(GLCD_DATA_PORT, 0x00);  // no pullups
  outb(GLCD_DATA_DDR, 0x00);
  
  sbi(GLCD_CTRL_PORT, GLCD_CTRL_RW);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RS);
  sbi(GLCD_CTRL_PORT, GLCD_CTRL_E);
  NOP; NOP; NOP; NOP;
  NOP; NOP; NOP; NOP;

  while(inb(GLCD_DATA_PIN) & (GLCD_STATUS_BUSY | GLCD_STATUS_RESET));

  cbi(GLCD_CTRL_PORT, GLCD_CTRL_E);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RW);
  outb(GLCD_DATA_DDR, 0xFF);   // output
}

void glcdControlWrite(u08 controller, u08 data) {
  glcdBusyWait(controller);	// wait until LCD not busy
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RS);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RW);
  sbi(GLCD_CTRL_PORT, GLCD_CTRL_E);
  outb(GLCD_DATA_DDR, 0xFF);
  outb(GLCD_DATA_PORT, data);
  NOP; NOP; NOP; NOP;
  NOP; NOP; NOP; NOP;
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_E);
}

u08 glcdControlRead(u08 controller) {
  register u08 data;
  
  glcdBusyWait(controller);		// wait until LCD not busy
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RS);
  outb(GLCD_DATA_DDR, 0x00);
  sbi(GLCD_CTRL_PORT, GLCD_CTRL_RW);
  sbi(GLCD_CTRL_PORT, GLCD_CTRL_E);
  NOP; NOP; NOP; NOP;
  NOP; NOP; NOP; NOP;
  data = inb(GLCD_DATA_PIN);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_E);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RW);
  outb(GLCD_DATA_DDR, 0xFF);
  
  return data;
}

void glcdDataWrite(u08 data) {
  register u08 controller = (GrLcdState.lcdXAddr/GLCD_CONTROLLER_XPIXELS);
	
  glcdBusyWait(controller);		// wait until LCD not busy
  
  outb(GLCD_DATA_DDR, 0xFF);
  outb(GLCD_DATA_PORT, data);
  
  sbi(GLCD_CTRL_PORT, GLCD_CTRL_RS);
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_RW);
  sbi(GLCD_CTRL_PORT, GLCD_CTRL_E);
  
  NOP; NOP; NOP; NOP;
  NOP; NOP; NOP; NOP;
  
  cbi(GLCD_CTRL_PORT, GLCD_CTRL_E);
  
  
  // increment our local address counter
  GrLcdState.ctrlr[controller].xAddr++;
  GrLcdState.lcdXAddr++;
  if(GrLcdState.lcdXAddr >= GLCD_XPIXELS) {
    glcdSetYAddress(GrLcdState.lcdYAddr+1);
    glcdSetXAddress(0);
  }
}

u08 glcdDataRead(u08 dummy)
{
  register u08 data;
  register u08 controller = (GrLcdState.lcdXAddr/GLCD_CONTROLLER_XPIXELS);

  glcdBusyWait(controller);		// wait until LCD not busy

  outb(GLCD_DATA_PORT, 0x00); // no pullups
  outb(GLCD_DATA_DDR, 0x00);  // don't drive data lines
  
  GLCD_CTRL_PORT |= _BV(GLCD_CTRL_RS) | 
                    _BV(GLCD_CTRL_RW) | _BV(GLCD_CTRL_E);
  
  NOP; NOP; NOP; NOP;
  NOP; NOP; NOP; NOP;
  
  data = inb(GLCD_DATA_PIN);

  GLCD_CTRL_PORT &= ~(_BV(GLCD_CTRL_RS) | 
		      _BV(GLCD_CTRL_RW) | _BV(GLCD_CTRL_E));

  if(!dummy) {
    // increment our local address counter, not done for dummy reads
    GrLcdState.ctrlr[controller].xAddr++;
    GrLcdState.lcdXAddr++;
    if(GrLcdState.lcdXAddr >= GLCD_XPIXELS) {
      glcdSetYAddress(GrLcdState.lcdYAddr+1);
      glcdSetXAddress(0);
    }
  }
  return data;
}

void glcdReset(u08 resetState)
{
  // reset lcd if argument is true
  // run lcd if argument is false

  if(resetState)
    cbi(GLCD_CTRL_PORT, GLCD_CTRL_RESET);
  else
    sbi(GLCD_CTRL_PORT, GLCD_CTRL_RESET);
}

void glcdSetXAddress(u08 xAddr)
{
	// record address change locally
	GrLcdState.lcdXAddr = xAddr;
	
	glcdControlWrite(0, GLCD_SET_Y_ADDR);
	GrLcdState.ctrlr[0].xAddr = 0;
	glcdControlWrite(1, GLCD_SET_Y_ADDR);
	GrLcdState.ctrlr[1].xAddr = 0;

	// set y (col) address on destination controller
	glcdControlWrite((GrLcdState.lcdXAddr/GLCD_CONTROLLER_XPIXELS),
		GLCD_SET_Y_ADDR | (GrLcdState.lcdXAddr & 0x3F));
}

void glcdSetYAddress(u08 yAddr) {
  // record address change locally
  GrLcdState.lcdYAddr = yAddr;
  // set page address for all controllers
  glcdControlWrite(0, GLCD_SET_PAGE | yAddr);
  glcdControlWrite(1, GLCD_SET_PAGE | yAddr);
}

/*************************************************************/
/********************* PUBLIC FUNCTIONS **********************/
/*************************************************************/

void glcdInit()
{
	// initialize hardware
	glcdInitHW();
	// bring lcd out of reset
	glcdReset(FALSE);
	// Turn on LCD
	glcdControlWrite(0, GLCD_ON_CTRL | GLCD_ON_DISPLAY);
	glcdControlWrite(1, GLCD_ON_CTRL | GLCD_ON_DISPLAY);
	// clear lcd
	glcdClearScreen();
	// initialize positions
	glcdHome();
}

void glcdHome(void)
{
	// initialize addresses/positions
	glcdStartLine(0);
	glcdSetAddress(0,0);
	// initialize local data structures
	GrLcdState.ctrlr[0].xAddr = GrLcdState.ctrlr[0].yAddr = 0;
	GrLcdState.ctrlr[1].xAddr = GrLcdState.ctrlr[1].yAddr = 0;
}

void glcdClearScreen(void)
{
	u08 pageAddr;
	u08 xAddr;

	// clear LCD
	// loop through all pages
	for(pageAddr=0; pageAddr<(GLCD_YPIXELS>>3); pageAddr++)
	{
		// set page address
		glcdSetAddress(0, pageAddr);
		// clear all lines of this page of display memory
		for(xAddr=0; xAddr<GLCD_XPIXELS; xAddr++)
		{
			glcdDataWrite(0x00);
		}
	}
}

void glcdStartLine(u08 start)
{
  glcdControlWrite(0, GLCD_START_LINE | start);
  glcdControlWrite(1, GLCD_START_LINE | start);
}

void glcdSetAddress(u08 x, u08 yLine) {
  // set addresses
  glcdSetYAddress(yLine);
  glcdSetXAddress(x);
}

void glcdGotoChar(u08 line, u08 col)
{
  glcdSetAddress(col*6, line);
}

void glcdDelay(u16 p)		// 1-8us      ...2-13us     ...5-31us
{				// 10-60us    ...50-290us
	unsigned int i;		// 100-580us  ...500-2,9ms
	unsigned char j; 	// 1000-5,8ms ...5000-29ms
				// 10000-56ms ...30000-170ms
				// 50000-295ms...60000-345ms
//	for (i = 0; i < p; i++) for (j = 0; j < 10; j++) NOP;
	for (i = 0; i < p; i++) for (j = 0; j < 10; j++);
}


// Higher level functionality has been moved to the API-layer glcd.c/glcd.h
