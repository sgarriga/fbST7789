
inc=glyph_type.h framebuffer.h
src=framebuffer.c
obj=$(src:.c=.o)

all: libfbST7789.so libfbST7789.a

libfbST7789.so: $(inc) $(src)
	$(CC) -shared $(src) -o $@

libfbST7789.a: $(inc) framebuffer.o
	ar rcs $@ $(obj)

clean:
	rm *.o *.so *.a

install: libfbST7789.so libfbST7789.a $(inc)
	cp -f $(inc) /usr/local/include/
	cp -f libfbST7789.so libfbST7789.a /usr/local/lib/
	ldconfig -n /usr/local/lib
	ldconfig

