#include <gint/display.h>
#include <gint/keyboard.h>

#include "hangul.h"

#define MAX_DEPTH 6   /* nested jumps (links / F2) */
#define MAX_LINKS 8

void hg_plot(int x, int y)
{
    if(x >= 0 && x < 128 && y >= 0 && y < 64) dpixel(x, y, C_BLACK);
}

/* ---- lookups by number prefix -------------------------------------------- */

static int find_folder(int num)
{
    for(int i = 0; i < hg_folder_count; i++)
        if(hg_folders[i].num == num) return i;
    return -1;
}

static int find_file(int folder, int num)
{
    for(int i = 0; i < hg_folders[folder].count; i++)
        if(hg_folders[folder].files[i].num == num) return i;
    return -1;
}

static int digit_of(int key)
{
    switch(key) {
    case KEY_0: return 0; case KEY_1: return 1; case KEY_2: return 2;
    case KEY_3: return 3; case KEY_4: return 4; case KEY_5: return 5;
    case KEY_6: return 6; case KEY_7: return 7; case KEY_8: return 8;
    case KEY_9: return 9;
    }
    return -1;
}

/* ---- list screens --------------------------------------------------------- */

static int cur_folder;

static const char *folder_name(int i) { return hg_folders[i].name; }
static int folder_num(int i) { return hg_folders[i].num; }
static int folder_count(int i) { return hg_folders[i].count; }
static const char *file_name(int i) { return hg_folders[cur_folder].files[i].name; }
static int file_num(int i) { return hg_folders[cur_folder].files[i].num; }

typedef struct {
    int count;
    const char *(*name)(int);
    int (*num)(int);
    int (*extra)(int);   /* optional right-aligned number (file count) */
} list_t;

static void draw_list(const list_t *l, int sel, int top)
{
    int rows = 64 / hg_height;
    int name_x = 8 + hg_text_width("00") + 3;

    dclear(C_WHITE);
    for(int r = 0; r < rows && top + r < l->count; r++) {
        int i = top + r, y = r * hg_height;
        char num[3] = "";
        int n = l->num(i);
        if(n >= 0) { num[0] = '0' + (n / 10) % 10; num[1] = '0' + n % 10; num[2] = 0; }

        if(i == sel) hg_draw_text(0, y, 1000, 0, ">");
        hg_draw_text(8, y, 1000, 0, num);
        hg_draw_text(name_x, y, 1000, 0, l->name(i));
        if(l->extra) dprint(116, y + (hg_height - 8) / 2 + 1, C_BLACK, "%d", l->extra(i));
    }
    dupdate();
}

enum { PICK_BACK, PICK_OPEN, PICK_INDEX };

/* Number keys jump to the item with that number prefix (two digits in a row make
   a two-digit number). Returns on EXIT / EXE / F2. */
static int pick(const list_t *l, int *sel, int *top)
{
    int rows = 64 / hg_height;
    int pending = -1;   /* first digit typed, waiting for an optional second */

    while(1) {
        if(*sel < *top) *top = *sel;
        if(*sel >= *top + rows) *top = *sel - rows + 1;
        draw_list(l, *sel, *top);

        int key = getkey().key;
        int d = digit_of(key);

        if(d >= 0) {
            int val = (pending >= 0) ? pending * 10 + d : d;
            pending = (pending >= 0) ? -1 : d;
            for(int i = 0; i < l->count; i++)
                if(l->num(i) == val) { *sel = i; break; }
            continue;
        }
        pending = -1;

        if(key == KEY_EXIT) return PICK_BACK;
        if(key == KEY_EXE) return PICK_OPEN;
        if(key == KEY_F2) return PICK_INDEX;
        if(key == KEY_DOWN && *sel < l->count - 1) (*sel)++;
        else if(key == KEY_UP && *sel > 0) (*sel)--;
    }
}

/* ---- fake calculator screen ----------------------------------------------- */

/* Looks like an idle Run-Mat screen (status bar + empty input line). It is only a
   picture: every key except 0 is ignored, 0 returns to the page. */
