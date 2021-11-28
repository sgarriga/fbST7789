CLIBS=-lfbST7789 -lgpiod -lz

all: pi_alarm pi_alarm_

pi_alarm: clock.c glyphs.c
	$(CC) -o $@ $^ $(CLIBS)

pi_alarm_: clock.c glyphs.c
	$(CC) -o $@ $^ $(CLIBS) -static

clean:
	rm -f pi_alarm  pi_alarm_

