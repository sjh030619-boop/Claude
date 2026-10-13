#include <gint/display.h>
#include <gint/keyboard.h>

#include "hangul.h"

void hg_plot(int x, int y)
{
    if(x >= 0 && x < 128 && y >= 0 && y < 64)
        dpixel(x, y, hg_white ? C_WHITE : C_BLACK);
}

/* One list screen. `names(i)` gives row text; the selected row is inverted. */
static void draw_list(int count, int sel, int top, const char *(*names)(int),
                      int (*counts)(int))
{
    int rows = 64 / hg_height;

    dclear(C_WHITE);
    for(int r = 0; r < rows && top + r < count; r++) {
        int i = top + r, y = r * hg_height;
        hg_white = (i == sel);
        if(hg_white) drect(0, y, 127, y + hg_height - 1, C_BLACK);
        hg_draw_text(1, y, 1000, 0, names(i));
        if(counts) dprint(116, y + (hg_height - 8) / 2 + 1, hg_white ? C_WHITE : C_BLACK, "%d", counts(i));
    }
    hg_white = 0;
    dupdate();
}

static int cur_folder;

static const char *folder_name(int i) { return hg_folders[i].name; }
static int folder_count(int i) { return hg_folders[i].count; }
static const char *file_name(int i) { return hg_folders[cur_folder].files[i].name; }

/* Returns when the user leaves the list (EXIT) or opens an item (EXE). */
static int pick(int count, int *sel, int *top, const char *(*names)(int), int (*counts)(int))
{
    int rows = 64 / hg_height;

    while(1) {
        if(*sel < *top) *top = *sel;
        if(*sel >= *top + rows) *top = *sel - rows + 1;
        draw_list(count, *sel, *top, names, counts);

        int key = getkey().key;
        if(key == KEY_EXIT) return 0;
        if(key == KEY_EXE) return 1;
        if(key == KEY_DOWN && *sel < count - 1) (*sel)++;
        else if(key == KEY_UP && *sel > 0) (*sel)--;
    }
}

static void view(const hg_file *f)
{
    int page = 0, scroll = 0, footer = 0;
    int pages = hg_page_count(f->text);

    while(1) {
        dclear(C_WHITE);
        hg_draw_text(0, -scroll * hg_height, 128, 0, hg_page(f->text, page));
        if(footer) {
            drect(0, 56, 127, 63, C_WHITE);
            dprint(1, 57, C_BLACK, "%d/%d", page + 1, pages);
        }
        dupdate();

        int key = getkey().key;
        if(key == KEY_EXIT) return;
        else if(key == KEY_F1) footer = !footer;
        else if(key == KEY_RIGHT && page < pages - 1) { page++; scroll = 0; }
        else if(key == KEY_LEFT && page > 0) { page--; scroll = 0; }
        else if(key == KEY_DOWN) scroll++;
        else if(key == KEY_UP && scroll > 0) scroll--;
    }
}

int main(void)
{
    int fsel = 0, ftop = 0;

    while(pick(hg_folder_count, &fsel, &ftop, folder_name, folder_count)) {
        int sel = 0, top = 0;
        cur_folder = fsel;
        while(pick(hg_folders[cur_folder].count, &sel, &top, file_name, 0))
            view(&hg_folders[cur_folder].files[sel]);
    }
    return 1;
}
