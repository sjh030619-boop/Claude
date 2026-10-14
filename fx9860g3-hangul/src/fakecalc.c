#include <gint/display.h>

#include "fakecalc.h"

void fakecalc_draw(void)
{
    dclear(C_WHITE);
    for(int y = 0; y < 64; y++)
        for(int x = 0; x < 128; x++)
            if(fakecalc_bits[y][x >> 3] & (0x80 >> (x & 7))) dpixel(x, y, C_BLACK);
}
