#ifndef HANGUL_H
#define HANGUL_H

#include <stdint.h>

/* Font tables (generated: font_data.c) */
extern const int hg_height;           /* cell height in pixels */
extern const int hg_count;            /* number of glyphs */
extern const uint16_t hg_cps[];       /* sorted code points */
extern const uint8_t hg_adv[];        /* advance width per glyph */
extern const uint16_t hg_bits[];      /* hg_height rows per glyph, MSB = left */
extern const uint8_t hg_tiny[95][5];  /* 3x5 glyphs, ASCII 0x20..0x7E */

/* Notes tree (generated: notes.c). In a file's text, pages are separated by '\f'. */
typedef struct {
    const char *name;
    const char *text;
} hg_file;

typedef struct {
    const char *name;
    const hg_file *files;
    int count;
} hg_folder;

extern const hg_folder hg_folders[];
extern const int hg_folder_count;

/* Number of '\f'-separated pages in a file text (at least 1). */
int hg_page_count(const char *text);
/* Start of page n (0-based) within a file text. */
const char *hg_page(const char *text, int n);

/* Provided by the host program: set one black pixel. Must clip. */
void hg_plot(int x, int y);

/* Draw UTF-8 text in the box starting at (x, y) with the given width. Wraps at
   the width and on '\n'; stops at the end of the string or at '\f'.
   Inline `_{...}` / `^{...}` draw a sub/superscript in a 3x5 font (ASCII only). `line_gap` = extra pixels between lines. Lines whose
   y is outside [0, 64) are still laid out but not visible (hg_plot clips).
   Returns the y coordinate just below the last line. */
int hg_draw_text(int x, int y, int width, int line_gap, const char *utf8);

#endif
