#ifndef NOTE_CALC_H
#define NOTE_CALC_H

#include "hangul.h"

/* Evaluate an @식 expression (UTF-8 text, see tools/calcpage.py) with the variables vcp[i] (a circled
   consonant code point) = vval[i] (decimal text, optional leading '-'). Returns CALC_OK / CALC_SYNTAX / CALC_MATH. */
int note_eval(const char *expr, const unsigned *vcp, const char (*vval)[16], int nv, double *out);

/* The calculation tab of a problem page: asks for each variable, then shows the filled-in problem
   and the results. Returns on EXIT. */
void note_calc_run(const hg_pageinfo *pi);

#endif
