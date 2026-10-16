#include <stddef.h>

#include "calc.h"

/* ---- math without libm (the add-in links no math library) ----------------------------------- */

#define PI 3.14159265358979323846
#define LN2 0.69314718055994530942
#define LN10 2.30258509299404568402

static double fabs_(double x) { return x < 0 ? -x : x; }
static double floor_(double x)
{
    if(fabs_(x) >= 4503599627370496.0) return x;
    long long i = (long long)x;
    double d = (double)i;
    return d > x ? d - 1.0 : d;
}

static double sqrt_(double x)
{
    if(x <= 0) return 0;
    double y = x > 1 ? x : 1;
    for(int i = 0; i < 200; i++) {
        double ny = 0.5 * (y + x / y);
        if(ny == y) break;
        y = ny;
    }
    return y;
}

static double exp_(double x)
{
    double n = floor_(x / LN2 + 0.5);
    double r = x - n * LN2, term = 1, sum = 1;
    for(int k = 1; k < 30; k++) { term *= r / k; sum += term; }
    int ni = (int)n;
    while(ni > 0) { sum *= 2; ni--; }
    while(ni < 0) { sum *= 0.5; ni++; }
    return sum;
}

static double ln_(double x)      /* x > 0 */
{
    int e = 0;
    while(x > 1.5) { x *= 0.5; e++; }
    while(x < 0.75) { x *= 2; e--; }
    double s = (x - 1) / (x + 1), s2 = s * s, term = s, sum = 0;
    for(int k = 1; k < 60; k += 2) { sum += term / k; term *= s2; }
    return 2 * sum + e * LN2;
}

static double sincos_(double x, int cosine)
{
    x = x - 2 * PI * floor_(x / (2 * PI) + 0.5);          /* -pi .. pi */
    if(cosine) x = PI / 2 - x;                            /* cos x = sin(pi/2 - x) */
    x = x - 2 * PI * floor_(x / (2 * PI) + 0.5);
    if(fabs_(x) < 1e-15 || fabs_(PI - fabs_(x)) < 1e-15) return 0;   /* sin(pi) = 0 like the 15-digit calculator */
    double x2 = x * x, term = x, sum = x;
    for(int k = 1; k < 30; k++) { term *= -x2 / ((2 * k) * (2 * k + 1)); sum += term; }
    return sum;
}

/* x^y; sets *err on a math error */
static double pow_(double x, double y, int *err)
{
    if(y == floor_(y) && fabs_(y) < 1e6) {
        long long n = (long long)fabs_(y);
        double r = 1, b = x;
        while(n) { if(n & 1) r *= b; b *= b; n >>= 1; if(fabs_(r) > 1e200) break; }
        if(y < 0) { if(r == 0) { *err = 1; return 0; } r = 1 / r; }
        return r;
    }
    if(x < 0) { *err = 1; return 0; }
    if(x == 0) { if(y > 0) return 0; *err = 1; return 0; }
    return exp_(y * ln_(x));
}

/* ---- parser (recursive descent over the token array) ---------------------------------------- */

typedef struct {
    const unsigned char *t;
    int n, i, deg, err;
    double ans;
} P;

static int peek(P *p) { return p->i < p->n ? p->t[p->i] : -1; }

static double expr(P *p);

static int starts_primary(int t)
{
    switch(t) {
    case CT_LP: case CT_SQRT: case CT_SIN: case CT_COS: case CT_TAN: case CT_LN:
    case CT_LOG: case CT_EXPF: case CT_TENF: case CT_PI: case CT_ANS: return 1;
    }
    return 0;
}

static double number(P *p)
{
    double v = 0, scale = 1;
    int digits = 0, dot = 0;
    for(;;) {
        int t = peek(p);
        if(t >= CT_0 && t <= CT_9) {
            digits++;
            if(!dot) v = v * 10 + t;
            else { scale *= 0.1; v += t * scale; }
        } else if(t == CT_DOT && !dot) dot = 1;
        else break;
        p->i++;
    }
    if(!digits) { p->err = CALC_SYNTAX; return 0; }
    if(peek(p) == CT_EXPE) {
        p->i++;
        int neg = 0, e = 0, ed = 0;
        if(peek(p) == CT_NEG || peek(p) == CT_SUB) { neg = 1; p->i++; }
        while(peek(p) >= CT_0 && peek(p) <= CT_9) { e = e * 10 + peek(p); if(e > 400) e = 400; ed++; p->i++; }
        if(!ed) { p->err = CALC_SYNTAX; return 0; }
        int perr = 0;
        v *= pow_(10, neg ? -e : e, &perr);
    }
    return v;
}

