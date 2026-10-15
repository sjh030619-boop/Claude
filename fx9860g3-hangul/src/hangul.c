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

/* Combining marks drawn over the previous glyph (no advance): U+0302 hat (y + hat = y-hat), U+0304 bar. */
static int is_mark(uint32_t cp) { return cp == 0x302 || cp == 0x304; }

static void draw_mark(int x, int y, int idx, uint32_t cp)
{
    const uint16_t *rows = hg_bits + (long)idx * hg_height;
    int top = 0, adv = hg_adv[idx];
    while(top < hg_height && !rows[top]) top++;
    int mx = x + (adv > 3 ? (adv - 3) / 2 : 0);
    if(cp == 0x302) {
        int r = top - 3 < 0 ? 0 : top - 3;
        hg_plot(mx + 1, y + r);
        hg_plot(mx, y + r + 1);
        hg_plot(mx + 2, y + r + 1);
    } else {
        int r = top - 2 < 0 ? 0 : top - 2;
        for(int i = 0; i < 3; i++) hg_plot(mx + i, y + r);
    }
}

static void draw_tiny(int x, int y, int ch)
{
    if(ch < 0x20 || ch > 0x7E) return;
    if(ch >= 'a' && ch <= 'z') ch -= 32;
    const uint8_t *g = hg_tiny[ch - 0x20];
    for(int r = 0; r < 5; r++)
        for(int c = 0; c < 3; c++)
            if(g[r] & (4 >> c)) hg_plot(x + c, y + r);
}

