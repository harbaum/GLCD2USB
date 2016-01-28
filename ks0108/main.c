/* Name: main.c
 * Project: GLCD2USB; graphic lcd display interface based on AVR USB driver
 * Author: Till Harbaum
 * Creation Date: 2007-07-03
 * Tabsize: 4
 * Copyright: (c) 2007 by Till Harbaum <till@harbaum.org>
 * License: GPL
 * This Revision: $Id: main.c,v 1.2 2007/01/14 12:12:27 harbaum Exp $
 */

// http://blog.yibble.org/2007/06/06/feisty-usb-joystick-issue-resolved/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>

#include <util/delay.h>

#include "usbdrv.h"
#include "oddebug.h"

#include "../lcd4linux/glcd2usb.h"
#include "version.h"

/* generic lcd related includes */
#include "glcd.h"
#include "rprintf.h"

/* display specific includes */
#include "ks0108.h"

/* this drivers info */
static const display_info_t display_info PROGMEM = {
  GLCD2USB_RID_GET_INFO,
  "KS0108",
  128, 64,
  FLAG_VERTICAL_UNITS | FLAG_BACKLIGHT
};

#define USB_HID_REPORT_TYPE_INPUT   1
#define USB_HID_REPORT_TYPE_OUTPUT  2
#define USB_HID_REPORT_TYPE_FEATURE 3

/* ------------------------------------------------------------------------- */
/* ---------------------------- RS232 debugging ---------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef DEBUG
#define DEBUGF(format, args...) printf_P(PSTR(format), ##args)

#define BITRATE    (115200/2)
#define UART_REG   (((F_CPU / BITRATE)-8)/16)

static int uart_putchar(char c, FILE *stream) {
  if (c == '\n') uart_putchar('\r', stream);
  loop_until_bit_is_set(UCSRA, UDRE);
  wdt_reset();

  UDR = c;
  return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar,NULL,_FDEV_SETUP_WRITE);

void uart_init(void) {
  UBRRL = UART_REG;
  UBRRH = 0;
  
  UCSRA = _BV(U2X);
  UCSRB = _BV(TXEN);                  // enable transmitter only
  UCSRC = _BV(URSEL) | (3 << UCSZ0);  // 8n1

  stdout = &mystdout;
}

#else
#define DEBUGF(format, args...)
#define uart_init()
#endif

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

static uchar    reportBuffer[sizeof(display_info_t)]; /* buffer for HID reports */

const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor specific)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)

    0x85, GLCD2USB_RID_GET_INFO,   //   REPORT_ID
    0x95, sizeof(display_info_t)-1,//   REPORT_COUNT
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, GLCD2USB_RID_SET_ALLOC,  //   REPORT_ID
    0x95, 1,                       //   REPORT_COUNT (1)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, GLCD2USB_RID_GET_BUTTONS,//   REPORT_ID
    0x95, 1,                       //   REPORT_COUNT (1)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, GLCD2USB_RID_SET_BL,     //   REPORT_ID
    0x95, 1,                       //   REPORT_COUNT (1)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, GLCD2USB_RID_WRITE_4,    //   REPORT_ID
    0x95, 4+3,                     //   REPORT_COUNT (7)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, GLCD2USB_RID_WRITE_8,    //   REPORT_ID
    0x95, 8+3,                     //   REPORT_COUNT (11)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, GLCD2USB_RID_WRITE_16,   //   REPORT_ID
    0x95, 16+3,                    //   REPORT_COUNT (19)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, GLCD2USB_RID_WRITE_32,   //   REPORT_ID
    0x95, 32+3,                    //   REPORT_COUNT (35)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, GLCD2USB_RID_WRITE_64,   //   REPORT_ID
    0x95, 64+3,                    //   REPORT_COUNT (67)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0x85, GLCD2USB_RID_WRITE_128,  //   REPORT_ID
    0x95, 128+3,                   //   REPORT_COUNT (131)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)

    0xc0                           // END_COLLECTION
};

/* some forward declarations */
void whirl_init(void);
void whirl_enable(char on);

typedef struct {
  unsigned short magic;
} config_t;

#define EEPROM_MAGIC  0x4711

/* config stuff is being stored in eeprom */
static const config_t config EEMEM;

/* the following default will be copied to eeprom on initial boot */
static const config_t config_default PROGMEM = {
  EEPROM_MAGIC,
};

uchar button_map;

uchar button_map_get(void) {
  uchar tmp = button_map;
  button_map = ~PINB & 0x0f;  /* clear button memory */

  return tmp;
}

