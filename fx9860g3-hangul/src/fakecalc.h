#ifndef FAKECALC_H
#define FAKECALC_H

/* 128x64 bitmap of an idle Run-Mat screen (src/fakecalc_data.c, made from a real
   screenshot by tools/make_fakecalc.py). 1 = black. */
extern const unsigned char fakecalc_bits[64][16];

/* Draw it (dclear + pixels, no dupdate). */
void fakecalc_draw(void);

#endif
