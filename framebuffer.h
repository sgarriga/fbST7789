#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H
#include "glyph_type.h"

// 16bpp is the default - sticking with that
// Red 5 bits, Green 6 bits (only 5 seem to work), Blue 5 bits
#define rgb(r, g, b) \
    (((r & 0x1f) << 11) | ((g & 0x3f) << 6) | (b & 0x1f))

#define Black           0x0000
#define Navy            0x000F
#define DarkGreen       0x03E0
#define DarkCyan        0x03EF
#define Maroon          0x7800
#define Purple          0x780F
#define Olive           0x7BE0
#define LightGrey       0xC618
#define DarkGrey        0x7BEF
#define Blue            0x001F
#define Green           0x07E0
#define Cyan            0x07FF
#define Red             0xF800
#define Magenta         0xF81F
#define Yellow          0xFFE0
#define White           0xFFFF
#define Orange          0xFD20
#define GreenYellow     0xAFE5
#define Pink            0xF81F

// basic drawing functions
void plot(unsigned short x, unsigned short y, unsigned short c);

void box(unsigned short xul, unsigned short yul, 
         unsigned short xlr, unsigned short ylr, 
         unsigned short c);

void line(unsigned short x0, unsigned short y0, 
          unsigned short x1, unsigned short y1, 
          unsigned short c);

// clear the screen
void fb_clear();

// open and close
int fb_open();
void fb_close();


// use these with custom glyphs
void draw_glyph(unsigned short x, unsigned short y, const glyph *g, 
                      unsigned short color, unsigned short scale);
//   ^^ draws a custom glyphs at a specified location

const glyph * char2glyph(char c);
//   ^^ if you use custom glyphs this must be implemented to map characters
//      to specific glyphs

void draw_gstr(unsigned short x, unsigned short y, char *str, 
                    unsigned short color, unsigned short scale);
//   ^^ draws a sequence of glyphs based on a C string, char2glyph() must be 
//      implemented to map every character used to a glyph


// use these with a system font
void draw_char(int x, int y, char ch, unsigned short color, uint8_t chars[],
                    unsigned short scale);
//   ^^ draws a system font character at a specified location

void draw_fstr(int x, int y, char *str, unsigned short color,
                    unsigned short scale);
//   ^^ draws a sequence of system font characters based on a C string

#endif