void keyPressed(void) {
  /* also maintain the button map. all button-on events are */
  /* remembered (thus they are or'ed) to make sure that short */
  /* presses don't get unnoticed */
  button_map |= (~PINB & 0x0f);
}

struct {
  uchar report_id;
  unsigned short offset;
  uchar len;
} cmd_state;

uchar	usbFunctionSetup(uchar data[8]) {
  usbRequest_t    *rq = (void *)data;
  
  usbMsgPtr = reportBuffer;
  
  /* class request type */
  if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    
    //    DEBUGF("Request 0x%x, ReportType 0x%x, ReportID 0x%x\n", 
    //	   rq->bRequest, rq->wValue.bytes[1], rq->wValue.bytes[0]);

    switch(rq->bRequest) {
    case USBRQ_HID_SET_REPORT:
      /* wValue: ReportType (highbyte), ReportID (lowbyte) */

      /* check reportType */
      switch(rq->wValue.bytes[1]) {
      case USB_HID_REPORT_TYPE_FEATURE:
	//	DEBUGF("set feature report: ID 0x%x\n", rq->wValue.bytes[0]);

	switch(rq->wValue.bytes[0]) {

	case GLCD2USB_RID_WRITE_4:
	case GLCD2USB_RID_WRITE_8:
	case GLCD2USB_RID_WRITE_16:
	case GLCD2USB_RID_WRITE_32:
	case GLCD2USB_RID_WRITE_64:
	case GLCD2USB_RID_WRITE_128:
	  cmd_state.report_id = GLCD2USB_RID_WRITE;
	  cmd_state.offset = 0xffff;

	  /* more data to come */
	  return 0xff;
	  break;
	  
	case GLCD2USB_RID_SET_ALLOC:
	  DEBUGF("-> set alloc\n");
	  cmd_state.report_id = GLCD2USB_RID_SET_ALLOC;

	  /* more data to come */
	  return 0xff;
	  break;

	case GLCD2USB_RID_SET_BL:
	  DEBUGF("-> set backlight\n");
	  cmd_state.report_id = GLCD2USB_RID_SET_BL;

	  /* more data to come */
	  return 0xff;
	  break;
	}
      }
      break;
      
    case USBRQ_HID_GET_REPORT:
      /* wValue: ReportType (highbyte), ReportID (lowbyte) */
      /* we only have one report type, so don't look at wValue */
      //      DEBUGF("get report\n");

      /* check reportType */
      switch(rq->wValue.bytes[1]) {

      case USB_HID_REPORT_TYPE_FEATURE:
	DEBUGF("get feature report with id %d\n", rq->wValue.bytes[0]);

	/* return the requested report */
	switch(rq->wValue.bytes[0]) {
	case GLCD2USB_RID_GET_INFO:
	  DEBUGF("<- get display info\n");
	  memcpy_P(reportBuffer, &display_info, sizeof(display_info_t));
	  return sizeof(display_info_t);
	  break;
       
	case GLCD2USB_RID_GET_BUTTONS:
	  DEBUGF("<- get buttons\n");
	  reportBuffer[0] = GLCD2USB_RID_GET_BUTTONS;
	  reportBuffer[1] = button_map_get();
	  return 2;
	  break;
	}

	break;
      }

      break;
      
    case USBRQ_HID_GET_IDLE:
      DEBUGF("get idle\n");
      break;
      
    case USBRQ_HID_SET_IDLE:
      DEBUGF("set idle\n");
      break;
      
    default:
      DEBUGF("default?\n");
      break;
    }
  } else {
    /* no vendor specific requests implemented */
  }
  return 0;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
  uchar i;

  switch(cmd_state.report_id) {
  case GLCD2USB_RID_WRITE:
  
    if(cmd_state.offset == 0xffff) {
    
      /* fetch parameters */
      cmd_state.offset = data[1] + 256*data[2];
      cmd_state.len = data[3];
      
      data += 4;
      len -= 4;
      
      /* set draw cursor */
      glcdSetAddress(cmd_state.offset%128, cmd_state.offset/128);
    }
    
    i = (len > cmd_state.len)?cmd_state.len:len;
    cmd_state.len -= i;
    
    while(i--)
      glcdDataWrite(*data++);

    break;

  case GLCD2USB_RID_SET_ALLOC:
    if(data[1]) {
      DEBUGF("-> allocate\n");
      glcdInit();
      whirl_enable(0);
    } else {
      DEBUGF("-> free\n");
      glcdInit();
      whirl_init();
    }

    break;

  case GLCD2USB_RID_SET_BL:
    DEBUGF("-> backlight %d\n", data[1]);
    OCR1AL = data[1];
    break;
  }

    
  return len;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
  DEBUGF("read %d\n",len); 
  return len;
}