static void fake_calc(void)
{
    dclear(C_WHITE);
    hg_draw_text(1, 0, 1000, 0, "Math Deg Norm1 d/c Real");
    dline(0, 10, 127, 10, C_BLACK);
    /* battery icon */
    drect(115, 2, 125, 7, C_BLACK);
    drect(116, 3, 124, 6, C_WHITE);
    drect(116, 3, 122, 6, C_BLACK);
    drect(126, 4, 127, 5, C_BLACK);
    /* input cursor */
    drect(2, 13, 3, 21, C_BLACK);
    dupdate();

    while(getkey().key != KEY_0) {}
}

/* ---- viewer / browser (mutually recursive for links and the index) -------- */

static int depth;

static void browse_files(int folder);

static void open_index(void)
{
    int f = find_folder(0);
    if(f >= 0 && depth < MAX_DEPTH) browse_files(f);
}

static void view(const hg_file *f, int page)
{
    int pages = hg_page_count(f->text);
    int scroll = 0, footer = 0, link_mode = 0, link_sel = 0;
    int pending = -1;   /* first digit of a possible two-digit page number */
    hg_link links[MAX_LINKS];
    int nlinks;

    if(page < 0 || page >= pages) page = 0;
    depth++;

    while(1) {
        const char *text = hg_page(f->text, page);
        nlinks = hg_links(text, links, MAX_LINKS);
        if(link_sel >= nlinks) link_sel = 0;

        dclear(C_WHITE);
        hg_draw_text(0, -scroll * hg_height, 128, 0, text);
        if(link_mode && nlinks) {
            int y = (links[link_sel].line - scroll) * hg_height + hg_height - 1;
            dline(0, y, 127, y, C_BLACK);
        }
        if(footer) {
            drect(0, 56, 127, 63, C_WHITE);
            dprint(1, 57, C_BLACK, "%d/%d", page + 1, pages);
        }
        dupdate();

        int key = getkey().key;

        if(link_mode) {
            if(key == KEY_UP && link_sel > 0) link_sel--;
            else if(key == KEY_DOWN && link_sel < nlinks - 1) link_sel++;
            else if(key == KEY_EXE) {
                int fo = find_folder(links[link_sel].folder);
                int fi = fo >= 0 ? find_file(fo, links[link_sel].file) : -1;
                if(fi >= 0 && depth < MAX_DEPTH)
                    view(&hg_folders[fo].files[fi], links[link_sel].page - 1);
            }
            else if(key == KEY_EXIT || key == KEY_LEFT || key == KEY_RIGHT) link_mode = 0;
            continue;
        }

        int d = digit_of(key);
        if(d >= 0) {
            if(pending >= 0) {
                int val = pending * 10 + d;   /* 10..99 (valid: set only if 10*pending <= pages) */
                pending = -1;
                if(val <= pages) { page = val - 1; scroll = 0; }
            }
            else if(d == 0) fake_calc();
            else if(d <= pages) {
                page = d - 1;
                scroll = 0;
                if(d * 10 <= pages) pending = d;   /* a two-digit page could follow */
            }
            continue;
        }
        pending = -1;

        if(key == KEY_EXIT) break;
        else if(key == KEY_F1) footer = !footer;
        else if(key == KEY_F2) open_index();
        else if(key == KEY_EXE && nlinks) { link_mode = 1; link_sel = 0; }
        else if(key == KEY_RIGHT && page < pages - 1) { page++; scroll = 0; }
        else if(key == KEY_LEFT && page > 0) { page--; scroll = 0; }
        else if(key == KEY_DOWN) scroll++;
        else if(key == KEY_UP && scroll > 0) scroll--;
    }
    depth--;
}

static void browse_files(int folder)
{
    int saved = cur_folder;
    list_t l = { hg_folders[folder].count, file_name, file_num, 0 };
    int sel = 0, top = 0, r;

    cur_folder = folder;
    depth++;
    while((r = pick(&l, &sel, &top)) != PICK_BACK) {
        if(r == PICK_INDEX) {
            int f = find_folder(0);
            if(f >= 0 && f != folder && depth < MAX_DEPTH) {
                browse_files(f);
                cur_folder = folder;   /* nested call changed it */
            }
        }
        else view(&hg_folders[folder].files[sel], 0);
    }
    depth--;
    cur_folder = saved;
}

int main(void)
{
    list_t l = { hg_folder_count, folder_name, folder_num, folder_count };
    int sel = 0, top = 0, r;

    while((r = pick(&l, &sel, &top)) != PICK_BACK) {
        if(r == PICK_INDEX) open_index();
        else browse_files(sel);
    }
    return 1;
}
