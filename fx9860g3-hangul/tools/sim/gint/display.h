/* Minimal gint stand-in so src/main.c can be built and driven on a PC. */
#ifndef SIM_GINT_DISPLAY_H
#define SIM_GINT_DISPLAY_H
#define C_WHITE 0
#define C_BLACK 1
void dclear(int color);
void dpixel(int x, int y, int color);
void drect(int x1, int y1, int x2, int y2, int color);
void dline(int x1, int y1, int x2, int y2, int color);
void dprint(int x, int y, int color, const char *fmt, ...);
void dupdate(void);
#endif
