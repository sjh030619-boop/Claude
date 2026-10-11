#include <gint/display.h>
#include <gint/keyboard.h>

#include "hangul.h"

void hg_plot(int x, int y)
{
    if(x >= 0 && x < 128 && y >= 0 && y < 64) dpixel(x, y, C_BLACK);
}

int main(void)
{
    int page = 0;
    int scroll = 0;        /* in lines */
    int footer = 0;
    const int line_h = hg_height;

    while(1) {
        dclear(C_WHITE);
        hg_draw_text(0, -scroll * line_h, 128, 0, hg_pages[page]);

        if(footer) {
            drect(0, 56, 127, 63, C_WHITE);
            dprint(1, 57, C_BLACK, "%d/%d", page + 1, hg_page_count);
        }
        dupdate();

        int key = getkey().key;
        if(key == KEY_EXIT) break;
        else if(key == KEY_F1) footer = !footer;
        else if(key == KEY_RIGHT && page < hg_page_count - 1) { page++; scroll = 0; }
        else if(key == KEY_LEFT && page > 0) { page--; scroll = 0; }
        else if(key == KEY_DOWN) scroll++;
        else if(key == KEY_UP && scroll > 0) scroll--;
    }
    return 1;
}
