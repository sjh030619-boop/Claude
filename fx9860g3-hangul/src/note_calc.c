#include <stddef.h>
#include <string.h>

#include <gint/display.h>
#include <gint/keyboard.h>

#include "calc.h"
#include "note_calc.h"

#define MAX_VARS 8
#define VAL_LEN 16
#define MAX_TOK 256

/* ---- expression text -> calculator tokens --------------------------------- */

static unsigned next_cp(const char **sp)
{
    const unsigned char *s = (const unsigned char *)*sp;
    unsigned c = s[0];
    int n = 0;
    if(c >= 0xF0) { c &= 7; n = 3; }
    else if(c >= 0xE0) { c &= 15; n = 2; }
    else if(c >= 0xC0) { c &= 31; n = 1; }
    for(int i = 1; i <= n && s[i]; i++) c = (c << 6) | (s[i] & 63);
    *sp += 1 + n;
    return c;
}

static int is_marker(unsigned cp) { return cp >= 0x3260 && cp <= 0x326D; }

static int word_is(const char *s, const char *w)
{
    return strncmp(s, w, strlen(w)) == 0;
}

int note_eval(const char *expr, const unsigned *vcp, const char (*vval)[VAL_LEN], int nv, double *out)
{
    unsigned char t[MAX_TOK];
    int n = 0, value_end = 0;   /* value_end: the previous token ends a value (so '-' is a subtraction) */
#define PUT(x) do { if(n >= MAX_TOK - 1) return CALC_SYNTAX; t[n++] = (x); } while(0)
    const char *s = expr;

    while(*s) {
        if(*s == ' ') { s++; continue; }
        if(*s >= '0' && *s <= '9') {
            if(s[0] == '1' && s[1] == '0' && s[2] == '^' && s[3] == '(' && !value_end) {
                PUT(CT_TENF); s += 4; value_end = 0; continue;
            }
            PUT(CT_0 + (*s - '0')); s++; value_end = 1; continue;
        }
        if(*s == '.') { PUT(CT_DOT); s++; value_end = 1; continue; }
        if(word_is(s, "sin(")) { PUT(CT_SIN); s += 4; value_end = 0; continue; }
        if(word_is(s, "cos(")) { PUT(CT_COS); s += 4; value_end = 0; continue; }
        if(word_is(s, "tan(")) { PUT(CT_TAN); s += 4; value_end = 0; continue; }
        if(word_is(s, "log(")) { PUT(CT_LOG); s += 4; value_end = 0; continue; }
        if(word_is(s, "ln(")) { PUT(CT_LN); s += 3; value_end = 0; continue; }
        if(word_is(s, "e^(")) { PUT(CT_EXPF); s += 3; value_end = 0; continue; }
        if(*s == '+') { PUT(CT_ADD); s++; value_end = 0; continue; }
        if(*s == '*') { PUT(CT_MUL); s++; value_end = 0; continue; }
        if(*s == '/') { PUT(CT_DIV); s++; value_end = 0; continue; }
        if(*s == '-') { PUT(value_end ? CT_SUB : CT_NEG); s++; value_end = 0; continue; }
        if(*s == '(') { PUT(CT_LP); s++; value_end = 0; continue; }
        if(*s == ')') { PUT(CT_RP); s++; value_end = 1; continue; }
        if(*s == '^') { PUT(CT_POW); s++; value_end = 0; continue; }

        unsigned cp = next_cp(&s);
        if(cp == 0xD7) { PUT(CT_MUL); value_end = 0; }               /* × */
        else if(cp == 0xF7) { PUT(CT_DIV); value_end = 0; }          /* ÷ */
        else if(cp == 0x2212) { PUT(value_end ? CT_SUB : CT_NEG); value_end = 0; }
        else if(cp == 0xB2) { PUT(CT_SQ); value_end = 1; }           /* ² */
        else if(cp == 0x3C0) { PUT(CT_PI); value_end = 1; }          /* π */
        else if(cp == 0x221A && *s == '(') { PUT(CT_SQRT); s++; value_end = 0; }   /* √( */
        else if(is_marker(cp)) {
            int k = 0;
            while(k < nv && vcp[k] != cp) k++;
            if(k == nv) return CALC_SYNTAX;
            PUT(CT_LP);
            for(const char *v = vval[k]; *v; v++) {
                if(*v == '-') PUT(CT_NEG);
                else if(*v == '.') PUT(CT_DOT);
                else PUT(CT_0 + (*v - '0'));
            }
            PUT(CT_RP);
            value_end = 1;
        }
        else return CALC_SYNTAX;
    }
    return calc_eval(t, n, 0, 0.0, out);
#undef PUT
}

