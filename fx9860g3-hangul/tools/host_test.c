/* Host-side check of the font pipeline: renders every page into a 128x64
   1-bit buffer and writes page<N>.pbm. Not part of the add-in build.
     gcc -Isrc tools/host_test.c src/hangul.c src/font_data.c src/pages.c -o host_test */
#include <stdio.h>
#include "hangul.h"

static unsigned char fb[64][128];

void hg_plot(int x, int y)
{
    if(x >= 0 && x < 128 && y >= 0 && y < 64) fb[y][x] = 1;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    for(int p = 0; p < hg_page_count; p++) {
        char name[512];
        for(int y = 0; y < 64; y++) for(int x = 0; x < 128; x++) fb[y][x] = 0;
        hg_draw_text(0, 0, 128, 0, hg_pages[p]);
        snprintf(name, sizeof name, "%s/page%d.pbm", dir, p + 1);
        FILE *f = fopen(name, "w");
        fprintf(f, "P1\n128 64\n");
        for(int y = 0; y < 64; y++) {
            for(int x = 0; x < 128; x++) fputc(fb[y][x] ? '1' : '0', f);
            fputc('\n', f);
        }
        fclose(f);
    }
    return 0;
}
