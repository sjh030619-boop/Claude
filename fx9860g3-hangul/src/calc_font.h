#ifndef CALC_FONT_H
#define CALC_FONT_H

/* The 5x7 system font of the Run-Mat screen: fixed pitch 6 px, rows 0..6 (see tools/make_calc_font.py).
   Codes: ASCII, 0x01 x, 0x02 /, 0x03 square, 0x04 pi, 0x05 sqrt, 0x06 small E, 0x07 small minus. */
extern const unsigned char calc_font[128][7];

#define CF_PITCH 6

#endif
