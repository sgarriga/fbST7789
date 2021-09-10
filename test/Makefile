all: pi_alarm pi_alarm_

pi_alarm: clock.c glyphs.c
	$(CC) -o $@ $^ -lfbST7789 -lz

pi_alarm_: clock.c glyphs.c
	$(CC) -o $@ $^ -lfbST7789 -lz -static

clean:
	rm -f pi_alarm  pi_alarm_

