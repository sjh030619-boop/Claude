#ifndef CALC_H
#define CALC_H

/* A small Run-Mat style calculator (linear input), following the fx-9860GIII manual:
   - calculation priority: ( ) and function arguments, x^2, ^, (-), implicit multiplication (before a
     function, "(" , pi, Ans), x and /, then + and -  (so 6/2(1+2) = 1, -2^2 = -4)
   - 10 significant digits, Norm 1 exponential switch-over (|x| < 0.01 or |x| >= 1e10 -> 1.2E+12 / 2.5E-03)
   - Ans memory; angle unit Deg / Rad (default Rad). */

enum {
    CT_0, CT_1, CT_2, CT_3, CT_4, CT_5, CT_6, CT_7, CT_8, CT_9,
    CT_DOT, CT_EXPE,                       /* "." and EXP ("E") */
    CT_ADD, CT_SUB, CT_NEG, CT_MUL, CT_DIV,
    CT_LP, CT_RP, CT_POW, CT_SQ,           /* ( ) ^ and x^2 */
    CT_SQRT, CT_SIN, CT_COS, CT_TAN, CT_LN, CT_LOG, CT_EXPF, CT_TENF,   /* each includes its "(" */
    CT_PI, CT_ANS,
    CT_COUNT
};

#define CALC_OK 0
#define CALC_SYNTAX 1
#define CALC_MATH 2

/* Evaluate the tokens t[0..n). Returns CALC_OK (result in *out) or an error code. */
int calc_eval(const unsigned char *t, int n, int deg, double ans, double *out);
/* Format a result like the calculator (Norm 1, 10 digits). out needs >= 24 bytes. */
void calc_format(double x, char *out);
/* Text shown for a token. */
const char *calc_token_text(int tok);

/* Interactive screen: returns when EXIT is pressed. */
void calc_run(void);

#endif
