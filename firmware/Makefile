TARGET=hypnagogo
MCU=attiny13a
AVRDUDE_TARGET=t13
F_CPU=1200000UL

UISP=avrdude
PROGRAMMER=avrisp2
PORT=usb
PROGRAMMER_ARGS=-c $(PROGRAMMER) -P $(PORT) -p $(AVRDUDE_TARGET) -v

CC=avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -std=c99 -pedantic -Wundef \
	-funsigned-char -funsigned-bitfields -fpack-struct \
	-fshort-enums -ffunction-sections \
	--combine -fwhole-program -fno-tree-scev-cprop -fno-split-wide-types \
	-mmcu=$(MCU) \
	-DF_CPU=$(F_CPU)
OBJ2HEX=avr-objcopy

program : $(TARGET).hex
	$(UISP)  $(PROGRAMMER_ARGS) \
	  -U flash:w:$(TARGET).hex

%.obj : %.o
	$(CC) $(CFLAGS) $< -o $@

%.hex : main.obj
	$(OBJ2HEX) -j .text -j .data -O ihex $< $@

prod: CFLAGS += -DPRODUCTION=1
prod: clean program

old_prod: CFLAGS += -DOLD_HARDWARE=1
old_prod: CFLAGS += -DPRODUCTION=1
old_prod: clean program

fuse:
	sudo $(UISP)  $(PROGRAMMER_ARGS) \
    -e \
	  -U lfuse:w:0x6A:m \
		-U hfuse:w:0xFF:m

clean :
	rm -f *.hex *.obj *.o a.out
