TARGET=dream-machine
MCU=attiny13a
AVRDUDE_TARGET=t13
F_CPU=1200000UL

UISP=avrdude
PROGRAMMER=usbtiny

CC=avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -DF_CPU=$(F_CPU) \
	-mmcu=$(MCU) -std=c99 -pedantic -Wundef
OBJ2HEX=avr-objcopy

program : $(TARGET).hex
	sudo $(UISP) -c $(PROGRAMMER) -P usb -p $(AVRDUDE_TARGET) \
	  -U flash:w:$(TARGET).hex -v

%.obj : %.o
	$(CC) $(CFLAGS) $< -o $@

%.hex : %.obj
	$(OBJ2HEX) -j .text -O ihex $< $@

clean :
	rm -f *.hex *.obj *.o a.out
