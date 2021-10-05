/*
** Basic code to write to framebuffer used by Adafruit PiTFT
** (Should work with any of the ST7789 chipset TFTs, but this was written and
**  and tested only on the 240x240 Part No, 4484.)
**
** The ST7789 datasheet I found suggested 12bpp (RGB 4/4/4), 16bpp (RGB 5/6/5)
** and 18bpp (RGB 6/6/6) are supported, not 8bpp using a palette - it certainly
** won't flip to 8bpp for me. Additionally, on 16bpp the most significant green
** bit doesn't seem to work either - I have not investigated 12bpp or 18bpp as
** C's "unsigned short" is 16 bits.
**
** Standing on the shoulders of giants...
** J.S.Battista
** - https://github.com/JSBattista/Characters_To_Linux_Buffer_THE_HARD_WAY
** J.P.Rosti 
** - http://raspberrycompote.blogspot.com/2015/01/low-level-graphics-on-raspberry-pi-part.html
** Neeraj Mishra
** - https://www.thecrazyprogrammer.com/2017/01/bresenhams-line-drawing-algorithm-c-c.html
**   Tushar Soni
** - https://www.codingalpha.com/bresenham-line-drawing-algorithm-c-program/
**/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <inttypes.h>
#include <zlib.h>
#include <stdbool.h>

#include "glyph_type.h"
#include "framebuffer.h"

