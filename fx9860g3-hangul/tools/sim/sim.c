/* Drive src/main.c with scripted keys; dump every frame to PBM.
     gcc -Isrc -Itools/sim -Dmain=app_main tools/sim/sim.c src/main.c src/hangul.c \
         src/font_data.c src/notes.c -o sim
     ./sim outdir "EXE 1 2 EXE ..."          keys: EXIT EXE UP DOWN LEFT RIGHT F1 F2 F3 DEL AC 0-9
   Writes frame_000.pbm ... and prints a line per frame. Missing keys -> EXIT. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gint/display.h>
#include <gint/keyboard.h>

int app_main(void);
#undef main

static unsigned char fb[64][128];
static const char *dir = ".";
static char keys[256][16];
static int nkeys, kpos, frame;
static int exits_after_end;

void dclear(int c) { memset(fb, c ? 1 : 0, sizeof fb); }
void dpixel(int x, int y, int c) { if(x >= 0 && x < 128 && y >= 0 && y < 64) fb[y][x] = c; }
void drect(int x1, int y1, int x2, int y2, int c) {
    for(int y = y1; y <= y2; y++) for(int x = x1; x <= x2; x++) dpixel(x, y, c);
}
void dline(int x1, int y1, int x2, int y2, int c) {
    (void)y2; for(int x = x1; x <= x2; x++) dpixel(x, y1, c);   /* horizontal only */
}
void dprint(int x, int y, int c, const char *fmt, ...) {
    char buf[64]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    /* crude 4x6 block per char so the position is visible */
    for(int i = 0; buf[i]; i++) drect(x + i * 5, y, x + i * 5 + 3, y + 5, c);
}
void dupdate(void) {
    char path[512]; snprintf(path, sizeof path, "%s/frame_%03d.pbm", dir, frame++);
    FILE *f = fopen(path, "w"); fprintf(f, "P1\n128 64\n");
    for(int y = 0; y < 64; y++) { for(int x = 0; x < 128; x++) fputc(fb[y][x] ? '1' : '0', f); fputc('\n', f); }
    fclose(f);
}

static int code(const char *s) {
    static const char *n[] = {"", "EXIT", "EXE", "UP", "DOWN", "LEFT", "RIGHT", "F1", "F2",
                              "DEL", "AC", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "F3", "F6",
                              "DOT", "EXP", "NEG", "PLUS", "MINUS", "MUL", "DIV", "LP", "RP",
                              "SQUARE", "POWER", "LOG", "LN", "SIN", "COS", "TAN"};
    for(int i = 1; i < 39; i++) if(!strcmp(s, n[i])) return i;
    fprintf(stderr, "bad key %s\n", s); exit(2);
}

key_event_t getkey(void) {
    key_event_t e;
    e.mod = e.shift = e.alpha = 0;
    if(kpos < nkeys) {
        const char *k = keys[kpos++];
        if(k[0] == 'S' && k[1] == '-') { e.mod = e.shift = 1; k += 2; }     /* S-EXP = SHIFT + EXP */
        e.key = code(k);
    }
    else { e.key = KEY_EXIT; if(++exits_after_end > 50) { fprintf(stderr, "runaway\n"); exit(3); } }
    return e;
}

int main(int argc, char **argv) {
    if(argc > 1) dir = argv[1];
    if(argc > 2) {
        char *s = strdup(argv[2]);
        for(char *t = strtok(s, " "); t && nkeys < 256; t = strtok(NULL, " ")) strncpy(keys[nkeys++], t, 15);
    }
    app_main();
    printf("frames: %d, keys consumed: %d/%d\n", frame, kpos, nkeys);
    return 0;
}
