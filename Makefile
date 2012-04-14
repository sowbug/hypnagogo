TARGET=dream-machine
MCU=attiny13a
AVRDUDE_TARGET=t13
F_CPU=1200000UL

UISP=avrdude
PROGRAMMER=usbtiny
PORT=usb
PROGRAMMER_ARGS=-c $(PROGRAMMER) -P $(PORT) -p $(AVRDUDE_TARGET) -v

CC=avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -std=c99 -pedantic -Wundef \
	-mmcu=$(MCU) \
	-DF_CPU=$(F_CPU)
OBJ2HEX=avr-objcopy

program : $(TARGET).hex
	sudo $(UISP)  $(PROGRAMMER_ARGS) \
	  -U flash:w:$(TARGET).hex

%.obj : %.o
	$(CC) $(CFLAGS) $< -o $@

%.hex : %.obj
	$(OBJ2HEX) -j .text -O ihex $< $@

prod: CFLAGS += -DPRODUCTION=1
prod: program

fuse:
	sudo $(UISP)  $(PROGRAMMER_ARGS) \
    -e \
	  -U lfuse:w:0x6A:m \
		-U hfuse:w:0xFF:m

clean :
	rm -f *.hex *.obj *.o a.out