int hg_tiny_text(int x, int y, const char *s)
{
    int w = 0;
    for(; *s; s++, w += TINY_ADV) draw_tiny(x + w, y, (uint8_t)*s);
    return w;
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

/* ---- formula lines ---------------------------------------------------------
   A source line that contains `$` is a formula line: it is FORMULA_H pixels tall (two text
   lines + a little air) and never wraps. Inside `$...$`, `{num;den}` draws a fraction (num over
   den with a bar). `_{..}` / `^{..}` are drawn in the normal 7px glyphs, 3 px lower / higher
   (inside a fraction: the small 3x5 font). `$` itself is not drawn. Layout (rows of the 22 px
   band): numerator 1..7, bar 10, denominator 12..18, base text centred on the bar. */
#define FORMULA_H 22

static int line_has_formula(const char *s)
{
    for(; *s && *s != '\n' && *s != '\f'; s++)
        if(*s == '$') return 1;
    return 0;
}

int hg_line_formula(const char *s) { return line_has_formula(s); }

/* s is just after '{'. Returns the matching '}' (or 0); *semi = the top-level ';' (or 0). */
static const char *frac_end(const char *s, const char **semi)
{
    int depth = 1;
    *semi = 0;
    for(; *s && *s != '\n' && *s != '\f'; s++) {
        if(*s == '{') depth++;
        else if(*s == '}') { if(--depth == 0) return s; }
        else if(*s == ';' && depth == 1 && !*semi) *semi = s;
    }
    return 0;
}

/* Text of [a, b) with scripts only (no fractions). big = scripts in 7px glyphs. Returns the width. */
static int run(int x, int ty, const char *a, const char *b, int big, int draw)
{
    int cx = 0, prev_x = 0, prev_idx = -1;
    while(a < b) {
        const char *end;
        int n;
        if(tiny_group(a, &end, &n) && end <= b) {
            int sup = (a[0] == '^');
            const char *p = a + 2;
            for(int i = 0; i < n; i++) {
                uint32_t c = decode(&p);
                if(big) {
                    int idx = glyph_index(c);
                    if(idx < 0) continue;
                    if(draw) draw_glyph(x + cx, ty + (sup ? -3 : 3), idx);
                    cx += hg_adv[idx];
                } else {
                    if(draw) draw_tiny(x + cx, ty + (sup ? 2 : hg_height - 5), c < 0x80 ? (int)c : '?');
                    cx += TINY_ADV;
                }
            }
            a = end;
            continue;
        }
        uint32_t cp = decode(&a);
        if(is_mark(cp)) {
            if(draw && prev_idx >= 0) draw_mark(prev_x, ty, prev_idx, cp);
            continue;
        }
        int idx = glyph_index(cp);
        if(idx < 0) continue;
        if(draw) draw_glyph(x + cx, ty, idx);
        prev_x = x + cx; prev_idx = idx;
        cx += hg_adv[idx];
    }
    return cx;
}

/* Draws (or measures, draw = 0) the formula line starting at s with its top at y. Returns the end of the line
   ('\n', '\f' or NUL) and stores the width in *width. */
static const char *formula_line(int x, int y, const char *s, int draw, int *width)
{
    int cx = 0, math = 0, ty = y + 5;

    while(*s && *s != '\n' && *s != '\f') {
        if(*s == '$') { math = !math; s++; continue; }
        if(math && *s == '{') {
            const char *semi, *close = frac_end(s + 1, &semi);
            if(close && semi) {
                int wn = run(0, 0, s + 1, semi, 0, 0), wd = run(0, 0, semi + 1, close, 0, 0);
                int w = wn > wd ? wn : wd;
                if(draw) {
                    for(int i = 0; i < w + 2; i++) hg_plot(x + cx + 1 + i, y + 10);
                    run(x + cx + 2 + (w - wn) / 2, y - 1, s + 1, semi, 0, 1);
                    run(x + cx + 2 + (w - wd) / 2, y + 10, semi + 1, close, 0, 1);
                }
                cx += w + 4;
                s = close + 1;
                continue;
            }
        }
        /* one item of base text or a script group, up to the next '$' or '{' (in math) */
        const char *e = s;
        const char *end;
        int n;
        if(tiny_group(e, &end, &n)) e = end;
        else {
            decode(&e);
            for(;;) {                       /* keep combining marks with their base glyph */
                const char *q = e;
                if(!*q || !is_mark(decode(&q))) break;
                e = q;
            }
        }
        cx += run(x + cx, ty, s, e, 1, draw);
        s = e;
    }
    *width = cx;
    return s;
}

int hg_formula_width(const char *line)
{
    int w;
    formula_line(0, 0, line, 0, &w);
    return w;
}

int hg_line_y(const char *s, int line)
{
    int y = 0;
    while(line-- > 0) {
        y += line_has_formula(s) ? FORMULA_H : hg_height;
        while(*s && *s != '\n' && *s != '\f') s++;
        if(*s != '\n') break;
        s++;
    }
    return y;
}

static int draw_impl(int x, int y, int width, int line_gap, const char *s,
                     const hg_link *links, int nlinks)
{
    int cx = 0, at_start = 1, last_h = hg_height, prev_x = 0, prev_y = 0, prev_idx = -1;

    while(*s && *s != '\f') {
        const char *end;
        int n;
        if(at_start) {
            at_start = 0;
            if(line_has_formula(s)) {
                int w;
                s = formula_line(x, y, s, 1, &w);
                last_h = FORMULA_H;
                if(*s == '\n') { s++; y += FORMULA_H + line_gap; at_start = 1; last_h = hg_height; }
                continue;
            }
        }
        for(int i = 0; i < nlinks; i++)
            if(links[i].hide && s == links[i].ts) { s = links[i].te; break; }
        if(!*s || *s == '\f') break;
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
            at_start = 1;
            continue;
        }
        if(cp == '\r') continue;
        if(is_mark(cp)) {
            if(prev_idx >= 0) draw_mark(prev_x, prev_y, prev_idx, cp);
            continue;
        }

        int idx = glyph_index(cp);
        if(idx < 0) continue;
        int adv = hg_adv[idx];

        if(cx + adv > width && cx > 0) {
            cx = 0;
            y += hg_height + line_gap;
            if(cp == ' ') continue; /* don't start a line with a space */
        }
        draw_glyph(x + cx, y, idx);
        prev_x = x + cx; prev_y = y; prev_idx = idx;
        cx += adv;
    }
    return y + last_h;
}

int hg_draw_text(int x, int y, int width, int line_gap, const char *s)
{
    return draw_impl(x, y, width, line_gap, s, 0, 0);
}

