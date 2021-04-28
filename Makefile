BIN=henio2.0
OBJS=henio2.0.o fast_hsv2rgb_8bit.o

CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-Os -DF_CPU=16000000UL -mmcu=atmega328p
PORT=/dev/ttyUSB0

${BIN}.hex: ${BIN}.elf
	${OBJCOPY} -O ihex -R .eeprom $< $@

${BIN}.elf: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^

flash: ${BIN}.hex
	avrdude -c arduino -p m328p -P ${PORT} -b 57600 -U flash:w:$<

clean:
	rm -f ${BIN}.elf ${BIN}.hex ${OBJS}
