#include <stdint.h>
#include <gint/display.h>

#include "fakecalc.h"
#include "hangul.h"

/* 5x7 glyphs in a 6x8 cell, like the calculator's own text font. */
typedef struct { char ch; uint8_t rows[7]; } glyph5;

static const glyph5 font5[] = {
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {'+', {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'*', {0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00}},   /* shown as × */
    {'/', {0x04, 0x00, 0x1F, 0x00, 0x04, 0x00, 0x00}},   /* shown as ÷ */
    {'(', {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}},
    {')', {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {'E', {0x00, 0x00, 0x1E, 0x10, 0x1C, 0x10, 0x1E}},   /* small "E" of 2.5E-03 */
};

static void text5(int x, int y, const char *s)
{
    for(; *s; s++, x += 6) {
        for(unsigned i = 0; i < sizeof font5 / sizeof font5[0]; i++) {
            if(font5[i].ch != *s) continue;
            for(int r = 0; r < 7; r++)
                for(int c = 0; c < 5; c++)
                    if(font5[i].rows[r] & (0x10 >> c)) dpixel(x + c, y + r, C_BLACK);
        }
    }
}

static int len(const char *s) { int n = 0; while(s[n]) n++; return n; }

static void left(int row, const char *s) { text5(2, row * 8 + 1, s); }
static void right(int row, const char *s) { text5(128 - 6 * len(s), row * 8 + 1, s); }

/* White 3x5 label on a black key box (F1..F4 labels of the Run-Mat menu). */
static void key_label(int i, const char *label, int arrow)
{
    int x0 = 3 + 21 * i;
    drect(x0, 57, x0 + 18, 63, C_BLACK);

    int w = len(label) * 4 + (arrow ? 4 : 0) - 1;
    int x = x0 + (19 - w) / 2;
    if(arrow) {   /* ▶ */
        for(int r = 0; r < 5; r++) dpixel(x, 58 + r, C_WHITE);
        for(int r = 1; r < 4; r++) dpixel(x + 1, 58 + r, C_WHITE);
        dpixel(x + 2, 60, C_WHITE);
        x += 4;
    }
    for(; *label; label++, x += 4) {
        int ch = *label;
        if(ch < 0x20 || ch > 0x7E) continue;
        const uint8_t *g = hg_tiny[ch - 0x20];
        for(int r = 0; r < 5; r++)
            for(int c = 0; c < 3; c++)
                if(g[r] & (4 >> c)) dpixel(x + c, 58 + r, C_WHITE);
    }
}

void fakecalc_draw(void)
{
    dclear(C_WHITE);

    left(0, "2+2*2");   right(1, "6");
    left(2, "(2+2)*2"); right(3, "8");
    left(4, "1/400");   right(5, "2.5E-03");

    /* input cursor: hollow box in row 6 */
    drect(2, 49, 6, 55, C_BLACK);
    drect(3, 50, 5, 54, C_WHITE);

    key_label(0, "JUMP", 0);
    key_label(1, "DEL", 0);
    key_label(2, "MAT", 1);
    key_label(3, "MATH", 0);
}