/* ------------------------------------------------------------------------- */
/* ----------------------------- screen saver ------------------------------ */
/* ------------------------------------------------------------------------- */

#define WHIRLS  4          // this much "trace" will a whirl leave
#define WHIRL_POINTS  3    // e.g. 2 = lines, 3 = triangles
#define WHIRL_MAX 3
#define WHIRL_TOP 0        // offset from screen top

typedef struct { int x,y; } point_t;
typedef struct { point_t p[WHIRL_POINTS]; } whirl_t;

/* global state of "screen saver" */
static struct { 
  uint8_t enabled; 
  whirl_t step, dir, whirl[WHIRLS]; 
} whirl_state;

void whirl_enable(char on) {
  whirl_state.enabled = on;
}

void whirl_init(void) {
  u08 i, j;

  // perform basic functionality tests
  glcdSetAddress(16,LINE1);
  rprintf("*** GLCD2USB ***");
  glcdSetAddress(1,LINE2);
  rprintf("Driver:  KS0108 V%d.%02x ", VERSION_MAJOR, VERSION_MINOR);

  /* prepare "whirl" animation */
  for(i=0;i<WHIRLS;i++) whirl_state.whirl[i].p[0].x = -1;
  for(j=0;j<WHIRL_POINTS;j++) {
    whirl_state.whirl[0].p[j].x = 
      whirl_state.whirl[WHIRLS-1].p[j].x = rand() % GLCD_XPIXELS;
    whirl_state.whirl[0].p[j].y = 
      whirl_state.whirl[WHIRLS-1].p[j].y = rand() % (GLCD_YPIXELS-WHIRL_TOP);
    whirl_state.step.p[j].x = 1 + rand() % WHIRL_MAX;
    whirl_state.step.p[j].y = 1 + rand() % WHIRL_MAX;
    whirl_state.dir.p[j].x = (rand()&1)?+1:-1;
    whirl_state.dir.p[j].y = (rand()&1)?+1:-1;
  }

  whirl_state.enabled = 1;
}

void whirl_progress(void) {
  u08 j;

  if(!whirl_state.enabled) return;

  /* undraw oldest whirl */
  if(whirl_state.whirl[0].p[0].x >= 0) { 
    for(j=0;j<WHIRL_POINTS-1;j++) 
      glcdLine(glcdChangeDot, 
	       whirl_state.whirl[0].p[j+0].x, 
	       WHIRL_TOP+whirl_state.whirl[0].p[j+0].y, 
	       whirl_state.whirl[0].p[j+1].x, 
	       WHIRL_TOP+whirl_state.whirl[0].p[j+1].y);
#if WHIRL_POINTS > 2
    glcdLine(glcdChangeDot, 
	     whirl_state.whirl[0].p[WHIRL_POINTS-1].x, 
	     WHIRL_TOP+whirl_state.whirl[0].p[WHIRL_POINTS-1].y, 
	     whirl_state.whirl[0].p[0].x, 
	     WHIRL_TOP+whirl_state.whirl[0].p[0].y);
#endif
  }
  
  memmove(whirl_state.whirl, whirl_state.whirl+1, 
	  sizeof(whirl_state.whirl)-sizeof(whirl_t));

  /* check all points of newest whirl */
  for(j=0;j<WHIRL_POINTS;j++) {
    /* check for horizontal boundaries */
    if(((whirl_state.whirl[WHIRLS-1].p[j].x + whirl_state.dir.p[j].x * 
	 whirl_state.step.p[j].x) >= GLCD_XPIXELS) ||
       ((whirl_state.whirl[WHIRLS-1].p[j].x + whirl_state.dir.p[j].x * 
	 whirl_state.step.p[j].x) < 0)) {
      whirl_state.dir.p[j].x = -whirl_state.dir.p[j].x;
      whirl_state.step.p[j].x = 1 + rand() % WHIRL_MAX; 
    } else 
      whirl_state.whirl[WHIRLS-1].p[j].x += 
	whirl_state.dir.p[j].x * whirl_state.step.p[j].x;
    
    /* check for vertical boundaries */
    if(((whirl_state.whirl[WHIRLS-1].p[j].y + whirl_state.dir.p[j].y * 
	 whirl_state.step.p[j].y) >= (GLCD_YPIXELS-WHIRL_TOP)) ||
       ((whirl_state.whirl[WHIRLS-1].p[j].y + whirl_state.dir.p[j].y * 
	 whirl_state.step.p[j].y) < 0)) {
      whirl_state.dir.p[j].y = -whirl_state.dir.p[j].y;
      whirl_state.step.p[j].y = 1 + rand() % WHIRL_MAX; 
    } else 
      whirl_state.whirl[WHIRLS-1].p[j].y += 
	whirl_state.dir.p[j].y * whirl_state.step.p[j].y;
  }

  /* draw new whirl */
  for(j=0;j<WHIRL_POINTS-1;j++) 
    glcdLine(glcdChangeDot, 
	     whirl_state.whirl[WHIRLS-1].p[j+0].x, 
	     WHIRL_TOP+whirl_state.whirl[WHIRLS-1].p[j+0].y, 
	     whirl_state.whirl[WHIRLS-1].p[j+1].x, 
	     WHIRL_TOP+whirl_state.whirl[WHIRLS-1].p[j+1].y);
#if WHIRL_POINTS > 2
  glcdLine(glcdChangeDot, 
	   whirl_state.whirl[WHIRLS-1].p[WHIRL_POINTS-1].x, 
	   WHIRL_TOP+whirl_state.whirl[WHIRLS-1].p[WHIRL_POINTS-1].y, 
	   whirl_state.whirl[WHIRLS-1].p[0].x, 
	   WHIRL_TOP+whirl_state.whirl[WHIRLS-1].p[0].y);
#endif
}

