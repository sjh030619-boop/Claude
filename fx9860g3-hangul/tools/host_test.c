/* Host-side check of the font: renders every page of every file into 128x64 PBMs.
   (For the browser UI / navigation use tools/sim/sim.c.)
     gcc -Isrc tools/host_test.c src/hangul.c src/font_data.c src/notes.c -o host_test
     ./host_test <outdir>        # then: python3 tools/preview.py <outdir> out.png */
#include <stdio.h>
#include "hangul.h"

static unsigned char fb[64][128];

void hg_plot(int x, int y)
{
    if(x >= 0 && x < 128 && y >= 0 && y < 64) fb[y][x] = 1;
}

static void clear(void) { for(int y = 0; y < 64; y++) for(int x = 0; x < 128; x++) fb[y][x] = 0; }

static void save(const char *dir, const char *name)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.pbm", dir, name);
    FILE *f = fopen(path, "w");
    fprintf(f, "P1\n128 64\n");
    for(int y = 0; y < 64; y++) {
        for(int x = 0; x < 128; x++) fputc(fb[y][x] ? '1' : '0', f);
        fputc('\n', f);
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    char name[64];

    for(int f = 0; f < hg_folder_count; f++)
        for(int i = 0; i < hg_folders[f].count; i++) {
            const char *text = hg_folders[f].files[i].text;
            for(int p = 0; p < hg_page_count(text); p++) {
                clear();
                hg_draw_text(0, 0, 128, 0, hg_page(text, p));
                snprintf(name, sizeof name, "c_view_%d_%d_%d", f, i, p);
                save(dir, name);
            }
        }
    return 0;
}
