CC=avr-gcc
CFLAGS=-g -Os -Wall -mcall-prologues -mmcu=attiny13a -std=c99 -pedantic -Wundef
OBJ2HEX=avr-objcopy 
UISP=avrdude 
TARGET=lucid
PROGRAMMER=usbtiny

program : $(TARGET).hex
	sudo $(UISP) -c $(PROGRAMMER) -P usb -p t13 -U flash:w:$(TARGET).hex -v

%.obj : %.o
	$(CC) $(CFLAGS) $< -o $@

%.hex : %.obj
	$(OBJ2HEX) -j .text -O ihex $< $@

clean :
	rm -f *.hex *.obj *.o a.out
