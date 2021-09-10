#ifndef GLYPH_TYPE_H
#define GLYPH_TYPE_H
typedef struct { unsigned char bitswide;
                 unsigned char bitshigh;
                 unsigned char *data; } glyph;

/* 
 * example usage...
 *
 * const char glyph_char_24[] = { // bitmap for a glyph most-significant to least-significant
 *      0x70, 0x40,               // top row of pixels, bitswide is 11 so we need 16 bits (next power of 2) and extra 5 are '0' padding bits 
 *      0x88, 0xc0,
 *      0x09, 0x40,
 *      0x32, 0x40,
 *      0x43, 0xe0,
 *      0x80, 0x40,
 *      0xf8, 0x40};              // bottom row of pixels - note number of rows must match bitshigh
 *
 * const glyph glyph_24 = { 11, 7, (char *)glyph_char_24}; // so this glyph is 11 pixels wide, 7 pixels high and the bitmap is glyph_char_24
 */
#endif
