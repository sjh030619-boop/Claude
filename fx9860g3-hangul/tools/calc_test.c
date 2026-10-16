/* host test for src/calc_eval.c: gcc -Isrc tools/calc_test.c src/calc_eval.c -o calc_test && ./calc_test */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "calc.h"

/* parse a test expression: digits . E + - m(×) d(÷) ( ) ^ q(²) r(√() s c t n(ln) l(log) x(e^) y(10^) p(π) a(Ans) N(neg) */
static int tokens(const char *s, unsigned char *t)
{
    int n = 0;
    for(; *s; s++) {
        int k;
        switch(*s) {
        case '.': k = CT_DOT; break; case 'E': k = CT_EXPE; break;
        case '+': k = CT_ADD; break; case '-': k = CT_SUB; break; case 'N': k = CT_NEG; break;
        case 'm': k = CT_MUL; break; case 'd': k = CT_DIV; break;
        case '(': k = CT_LP; break; case ')': k = CT_RP; break; case '^': k = CT_POW; break; case 'q': k = CT_SQ; break;
        case 'r': k = CT_SQRT; break; case 's': k = CT_SIN; break; case 'c': k = CT_COS; break; case 't': k = CT_TAN; break;
        case 'n': k = CT_LN; break; case 'l': k = CT_LOG; break; case 'x': k = CT_EXPF; break; case 'y': k = CT_TENF; break;
        case 'p': k = CT_PI; break; case 'a': k = CT_ANS; break;
        default: k = *s - '0';
        }
        t[n++] = (unsigned char)k;
    }
    return n;
}

static int fails;
static void check(const char *e, int deg, double ans, const char *want)
{
    unsigned char t[64];
    double v;
    char buf[32];
    int n = tokens(e, t), r = calc_eval(t, n, deg, ans, &v);
    if(r == CALC_OK) calc_format(v, buf); else strcpy(buf, r == CALC_SYNTAX ? "Syntax ERROR" : "Ma ERROR");
    int ok = !strcmp(buf, want);
    if(!ok) fails++;
    printf("%s %-22s = %-14s %s%s\n", ok ? "ok  " : "FAIL", e, buf, ok ? "" : "want ", ok ? "" : want);
}

int main(void)
{
    check("100d6", 0, 0, "16.66666667");
    check("200d7m14", 0, 0, "400");
    check("2+2m2", 0, 0, "6");
    check("(2+2)m2", 0, 0, "8");
    check("1d400", 0, 0, "2.5E-03");
    check("6d2(1+2)", 0, 0, "1");          /* manual: 6 / 2(1+2) = 1 */
    check("6d2p", 0, 0, "0.9549296586");   /* manual: 6 / 2pi = 0.9549296586 */
    check("N2^2", 0, 0, "-4");
    check("N2q", 0, 0, "-4");
    check("2^10", 0, 0, "1024");
    check("2^3^2", 0, 0, "64");
    check("2^N3", 0, 0, "0.125");
    check("r(2)", 0, 0, "1.414213562");
    check("r(N1)", 0, 0, "Ma ERROR");
    check("s(30)", 1, 0, "0.5");
    check("c(60)", 1, 0, "0.5");
    check("t(45)", 1, 0, "1");
    check("t(90)", 1, 0, "Ma ERROR");
    check("s(p)", 0, 0, "0");
    check("n(120)", 0, 0, "4.787491743");
    check("xn(120)", 0, 0, "120");          /* e^ln120 = 120 */
    check("l(100)", 0, 0, "2");
    check("l(1000)", 0, 0, "3");
    check("y(3)", 0, 0, "1000");
    check("1E12", 0, 0, "1E+12");
    check("1.2E+3", 0, 0, "Syntax ERROR");
    check("1.2E3", 0, 0, "1200");
    check("1.2EN3", 0, 0, "1.2E-03");
    check("123456789012", 0, 0, "1.23456789E+11");
    check("0.1+0.2", 0, 0, "0.3");
    check("1d3", 0, 0, "0.3333333333");
    check("2d3", 0, 0, "0.6666666667");
    check("5d0", 0, 0, "Ma ERROR");
    check("a+1", 0, 41, "42");
    check("3(5+6)", 0, 0, "33");
    check("2s(30)", 1, 0, "1");
    check("(1+2)(3+4)", 0, 0, "21");
    check("2m(3+4", 0, 0, "14");            /* missing ")" */
    check("2+", 0, 0, "Syntax ERROR");
    check("5)", 0, 0, "Syntax ERROR");
    check("N5+3", 0, 0, "-2");
    check("3-N2", 0, 0, "5");
    check("0.00001", 0, 0, "1E-05");
    check("1234567890", 0, 0, "1234567890");
    check("12345678901", 0, 0, "1.23456789E+10");
    check("3q+4q", 0, 0, "25");
    check("0.01", 0, 0, "0.01");
    check("0.009", 0, 0, "9E-03");
    printf("%d failures\n", fails);
    return fails != 0;
}
