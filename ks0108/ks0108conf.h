/*! \file ks0108conf.h \brief Graphic LCD driver configuration. */
//*****************************************************************************
//
// File Name	: 'ks0108conf.h'
// Title		: Graphic LCD driver for HD61202/KS0108 displays
// Author		: Pascal Stang - Copyright (C) 2001-2003
// Date			: 10/19/2001
// Revised		: 5/1/2003
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


#ifndef KS0108CONF_H
#define KS0108CONF_H

// GLCD_PORT_INTERFACE specifics
// make sure these parameters are not already defined elsewhere
#define GLCD_CTRL_PORT	PORTA	// PORT for LCD control signals
#define GLCD_CTRL_DDR	DDRA	// DDR register of LCD_CTRL_PORT
#define GLCD_CTRL_RS	PA7	// pin for LCD Register Select
#define GLCD_CTRL_RW	PA6	// pin for LCD Read/Write
#define GLCD_CTRL_E	PA5	// pin for LCD Enable
#define GLCD_CTRL_CS0	PA4	// pin for LCD Controller 0 Chip Select
#define GLCD_CTRL_CS1	PA3	// pin for LCD Controller 1 Chip Select(*)
#define GLCD_CTRL_RESET	PA0	// pin for LCD Reset
// (*) NOTE: additonal controller chip selects are optional and 
// will be automatically used per each step in 64 pixels of display size
// Example: Display with 128 hozizontal pixels uses 2 controllers
#define GLCD_DATA_PORT	PORTC	// PORT for LCD data signals
#define GLCD_DATA_DDR	DDRC	// DDR register of LCD_DATA_PORT
#define GLCD_DATA_PIN	PINC	// PIN register of LCD_DATA_PORT

// LCD geometry defines (change these definitions to adapt code/settings)
#define GLCD_XPIXELS	128		// pixel width of entire display
#define GLCD_YPIXELS	64		// pixel height of entire display
#define GLCD_CONTROLLER_XPIXELS	64	// pixel width of one display controller

// Set text size of display
// These definitions are not currently used and will probably move to glcd.h
#define GLCD_TEXT_LINES           8     // visible lines
#define GLCD_TEXT_LINE_LENGTH    22     // internal line length

#endif
