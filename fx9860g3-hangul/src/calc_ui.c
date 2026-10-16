#include <stddef.h>
#include <string.h>

#include <gint/display.h>
#include <gint/keyboard.h>

#include "calc.h"
#include "fakecalc.h"
#include "hangul.h"

#define MAXT 48

/* state kept while the add-in runs: the line being edited, two finished lines, Ans, angle unit */
static unsigned char tok[MAXT];
static int ntok, cur;
static unsigned char last_tok[MAXT];
static int last_n;
#define NH 3
static char h_expr[NH][120], h_res[NH][28];
static double ans;
static int deg;
static const char *msg;

static void text_of(char *buf, int upto)
{
    buf[0] = 0;
    int n = 0;
    for(int i = 0; i < upto; i++) {
        const char *s = calc_token_text(tok[i]);
        int l = (int)strlen(s);
        if(n + l >= 118) break;
        memcpy(buf + n, s, l);
        n += l;
        buf[n] = 0;
    }
}

static void ins(int t)
{
    if(ntok >= MAXT) return;
    if(ntok == 0 && (t == CT_ADD || t == CT_SUB || t == CT_MUL || t == CT_DIV || t == CT_POW || t == CT_SQ)) {
        tok[ntok++] = CT_ANS;                      /* an operator on an empty line continues with Ans */
        cur = 1;
        if(ntok >= MAXT) return;
    }
    memmove(tok + cur + 1, tok + cur, ntok - cur);
    tok[cur++] = (unsigned char)t;
    ntok++;
}

static void del(void)
{
    if(cur == 0) return;
    memmove(tok + cur - 1, tok + cur, ntok - cur);
    cur--;
    ntok--;
}

/* Layout of the Run-Mat screen: 8 px per line (expression, result, expression, ...), the line being
   edited at y = 48, and the function-key bar (JUMP DEL >MAT MATH) at y = 56..63, copied from the
   original screenshot bitmap. The Galmuri 7 px glyphs have their ink 2 px below the cell top. */
#define INK 2

static void draw(void)
{
    char buf[130], pre[130];
    dclear(C_WHITE);
    for(int i = 0; i < NH; i++) {
        hg_draw_text(0, i * 16 - INK, 1000, 0, h_expr[i]);
        int w = hg_text_width(h_res[i]);
        hg_draw_text(128 - w, i * 16 + 8 - INK, 1000, 0, h_res[i]);
    }
    text_of(buf, ntok);
    text_of(pre, cur);
    int wc = hg_text_width(pre), off = wc > 120 ? wc - 120 : 0;
    hg_draw_text(-off, 48 - INK, 1000, 0, buf);
    if(cur == ntok) {                                          /* block cursor at the end of the line */
        for(int k = 0; k < 6; k++) { dpixel(wc - off + k, 48, C_BLACK); dpixel(wc - off + k, 54, C_BLACK); }
        for(int k = 0; k < 7; k++) { dpixel(wc - off, 48 + k, C_BLACK); dpixel(wc - off + 5, 48 + k, C_BLACK); }
    } else dline(wc - off, 48, wc - off, 54, C_BLACK);
    if(msg) {                                                   /* error text on the edited line */
        drect(128 - hg_text_width(msg) - 2, 47, 127, 55, C_WHITE);
        hg_draw_text(128 - hg_text_width(msg), 48 - INK, 1000, 0, msg);
    }
    for(int y = 56; y < 64; y++)                                /* JUMP DEL >MAT MATH */
        for(int x = 0; x < 128; x++)
            if(fakecalc_bits[y][x >> 3] & (0x80 >> (x & 7))) dpixel(x, y, C_BLACK);
    hg_tiny_text(106, 58, deg ? "DEG" : "RAD");               /* angle unit; F6 toggles */
    dupdate();
}

static void execute(void)
{
    double v;
    int r = calc_eval(tok, ntok, deg, ans, &v);
    if(r != CALC_OK) {
        msg = r == CALC_SYNTAX ? "Syntax ERROR" : "Ma ERROR";
        return;
    }
    for(int i = 0; i + 1 < NH; i++) {
        memcpy(h_expr[i], h_expr[i + 1], sizeof h_expr[i]);
        memcpy(h_res[i], h_res[i + 1], sizeof h_res[i]);
    }
    text_of(h_expr[NH - 1], ntok);
    calc_format(v, h_res[NH - 1]);
    ans = v;
    memcpy(last_tok, tok, ntok);
    last_n = ntok;
    ntok = cur = 0;
}

void calc_run(void)
{
    msg = NULL;
    for(;;) {
        draw();
        key_event_t ev = getkey();
        int sh = ev.mod && ev.shift;
        int key = ev.key;
        msg = NULL;

        switch(key) {
        case KEY_EXIT: return;
        case KEY_ACON: ntok = cur = 0; break;
        case KEY_EXE: execute(); break;
        case KEY_DEL: del(); break;
        case KEY_LEFT: if(cur > 0) cur--; break;
        case KEY_RIGHT: if(cur < ntok) cur++; break;
        case KEY_UP: memcpy(tok, last_tok, last_n); ntok = cur = last_n; break;
        case KEY_F2: del(); break;                     /* DEL on the function-key bar */
        case KEY_F6: deg = !deg; break;
        case KEY_0: ins(CT_0); break;
        case KEY_1: ins(CT_1); break;
        case KEY_2: ins(CT_2); break;
        case KEY_3: ins(CT_3); break;
        case KEY_4: ins(CT_4); break;
        case KEY_5: ins(CT_5); break;
        case KEY_6: ins(CT_6); break;
        case KEY_7: ins(CT_7); break;
        case KEY_8: ins(CT_8); break;
        case KEY_9: ins(CT_9); break;
        case KEY_DOT: ins(CT_DOT); break;
        case KEY_EXP: ins(sh ? CT_PI : CT_EXPE); break;
        case KEY_NEG: ins(sh ? CT_ANS : CT_NEG); break;
        case KEY_PLUS: ins(CT_ADD); break;
        case KEY_MINUS: ins(CT_SUB); break;
        case KEY_MUL: ins(CT_MUL); break;
        case KEY_DIV: ins(CT_DIV); break;
        case KEY_LEFTP: ins(CT_LP); break;
        case KEY_RIGHTP: ins(CT_RP); break;
        case KEY_SQUARE: ins(sh ? CT_SQRT : CT_SQ); break;
        case KEY_POWER: ins(CT_POW); break;
        case KEY_LOG: ins(sh ? CT_TENF : CT_LOG); break;
        case KEY_LN: ins(sh ? CT_EXPF : CT_LN); break;
        case KEY_SIN: ins(CT_SIN); break;
        case KEY_COS: ins(CT_COS); break;
        case KEY_TAN: ins(CT_TAN); break;
        default: break;
        }
    }
}