static const unsigned char bitmask[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
bool force_180 = false;

// for system font
#define FONT_FILE "/usr/share/consolefonts/Lat15-VGA8.psf.gz"
struct psf_header {
    uint8_t magic[2];
    uint8_t filemode;
    uint8_t fontheight;
};
struct psf_header header;
static uint8_t *chars = NULL;

// for frame buffer itself
static int fb_file_desc = 0;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;

static unsigned short *fb_ptr = NULL; 
// conveniently an unsigned short is 16 bits

// 16bpp is the default - sticking with that
// Red 5 bits, Green 6 bits (only 5 seem to work), Blue 5 bits
#define rgb(r, g, b) \
    (((r & 0x1f) << 11) | ((g & 0x3f) << 6) | (b & 0x1f))

void plot(unsigned short x, unsigned short y, unsigned short c) {
    // only plot within screen limits
    if (x < vinfo.xres && y < vinfo.yres) {
        if (force_180) {
            x = (vinfo.xres - 1) - x;
            y = (vinfo.yres - 1) - y;
        }
        *(fb_ptr + (y * vinfo.xres) + x) = c;
    }
}

void box(unsigned short xul, unsigned short yul, 
                unsigned short xlr, unsigned short ylr, 
                unsigned short c) {
    unsigned short x, y;

    for (y = yul; y <= ylr; y++)
        for (x = xul; x <= xlr; x++)
            plot(x, y, c);
}

void fb_clear() {
    // clear screen
    memset(fb_ptr, 0, finfo.smem_len); // all bits 0 = black
}

int fb_open()
{
    int cc = 0;
    gzFile font_file = NULL;

    // Open the device file for reading and writing
    fb_file_desc = open("/dev/fb1", O_RDWR);
    if (!fb_file_desc) {
        cc = errno;
        printf("Error: cannot open framebuffer device.\n");
        printf("(%s)\n", strerror(cc));
        return(cc);
    }
    printf("The framebuffer device was opened successfully.\n");

    // Get variable screen information - this won't change for a given device
    // but it allows us to (maybe) support the other screen sizes
    if (cc = ioctl(fb_file_desc, FBIOGET_VSCREENINFO, &vinfo)) { // not ==
        printf("Error reading variable information.\n");
        printf("(%s)\n", strerror(cc));
        close(fb_file_desc);
        return(cc);
    }

    // Get fixed screen information - again, for other screen sizes
    if (cc = ioctl(fb_file_desc, FBIOGET_FSCREENINFO, &finfo)) { // not ==
        printf("Error reading fixed information.\n");
        printf("(%s)\n", strerror(cc));
        close(fb_file_desc);
        return(cc);
    }
    printf("Screen is %dx%d, %dbpp, we require %d bytes\n", 
              vinfo.xres, vinfo.yres, vinfo.bits_per_pixel, finfo.smem_len );

    // map fb to user mem 
    fb_ptr = (unsigned short *)mmap(0, 
                                    finfo.smem_len, 
                                    PROT_READ | PROT_WRITE, 
                                    MAP_SHARED, 
                                    fb_file_desc, 
                                    0);

    if ((int)fb_ptr == -1) {
        cc = errno;
        printf("Failed to mmap.\n");
        printf("(%s)\n", strerror(cc));
        close(fb_file_desc);
        return(cc);
    }

    font_file = gzopen(FONT_FILE, "r");
    if (font_file == NULL) {
        cc = errno;
        printf("Failed to open %s.\n", FONT_FILE);
        printf("(%s)\n", strerror(cc));
        return cc;
    }

    gzread(font_file, &header, sizeof (header));
    
    chars = malloc(header.fontheight * 256);
    if (chars == NULL) {
        cc = errno;
        printf("Failed to malloc.\n");
        printf("(%s)\n", strerror(cc));
        gzclose(font_file);
        return cc;
    }

    gzread(font_file, chars, header.fontheight * 256);

    gzclose(font_file);

    return cc;
}

void fb_close() {

    // cleanup
    free(chars);
    munmap(fb_ptr, finfo.smem_len);
    close(fb_file_desc);
}

void line(unsigned short x0, unsigned short y0, 
          unsigned short x1, unsigned short y1, 
          unsigned short c)
{
    unsigned short i;

    if (x0 == x1) {
        for (i = y0; i <= y1; i++)
            plot(x0, i, c);
    } 
    else if (y0 == y1) {
        for (i = x0; i <= x1; i++)
            plot(i, y0, c);
    }
}

void draw_glyph(unsigned short x, unsigned short y, const glyph *g, unsigned short color, unsigned short scale) {
    unsigned short igx, igy;

    if (g->data != (unsigned char *)NULL) {
        for (igy = 0; igy < g->bitshigh; igy++) {
            for (igx = 0; igx < g->bitswide; igx++) {
                if (g->data[(igy * ((g->bitswide+7) / 8)) + (igx / 8)] & bitmask[igx % 8])
                    if (scale != 1)
                        box(x + (scale * igx), y + (scale * igy), x + (scale * igx) + scale, y + (scale * igy) + scale, color);
                    else
                        plot(x+igx, y+igy, color);
            }
        }
    }
}


void draw_gstr(unsigned short x, unsigned short y, char *str, unsigned short color, unsigned short scale) {
    char *p = NULL;
    glyph *g;
    
    for (p=str; *p; p++) {
        g = (glyph *)char2glyph(*p);
        if (g->bitswide) {
            draw_glyph(x, y, g, color, scale);
            x += (g->bitswide * scale); // add extra for inter-glyph spacing
        }
    }
}


void draw_char(int x, int y, char ch, unsigned short c, uint8_t chars[],
                    unsigned short scale) {
    uint8_t row;
    int x1 = x;
    for (int j = 0; j < header.fontheight; j++) {
        row = chars[ch * header.fontheight + j];
        for (int i = 0; i < 8; i++) {
            if (row & 0x80) {
                if (scale != 1)
                    box(x1, y, x1 + scale, y + scale, c);
                else
                    plot(x1, y, c);
            }
            row = row << 1;
            x1 += scale;
        }
        y += scale;
        x1 = x;
    }
}

void draw_fstr(int x, int y, char *s, unsigned short c, unsigned short scale) {
    int k = 0;

    while (s[k]) {
        draw_char(x, y, s[k], c, chars, scale);
        x += (8 + 1) * scale; // 8 pixels wide + 1 pixel gap - per font
        k++;
    }
}