/* ------------------------------------------------------------------------- */
/* ------------------------------- main loop ------------------------------- */
/* ------------------------------------------------------------------------- */

int	main(void) {
  wdt_enable(WDTO_1S);

  /* clear usb ports */
  USB_CFG_IOPORT   &= ~(_BV(USB_CFG_DMINUS_BIT) | _BV(USB_CFG_DPLUS_BIT));

  /* make usb data lines outputs */
  USBDDR    |= _BV(USB_CFG_DMINUS_BIT) | _BV(USB_CFG_DPLUS_BIT);

  /* USB Reset by device only required on Watchdog Reset */
  _delay_loop_2(40000);   // 10ms

  /* make usb data lines inputs */
  USBDDR &= ~(_BV(USB_CFG_DMINUS_BIT) | _BV(USB_CFG_DPLUS_BIT));

  DDRD |= _BV(4);    /* LED port is output */
  PORTD &= ~_BV(4);  /* LED on */

  DDRB &= ~0x0f;     /* key port is input */
  PORTB |= 0x0f;     /* enable pullups */

  /* configure timer 0 for a rate of 12M/(1024 * 256) = 45.78 Hz (~22ms) */
  TCCR0 = 5;         /* timer 0 prescaler: 1024 */

  DDRD |= _BV(5);    /* Backlight port is output */

  /* backlight: OC1A with 8 bit fast PWM, full speed */
  TCCR1A |= _BV(COM1A1) | _BV(WGM10) | _BV(WGM12) ;
  TCCR1B |= _BV(CS10);   
  OCR1AL = 32;

  usbInit();

  uart_init();
  DEBUGF("\n\n*** GLCD2USB ***\n");
  DEBUGF("Driver: KS0108 %ux%u\n", GLCD_XPIXELS, GLCD_YPIXELS);
  DEBUGF("Version: %d.%02x\n", VERSION_MAJOR, VERSION_MINOR);

  /* make sure eeprom is valid */
  if(eeprom_read_word((const uint16_t*)&(config.magic)) != EEPROM_MAGIC) {
    uchar i,c;

    DEBUGF("invalid EEPROM, setting defaults from flash\n");
    
    /* copy config structure from flash to eeprom */
    for(i=0;i<sizeof(config_t);i++) {
      c = pgm_read_byte(((char*)&config_default)+i);
      eeprom_write_byte(((uint8_t*)&config)+i, c);
    }
  } else 
    DEBUGF("EEPROM is valid\n");

  glcdInit();
  // send rprintf output to lcd display
  rprintfInit(glcdWriteChar);
  
  whirl_init();

  extern void *__vectors;
  DEBUGF("base is at %p\n", __vectors);

  sei();
  for(;;) {	/* main event loop */
    whirl_progress();
    wdt_reset();
    usbPoll();
    keyPressed();

    /* vectors is only != NULL if a bootloader is in use */
    if(__vectors) {
      /* check for reset sequence for easy update, */
      /* may be confusing for the user */
      if((PINB & 0x0f) == 0x0a)
	while(1);   /* watchdog will do its job ... */
    }
  }
  
  return 0;
}

/* ------------------------------------------------------------------------- */
