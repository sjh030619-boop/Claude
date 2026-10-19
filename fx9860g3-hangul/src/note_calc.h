#ifndef NOTE_CALC_H
#define NOTE_CALC_H

#include "hangul.h"

/* Evaluate an @식 expression (UTF-8 text, see tools/calcpage.py) with the variables vcp[i] (a circled
   consonant code point) = vval[i] (decimal text, optional leading '-'). Returns CALC_OK / CALC_SYNTAX / CALC_MATH. */
int note_eval(const char *expr, const unsigned *vcp, const char (*vval)[16], int nv, double *out);

/* The calculation tab of a problem page: asks for each variable (the filled-in problem stays on screen), then shows
   the problem, the @식 results and the solution pages of `extra` (from page `extra_page`, 0-based; may be NULL)
   filled with the entered numbers. Returns on EXIT. */
void note_calc_run(const hg_pageinfo *pi, const hg_file *extra, int extra_page);

#endif