static double primary(P *p)
{
    int t = peek(p);
    if(t >= CT_0 && t <= CT_9) return number(p);
    if(t == CT_DOT) return number(p);
    if(t == CT_PI) { p->i++; return PI; }
    if(t == CT_ANS) { p->i++; return p->ans; }
    if(t == CT_LP || (t >= CT_SQRT && t <= CT_TENF)) {
        p->i++;
        double v = expr(p);
        if(p->err) return 0;
        if(peek(p) == CT_RP) p->i++;                    /* a missing ")" at the end is accepted */
        double ang = p->deg ? v * PI / 180 : v;
        int err = 0;
        switch(t) {
        case CT_SQRT: if(v < 0) p->err = CALC_MATH; else v = sqrt_(v); break;
        case CT_SIN: v = sincos_(ang, 0); break;
        case CT_COS: v = sincos_(ang, 1); break;
        case CT_TAN: {
            double c = sincos_(ang, 1);
            if(p->deg) {                                 /* tan(90) etc. is an error */
                double q = v / 180 - floor_(v / 180);
                if(fabs_(q - 0.5) < 1e-12) { p->err = CALC_MATH; break; }
            }
            v = sincos_(ang, 0) / c;
            break;
        }
        case CT_LN: if(v <= 0) p->err = CALC_MATH; else v = ln_(v); break;
        case CT_LOG: if(v <= 0) p->err = CALC_MATH; else v = ln_(v) / LN10; break;
        case CT_EXPF: if(v > 230) p->err = CALC_MATH; else v = exp_(v); break;
        case CT_TENF: v = pow_(10, v, &err); if(err || fabs_(v) >= 1e100) p->err = CALC_MATH; break;
        default: break;
        }
        return v;
    }
    p->err = CALC_SYNTAX;
    return 0;
}

static double postfix(P *p)
{
    double v = primary(p);
    while(!p->err && peek(p) == CT_SQ) { p->i++; v *= v; }
    return v;
}

static double power(P *p)
{
    double v = postfix(p);
    while(!p->err && peek(p) == CT_POW) {
        p->i++;
        int neg = 0;
        if(peek(p) == CT_NEG || peek(p) == CT_SUB) { neg = 1; p->i++; }
        double e = postfix(p);
        if(p->err) return 0;
        int err = 0;
        v = pow_(v, neg ? -e : e, &err);
        if(err) p->err = CALC_MATH;
    }
    return v;
}

static double negation(P *p)
{
    if(peek(p) == CT_NEG || peek(p) == CT_SUB) { p->i++; return -negation(p); }
    return power(p);
}

static double implicit(P *p)
{
    double v = negation(p);
    while(!p->err && starts_primary(peek(p))) v *= power(p);     /* 2pi, 3(5+6), 2sin(30) */
    return v;
}

static double term(P *p)
{
    double v = implicit(p);
    while(!p->err && (peek(p) == CT_MUL || peek(p) == CT_DIV)) {
        int div = peek(p) == CT_DIV;
        p->i++;
        double r = implicit(p);
        if(p->err) return 0;
        if(div) { if(r == 0) { p->err = CALC_MATH; return 0; } v /= r; }
        else v *= r;
    }
    return v;
}

static double expr(P *p)
{
    double v = term(p);
    while(!p->err && (peek(p) == CT_ADD || peek(p) == CT_SUB)) {
        int sub = peek(p) == CT_SUB;
        p->i++;
        double r = term(p);
        v = sub ? v - r : v + r;
    }
    return v;
}

int calc_eval(const unsigned char *t, int n, int deg, double ans, double *out)
{
    P p = { t, n, 0, deg, 0, ans };
    if(n == 0) return CALC_SYNTAX;
    double v = expr(&p);
    if(!p.err && p.i < p.n) p.err = CALC_SYNTAX;          /* stray ")" or leftovers */
    if(!p.err && fabs_(v) >= 1e100) p.err = CALC_MATH;
    if(p.err) return p.err;
    *out = v;
    return CALC_OK;
}

/* ---- number display ------------------------------------------------------------------------ */

void calc_format(double x, char *out)
{
    char *o = out;
    if(x == 0) { o[0] = '0'; o[1] = 0; return; }
    if(x < 0) { *o++ = '-'; x = -x; }

    int e = 0;
    double t = x;
    while(t >= 10) { t /= 10; e++; }
    while(t < 1) { t *= 10; e--; }
    long long m = (long long)(t * 1e9 + 0.5);
    if(m >= 10000000000LL) { m = 1000000000LL; e++; }

    char d[11];
    for(int i = 9; i >= 0; i--) { d[i] = (char)('0' + m % 10); m /= 10; }
    int nd = 10;
    while(nd > 1 && d[nd - 1] == '0') nd--;

    if(e < -2 || e >= 10) {                                  /* Norm 1 -> exponential */
        *o++ = d[0];
        if(nd > 1) { *o++ = '.'; for(int i = 1; i < nd; i++) *o++ = d[i]; }
        *o++ = 'E';
        *o++ = e < 0 ? '-' : '+';
        int ae = e < 0 ? -e : e;
        *o++ = (char)('0' + ae / 10);
        *o++ = (char)('0' + ae % 10);
    } else if(e >= 0) {
        for(int i = 0; i <= e; i++) *o++ = i < nd ? d[i] : '0';
        if(nd > e + 1) { *o++ = '.'; for(int i = e + 1; i < nd; i++) *o++ = d[i]; }
    } else {
        *o++ = '0'; *o++ = '.';
        for(int i = 0; i < -e - 1; i++) *o++ = '0';
        for(int i = 0; i < nd; i++) *o++ = d[i];
    }
    *o = 0;
}

const char *calc_token_text(int tok)
{
    static const char *txt[CT_COUNT] = {
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "E",
        "+", "-", "-", "\xC3\x97", "\xC3\xB7",                 /* x (U+00D7), / (U+00F7) */
        "(", ")", "^", "\xC2\xB2",                             /* x^2 (U+00B2) */
        "\xE2\x88\x9A(", "sin(", "cos(", "tan(", "ln(", "log(", "e^(", "10^(",
        "\xCF\x80", "Ans"
    };
    return tok >= 0 && tok < CT_COUNT ? txt[tok] : "";
}
