/* Host-side check of the font + browser layout: renders the folder list, a file
   list and the first page of every file into 128x64 PBMs (no calculator needed).
     gcc -Isrc tools/host_test.c src/hangul.c src/font_data.c src/notes.c -o host_test
     ./host_test <outdir>        # then: python3 tools/preview.py <outdir> out.png */
#include <stdio.h>
#include "hangul.h"

static unsigned char fb[64][128];

void hg_plot(int x, int y)
{
    if(x >= 0 && x < 128 && y >= 0 && y < 64) fb[y][x] = hg_white ? 0 : 1;
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

static void list(int n, int sel, const char *(*name)(int))
{
    clear();
    for(int i = 0; i < n && (i + 1) * hg_height <= 64; i++) {
        int y = i * hg_height;
        hg_white = (i == sel);
        if(hg_white) for(int r = 0; r < hg_height; r++) for(int x = 0; x < 128; x++) fb[y + r][x] = 1;
        hg_draw_text(1, y, 1000, 0, name(i));
    }
    hg_white = 0;
}

static int cf;
static const char *fol(int i) { return hg_folders[i].name; }
static const char *fil(int i) { return hg_folders[cf].files[i].name; }

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    char name[64];

    list(hg_folder_count, 1, fol);
    save(dir, "a_folders");
    cf = 0;
    list(hg_folders[0].count, 0, fil);
    save(dir, "b_files");
    for(int f = 0; f < hg_folder_count; f++)
        for(int i = 0; i < hg_folders[f].count; i++) {
            clear();
            hg_draw_text(0, 0, 128, 0, hg_folders[f].files[i].text);
            snprintf(name, sizeof name, "c_view_%d_%d", f, i);
            save(dir, name);
        }
    return 0;
}
