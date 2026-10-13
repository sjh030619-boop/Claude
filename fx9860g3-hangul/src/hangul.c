#include "hangul.h"

/* Decode one UTF-8 code point and advance *s. Invalid bytes give U+FFFD. */
static uint32_t decode(const char **s)
{
    const uint8_t *p = (const uint8_t *)*s;
    uint32_t c = p[0];
    int n = 0;

    if(c < 0x80) n = 0;
    else if((c & 0xE0) == 0xC0) { c &= 0x1F; n = 1; }
    else if((c & 0xF0) == 0xE0) { c &= 0x0F; n = 2; }
    else if((c & 0xF8) == 0xF0) { c &= 0x07; n = 3; }
    else { *s += 1; return 0xFFFD; }

    for(int i = 1; i <= n; i++) {
        if((p[i] & 0xC0) != 0x80) { *s += i; return 0xFFFD; }
        c = (c << 6) | (p[i] & 0x3F);
    }
    *s += n + 1;
    return c;
}

/* Binary search; returns glyph index or -1. */
static int find(uint32_t cp)
{
    int lo = 0, hi = hg_count - 1;
    if(cp > 0xFFFF) return -1;
    while(lo <= hi) {
        int mid = (lo + hi) >> 1;
        if(hg_cps[mid] == cp) return mid;
        if(hg_cps[mid] < cp) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

static int glyph_index(uint32_t cp)
{
    int i = find(cp);
    if(i < 0) i = find('?');
    return i;
}

static void draw_glyph(int x, int y, int idx)
{
    const uint16_t *rows = hg_bits + (long)idx * hg_height;
    for(int r = 0; r < hg_height; r++) {
        uint16_t v = rows[r];
        for(int c = 0; v; c++, v <<= 1)
            if(v & 0x8000) hg_plot(x + c, y + r);
    }
}


#define TINY_ADV 4

static void draw_tiny(int x, int y, int ch)
{
    if(ch < 0x20 || ch > 0x7E) return;
    if(ch >= 'a' && ch <= 'z') ch -= 32;
    const uint8_t *g = hg_tiny[ch - 0x20];
    for(int r = 0; r < 5; r++)
        for(int c = 0; c < 3; c++)
            if(g[r] & (4 >> c)) hg_plot(x + c, y + r);
}

/* If *s is `_{...}` or `^{...}`, return 1 and set *end past the '}' and *n to
   the number of characters inside; else return 0. */
static int tiny_group(const char *s, const char **end, int *n)
{
    if((s[0] != '_' && s[0] != '^') || s[1] != '{') return 0;
    const char *p = s + 2;
    int count = 0;
    while(*p && *p != '}' && *p != '\n' && *p != '\f') {
        decode(&p);
        count++;
    }
    if(*p != '}') return 0;
    *end = p + 1;
    *n = count;
    return 1;
}

int hg_draw_text(int x, int y, int width, int line_gap, const char *s)
{
    int cx = 0;

    while(*s && *s != '\f') {
        const char *end;
        int n;
        if(tiny_group(s, &end, &n)) {
            int sup = (s[0] == '^');
            int w = n * TINY_ADV;
            if(cx + w > width && cx > 0) {
                cx = 0;
                y += hg_height + line_gap;
            }
            const char *p = s + 2;
            for(int i = 0; i < n; i++) {
                uint32_t c = decode(&p);
                draw_tiny(x + cx, y + (sup ? 0 : hg_height - 5), c < 0x80 ? (int)c : '?');
                cx += TINY_ADV;
            }
            s = end;
            continue;
        }

        uint32_t cp = decode(&s);

        if(cp == '\n') {
            cx = 0;
            y += hg_height + line_gap;
            continue;
        }
        if(cp == '\r') continue;

        int idx = glyph_index(cp);
        if(idx < 0) continue;
        int adv = hg_adv[idx];

        if(cx + adv > width && cx > 0) {
            cx = 0;
            y += hg_height + line_gap;
            if(cp == ' ') continue; /* don't start a line with a space */
        }
        draw_glyph(x + cx, y, idx);
        cx += adv;
    }
    return y + hg_height;
}

int hg_page_count(const char *text)
{
    int n = 1;
    for(; *text; text++) if(*text == '\f') n++;
    return n;
}

const char *hg_page(const char *text, int n)
{
    while(n > 0 && *text) {
        if(*text++ == '\f') n--;
    }
    return text;
}