/* ---- the calculation tab ---------------------------------------------------- */

typedef struct {
    int nv;
    unsigned cp[MAX_VARS];
    char def[MAX_VARS][VAL_LEN];
    char val[MAX_VARS][VAL_LEN];
} vars_t;

static void put_cp(char *d, unsigned cp)
{
    d[0] = 0xE0 | (cp >> 12); d[1] = 0x80 | ((cp >> 6) & 63); d[2] = 0x80 | (cp & 63); d[3] = 0;
}

/* variables and defaults from the markers in the page source */
static void read_vars(const char *src, vars_t *v)
{
    v->nv = 0;
    for(const char *s = src; *s;) {
        const char *at = s;
        unsigned cp = next_cp(&s);
        if(!is_marker(cp)) continue;
        int k = 0;
        while(k < v->nv && v->cp[k] != cp) k++;
        if(k < v->nv || v->nv >= MAX_VARS) continue;
        char *d = v->def[v->nv];
        int n = 0;
        const char *q = s;
        if(*q == '-') d[n++] = *q++;
        while(((*q >= '0' && *q <= '9') || *q == '.') && n < VAL_LEN - 1) d[n++] = *q++;
        d[n] = 0;
        (void)at;
        v->cp[v->nv] = cp;
        strcpy(v->val[v->nv], d);
        v->nv++;
    }
    /* sort by code point (㉠ first) */
    for(int i = 1; i < v->nv; i++)
        for(int j = i; j > 0 && v->cp[j] < v->cp[j - 1]; j--) {
            unsigned c = v->cp[j]; v->cp[j] = v->cp[j - 1]; v->cp[j - 1] = c;
            char tmp[VAL_LEN];
            strcpy(tmp, v->def[j]); strcpy(v->def[j], v->def[j - 1]); strcpy(v->def[j - 1], tmp);
            strcpy(tmp, v->val[j]); strcpy(v->val[j], v->val[j - 1]); strcpy(v->val[j - 1], tmp);
        }
}

/* the page source with every marker + number replaced by the entered value */
static void fill(const char *src, const vars_t *v, char *out, int max)
{
    int n = 0;
    for(const char *s = src; *s && n < max - 4;) {
        const char *at = s;
        unsigned cp = next_cp(&s);
        if(is_marker(cp)) {
            int k = 0;
            while(k < v->nv && v->cp[k] != cp) k++;
            if(*s == '-') s++;
            while((*s >= '0' && *s <= '9') || *s == '.') s++;
            if(k < v->nv) for(const char *c = v->val[k]; *c && n < max - 4; c++) out[n++] = *c;
        }
        else while(at < s && n < max - 4) out[n++] = *at++;
    }
    out[n] = 0;
}

static void prompt_screen(const vars_t *v, int k, const char *buf)
{
    char m[4], line[40];
    put_cp(m, v->cp[k]);
    dclear(C_WHITE);
    hg_draw_text(0, 0, 100, 0, m);
    hg_draw_text(hg_text_width(m) + 2, 0, 100, 0, "= ?");
    dprint(104, 2, C_BLACK, "%d/%d", k + 1, v->nv);
    strcpy(line, "원래 값: "); strcat(line, v->def[k]);
    hg_draw_text(0, 14, 128, 0, line);
    hg_draw_text(0, 28, 128, 0, ">");
    hg_draw_text(10, 28, 118, 0, buf);
    int w = hg_text_width(buf);
    drect(10 + w + 1, 36, 10 + w + 5, 37, C_BLACK);
    hg_draw_text(0, 53, 128, 0, "EXE 확인  ↑ 이전");
    dupdate();
}

