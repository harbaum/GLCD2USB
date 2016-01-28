# Name: Makefile
# Project: GLCD2USB
# Author: Christian Starkjohann, modified for GLCD2USB by Till Harbaum
# Creation Date: 2005-03-20
# Tabsize: 4
# Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
# License: Proprietary, free under certain conditions. See Documentation.
# This Revision: $Id: Makefile.avrusb,v 1.1 2007/01/14 12:12:27 harbaum Exp $

# DEFINES += -DBWCT_COMPAT 
# DEFINES += -DDEBUG_LEVEL=1 -DDEBUG
# change the following line to atmega32 to use that cpu
MCU=atmega32
DEFINES += -DF_CPU=16000000
COMPILE = avr-gcc -Wall -O2 -fno-strict-aliasing -Iusbdrv -I. -mmcu=$(MCU) $(DEFINES)

OBJECTS = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o main.o ks0108.o glcd.o rprintf.o


# symbolic targets:
all:	firmware.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(COMPILE) -S $< -o $@

flash:	all
	avrdude -c usbasp -P usb -p $(MCU) -U lfuse:w:0x9f:m -U hfuse:w:0xc9:m -U flash:w:firmware.hex

#	avrdude -c stk500 -P /dev/ttyUSB0 -p $(MCU) -U lfuse:w:0x9f:m -U hfuse:w:0xc9:m -U flash:w:firmware.hex

# Fuse high byte:
# 0xc9 = 1 1 0 0   1 0 0 1 <-- BOOTRST (boot reset vector at 0x0000)
#        ^ ^ ^ ^   ^ ^ ^------ BOOTSZ0
#        | | | |   | +-------- BOOTSZ1
#        | | | |   + --------- EESAVE (don't preserve EEPROM over chip erase)
#        | | | +-------------- CKOPT (full output swing)
#        | | +---------------- SPIEN (allow serial programming)
#        | +------------------ JTAGEN (JTAG disabled)
#        +-------------------- OCDEN (OCD disabled)
# Fuse low byte:
# 0x9f = 1 0 0 1   1 1 1 1
#        ^ ^ \ /   \--+--/
#        | |  |       +------- CKSEL 3..0 (external >8M crystal)
#        | |  +--------------- SUT 1..0 (crystal osc, BOD enabled)
#        | +------------------ BODEN (BrownOut Detector enabled)
#        +-------------------- BODLEVEL (2.7V)
fuse:
	$(UISP) --wr_fuse_h=0xc9 --wr_fuse_l=0x9f


clean:
	rm -f *~ firmware.lst firmware.obj firmware.cof firmware.list firmware.map firmware.bin *.o usbdrv/*.o firmware.s usbdrv/oddebug.s usbdrv/usbdrv.s

# file targets:
firmware.bin:	$(OBJECTS)
	$(COMPILE) -o firmware.bin $(OBJECTS)

firmware.hex:	firmware.bin
	rm -f firmware.hex
	avr-objcopy -j .text -j .data -O ihex firmware.bin firmware.hex
	./checksize firmware.bin 16384 960
# do the checksize script as our last action to allow successful compilation
# on Windows with WinAVR where the Unix commands will fail.

disasm:	firmware.bin
	avr-objdump -d firmware.bin

cpp:
	$(COMPILE) -E main.c