int hg_draw_text_links(int x, int y, int width, int line_gap, const char *s,
                       const hg_link *links, int nlinks)
{
    return draw_impl(x, y, width, line_gap, s, links, nlinks);
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

int hg_text_width(const char *s)
{
    int w = 0;
    while(*s && *s != '\n' && *s != '\f') {
        uint32_t cp = decode(&s);
        if(is_mark(cp)) continue;
        int idx = glyph_index(cp);
        if(idx >= 0) w += hg_adv[idx];
    }
    return w;
}

static int read_int(const char **p, int *out)
{
    if(**p < '0' || **p > '9') return 0;
    int v = 0;
    while(**p >= '0' && **p <= '9') v = v * 10 + (*(*p)++ - '0');
    *out = v;
    return 1;
}

/* Pixel width of the text in [a, b) (single line); `_{..}` / `^{..}` count 4 px per character.
   The range [skip_a, skip_b) is left out (a hidden link token). */
static int span_width(const char *a, const char *b, const char *skip_a, const char *skip_b)
{
    int w = 0;
    while(a < b) {
        const char *end;
        int n;
        if(skip_a && a == skip_a) { a = skip_b; continue; }
        if(tiny_group(a, &end, &n) && end <= b) { w += n * TINY_ADV; a = end; continue; }
        uint32_t cp = decode(&a);
        if(is_mark(cp)) continue;
        int idx = glyph_index(cp);
        if(idx >= 0) w += hg_adv[idx];
    }
    return w;
}

static int is_space(char c) { return c == ' ' || c == '\r'; }

/* Finish the links of one line: links[first..n) all sit on the line that starts at line_start
   and ends at line_end (exclusive). A line with exactly one link is selected as a whole
   (underline = the visible text of the line). If it has a title in front of the token
   (and the title is not a label ending in ':'), the "->f/n:p" token is hidden. Lines with
   several links keep per-token selection, with the token visible. */
static void finish_line(hg_link *links, int first, int n, const char *line_start, const char *line_end)
{
    if(n == first) return;
    if(n - first > 1) {
        for(int i = first; i < n; i++) {
            links[i].hide = 0;
            links[i].ux = links[i].x; links[i].uw = links[i].w;
            links[i].lw = span_width(line_start, line_end, 0, 0);
        }
        return;
    }
    hg_link *l = &links[first];
    const char *pe = l->ts;                       /* end of the title in front of the token */
    while(pe > line_start && is_space(pe[-1])) pe--;
    int title = pe > line_start;
    int label = title && pe[-1] == ':';
    const char *le = line_end;                    /* line without trailing blanks */
    while(le > l->te && is_space(le[-1])) le--;
    l->hide = title && !label;
    if(l->hide) {
        l->lw = span_width(line_start, pe, 0, 0) + span_width(l->te, le, 0, 0);
        if(le <= l->te) l->lw = span_width(line_start, pe, 0, 0);
        l->ux = 0; l->uw = l->lw;
    } else {
        const char *e = le > l->te ? le : l->te;
        l->lw = span_width(line_start, e, 0, 0);
        l->ux = 0; l->uw = l->lw;
    }
}

int hg_links(const char *s, hg_link *out, int max)
{
    int n = 0, line = 0, first = 0;
    const char *line_start = s;

    for(;;) {
        if(*s == 0 || *s == '\f' || *s == '\n') {
            finish_line(out, first, n, line_start, s);
            if(*s != '\n') break;
            line++; s++; line_start = s; first = n;
            continue;
        }
        /* U+2192 RIGHTWARDS ARROW = E2 86 92 */
        if((uint8_t)s[0] == 0xE2 && (uint8_t)s[1] == 0x86 && (uint8_t)s[2] == 0x92) {
            const char *p = s + 3;
            int f, fi, pg = 1;
            if(read_int(&p, &f) && *p == '/') {
                p++;
                if(read_int(&p, &fi)) {
                    if(*p == ':') { const char *q = p + 1; if(read_int(&q, &pg)) p = q; }
                    if(n < max) {
                        out[n].line = line; out[n].folder = f;
                        out[n].file = fi;  out[n].page = pg;
                        out[n].x = span_width(line_start, s, 0, 0);
                        out[n].w = span_width(s, p, 0, 0);
                        out[n].ts = s; out[n].te = p;
                        out[n].hide = 0;
                        n++;
                    }
                    s = p;
                    continue;
                }
            }
        }
        s++;
    }
    return n;
}
