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
    int num;                 /* number prefix of the file name (01_ -> 1), or -1 */
} hg_file;

typedef struct {
    const char *name;
    const hg_file *files;
    int count;
    int num;                 /* number prefix of the folder name, or -1 */
    int idx;                 /* 0 = normal folder; 1, 2 = index folder opened by F2 / F3 */
} hg_folder;

extern const hg_folder hg_folders[];
extern const int hg_folder_count;

/* Number of '\f'-separated pages in a file text (at least 1). */
int hg_page_count(const char *text);
/* Start of page n (0-based) within a file text. */
const char *hg_page(const char *text, int n);

/* Width in pixels of a single line of text. */
int hg_text_width(const char *utf8);

/* Links inside a page: "→F/N" or "→F/N:P" (folder number, file number, optional
   1-based page). `line` = 0-based line index (count of '\n' before it). */
typedef struct {
    int line, folder, file, page;
    int x, w;           /* pixel offset of the "->F/N:P" token from the start of its line, and its width */
    const char *ts, *te;/* the token itself in the page text: [ts, te) */
    int hide;           /* 1: the token is not drawn (hg_draw_text_links skips it) */
    int ux, uw;         /* selection underline of this link: offset from the line start and width */
    int lw;             /* width of the visible line (token left out when hidden) */
} hg_link;
int hg_links(const char *page_text, hg_link *out, int max);
/* Like hg_draw_text, but skips the tokens of links whose `hide` is set. */
int hg_draw_text_links(int x, int y, int width, int line_gap, const char *utf8,
                       const hg_link *links, int nlinks);

/* ASCII text in the 3x5 font (4 px per character, lower case drawn as upper case).
   Returns the width in pixels. */
int hg_tiny_text(int x, int y, const char *ascii);

/* Provided by the host program: set one black pixel. Must clip. */
void hg_plot(int x, int y);

/* Draw UTF-8 text in the box starting at (x, y) with the given width. Wraps at
   the width and on '\n'; stops at the end of the string or at '\f'.
   Inline `_{...}` / `^{...}` draw a sub/superscript in a 3x5 font (ASCII only). `line_gap` = extra pixels between lines. Lines whose
   y is outside [0, 64) are still laid out but not visible (hg_plot clips).
   Returns the y coordinate just below the last line. */
int hg_draw_text(int x, int y, int width, int line_gap, const char *utf8);

#endif