/* enter the values; returns 0 on EXIT */
static int ask(vars_t *v)
{
    int k = 0;
    char buf[VAL_LEN] = "";
    while(k < v->nv) {
        prompt_screen(v, k, buf);
        int key = getkey().key, len = strlen(buf);
        int d = -1;
        switch(key) {
        case KEY_0: d = 0; break; case KEY_1: d = 1; break; case KEY_2: d = 2; break;
        case KEY_3: d = 3; break; case KEY_4: d = 4; break; case KEY_5: d = 5; break;
        case KEY_6: d = 6; break; case KEY_7: d = 7; break; case KEY_8: d = 8; break;
        case KEY_9: d = 9; break;
        }
        if(key == KEY_EXIT) return 0;
        if(d >= 0) { if(len < VAL_LEN - 2) { buf[len] = '0' + d; buf[len + 1] = 0; } }
        else if(key == KEY_DOT) { if(!strchr(buf, '.') && len < VAL_LEN - 2) { buf[len] = '.'; buf[len + 1] = 0; } }
        else if(key == KEY_NEG || key == KEY_MINUS) {
            if(buf[0] == '-') memmove(buf, buf + 1, len);
            else if(len < VAL_LEN - 2) { memmove(buf + 1, buf, len + 1); buf[0] = '-'; }
        }
        else if(key == KEY_DEL) { if(len) buf[len - 1] = 0; }
        else if(key == KEY_ACON) buf[0] = 0;
        else if(key == KEY_UP) { if(k > 0) k--; buf[0] = 0; }
        else if(key == KEY_EXE) {
            int ok = (len == 0), i;
            for(i = 0; i < len; i++) if(buf[i] >= '0' && buf[i] <= '9') ok = 1;
            if(!ok) continue;                       /* just "-" or "." */
            strcpy(v->val[k], len ? buf : v->def[k]);
            if(v->val[k][strlen(v->val[k]) - 1] == '.') v->val[k][strlen(v->val[k]) - 1] = 0;
            buf[0] = 0;
            k++;
        }
    }
    return 1;
}

static void results_screen(const hg_pageinfo *pi, const vars_t *v)
{
    int y = 0;
    dclear(C_WHITE);
    for(const char *f = pi->formulas; *f; f += strlen(f) + 1) {
        const char *eq = strchr(f, '=');
        char line[48], num[28];
        double r;
        int n = eq - f;
        if(n > 12) n = 12;
        memcpy(line, f, n);
        line[n] = 0;
        strcat(line, " = ");
        int e = note_eval(eq + 1, v->cp, v->val, v->nv, &r);
        if(e == CALC_OK) { calc_format(r, num); strcat(line, num); }
        else strcat(line, e == CALC_MATH ? "Ma ERROR" : "Syntax ERROR");
        hg_draw_text(0, y, 128, 0, line);
        y += hg_height;
    }
    dupdate();
}

void note_calc_run(const hg_pageinfo *pi)
{
    static char text[1024];
    vars_t v;
    int has_res = pi->formulas[0] != 0;

    read_vars(pi->src, &v);
    while(1) {
        if(v.nv && !ask(&v)) return;
        fill(pi->src, &v, text, sizeof text);
        int mode = 0, scroll = 0;
        while(1) {
            if(mode == 0) {
                dclear(C_WHITE);
                hg_draw_text(0, -scroll * hg_height, 128, 0, text);
                if(has_res) dprint(120, 56, C_BLACK, ">");
                dupdate();
            }
            else results_screen(pi, &v);
            int key = getkey().key;
            if(key == KEY_EXIT) return;
            if(key == KEY_F1 && v.nv) break;                      /* enter the numbers again */
            if(key == KEY_RIGHT && has_res) mode = 1;
            else if(key == KEY_LEFT) mode = 0;
            else if(key == KEY_EXE && has_res) mode = !mode;
            else if(key == KEY_DOWN && mode == 0) scroll++;
            else if(key == KEY_UP && mode == 0 && scroll > 0) scroll--;
        }
    }
}
