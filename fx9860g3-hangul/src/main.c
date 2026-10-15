#include <stddef.h>

#include <gint/display.h>
#include <gint/keyboard.h>

#include "fakecalc.h"
#include "hangul.h"

#define MAX_DEPTH 6   /* nested jumps (links / F2) */
#define MAX_LINKS 8
#define MARK_W 6      /* gutter for the ">" marker on pages that contain links */

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

/* index folders (hg_folder.idx 1 / 2) are opened by F2 / F3 */
static int find_index(int k)
{
    for(int i = 0; i < hg_folder_count; i++)
        if(hg_folders[i].idx == k) return i;
    return -1;
}

/* which index (1 or 2) the document f is, or 0 */
static int index_k_of(const hg_file *f)
{
    for(int k = 1; k <= 2; k++) {
        int i = find_index(k);
        if(i >= 0 && hg_folders[i].count == 1 && f == &hg_folders[i].files[0]) return k;
    }
    return 0;
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

enum { PICK_BACK, PICK_OPEN, PICK_INDEX1, PICK_INDEX2, PICK_HOME };

/* Number keys jump to the item with that number prefix (two digits in a row make
   a two-digit number). Returns on EXIT or DEL (back), EXE (open), F2 / F3 (index 1 / 2), AC (home).
   At the top level DEL is ignored, so only EXIT can leave the add-in. */
static int pick(const list_t *l, int *sel, int *top, int top_level)
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
        if(key == KEY_DEL && !top_level) return PICK_BACK;
        if(key == KEY_ACON) return PICK_HOME;
        if(key == KEY_EXE) return PICK_OPEN;
        if(key == KEY_F2) return PICK_INDEX1;
        if(key == KEY_F3) return PICK_INDEX2;
        if(key == KEY_DOWN && *sel < l->count - 1) (*sel)++;
        else if(key == KEY_UP && *sel > 0) (*sel)--;
    }
}

/* ---- fake calculator screen ----------------------------------------------- */

/* A picture of an idle Run-Mat screen. Every key except 0 is ignored; 0 returns. */
static void fake_calc(void)
{
    fakecalc_draw();
    dupdate();
    while(getkey().key != KEY_0) {}
}

/* ---- viewer / browser (mutually recursive for links and the index) -------- */

static int depth;
static int go_home;   /* set by AC: every level returns until the top-level list */

static void browse_files(int folder);
static void view(const hg_file *f, int page, int *last_page);

/* An index folder holds a single merged document. F2 / F3 open it directly at the page where
   you left it; inside that index the same key goes back to page 1 (the table of contents). */
static int index_page[3];

static void open_index(int k)
{
    int f = find_index(k);
    if(f < 0 || depth >= MAX_DEPTH) return;
    if(hg_folders[f].count == 1) view(&hg_folders[f].files[0], index_page[k], &index_page[k]);
    else browse_files(f);
}

/* A page with links is a menu: ">" marks the selected link line (the whole line is underlined
   when it has one link; with several links on a line the selected token is underlined),
   UP/DOWN move between links, EXE follows the link. Other pages scroll with UP/DOWN. */
static void view(const hg_file *f, int page, int *last_page)
{
    int pages = hg_page_count(f->text);
    int ik = index_k_of(f);
    int scroll = 0, footer = 0, link_sel = 0;
    int pending = -1;   /* first digit of a possible two-digit page number */
    hg_link links[MAX_LINKS];

    if(page < 0 || page >= pages) page = 0;
    depth++;

    while(1) {
        const char *text = hg_page(f->text, page);
        int nlinks = hg_links(text, links, MAX_LINKS);
        int x0 = nlinks ? MARK_W : 0;
        if(link_sel >= nlinks) link_sel = 0;

        dclear(C_WHITE);
        hg_draw_text_links(x0, -scroll * hg_height, 128 - x0, 0, text, links, nlinks);
        if(nlinks) {
            int ly = hg_line_y(text, links[link_sel].line) - scroll * hg_height;
            hg_draw_text(0, ly, 1000, 0, ">");
            /* several links can share a line: underline the selected one */
            int ux = x0 + links[link_sel].ux, uy = ly + hg_height - 1;
            if(links[link_sel].uw > 0) dline(ux, uy, ux + links[link_sel].uw - 1, uy, C_BLACK);
        }
        /* "page/total" in the top-right corner, only if the first line leaves room for it */
        if(scroll == 0) {
            char buf[16];
            int n = 0, v;
            /* tiny itoa: page/pages */
            char tmp[8]; int k;
            v = page + 1; k = 0; do { tmp[k++] = '0' + v % 10; v /= 10; } while(v);
            while(k) buf[n++] = tmp[--k];
            buf[n++] = '/';
            v = pages; k = 0; do { tmp[k++] = '0' + v % 10; v /= 10; } while(v);
            while(k) buf[n++] = tmp[--k];
            buf[n] = 0;
            int w = n * 4;
            int w0 = hg_line_formula(text) ? hg_formula_width(text) : hg_text_width(text);   /* first line as drawn */
            for(int i = 0; i < nlinks; i++)
                if(links[i].line == 0) { w0 = links[i].lw; break; }
            if(x0 + w0 + 3 + w <= 128) hg_tiny_text(128 - w, 0, buf);
        }
        if(footer) {
            drect(0, 56, 127, 63, C_WHITE);
            dprint(1, 57, C_BLACK, "%d/%d", page + 1, pages);
        }
        dupdate();

        int key = getkey().key;

        int d = digit_of(key);
        if(d >= 0) {
            if(pending >= 0) {
                int val = pending * 10 + d;   /* 10..99 (valid: set only if 10*pending <= pages) */
                pending = -1;
                if(val <= pages) { page = val - 1; scroll = 0; link_sel = 0; }
            }
            else if(d == 0) fake_calc();
            else if(d <= pages) {
                page = d - 1;
                scroll = 0;
                link_sel = 0;
                if(d * 10 <= pages) pending = d;   /* a two-digit page could follow */
            }
            continue;
        }
        pending = -1;

        if(key == KEY_EXIT || key == KEY_DEL) break;
        else if(key == KEY_ACON) { go_home = 1; break; }
        else if(key == KEY_F1) footer = !footer;
        else if(key == KEY_F2 || key == KEY_F3) {
            int k = (key == KEY_F2) ? 1 : 2;
            if(ik == k) { page = 0; scroll = 0; link_sel = 0; }   /* back to the table of contents */
            else { open_index(k); if(go_home) break; }
        }
        else if(key == KEY_EXE && nlinks) {
            int fo = find_folder(links[link_sel].folder);
            int fi = fo >= 0 ? find_file(fo, links[link_sel].file) : -1;
            if(fi >= 0 && depth < MAX_DEPTH) {
                view(&hg_folders[fo].files[fi], links[link_sel].page - 1, NULL);
                if(go_home) break;
            }
        }
        else if(key == KEY_RIGHT && page < pages - 1) { page++; scroll = 0; link_sel = 0; }
        else if(key == KEY_LEFT && page > 0) { page--; scroll = 0; link_sel = 0; }
        else if(key == KEY_DOWN) {
            if(nlinks) { if(link_sel < nlinks - 1) link_sel++; }
            else scroll++;
        }
        else if(key == KEY_UP) {
            if(nlinks) { if(link_sel > 0) link_sel--; }
            else if(scroll > 0) scroll--;
        }
    }
    depth--;
    if(last_page) *last_page = page;
}

static void browse_files(int folder)
{
    int saved = cur_folder;
    list_t l = { hg_folders[folder].count, file_name, file_num, 0 };
    int sel = 0, top = 0, r;

    cur_folder = folder;
    depth++;
    while(1) {
        r = pick(&l, &sel, &top, 0);
        if(r == PICK_BACK) break;
        if(r == PICK_HOME) { go_home = 1; break; }
        if(r == PICK_INDEX1 || r == PICK_INDEX2) {
            int k = (r == PICK_INDEX1) ? 1 : 2;
            if(find_index(k) != folder) {
                open_index(k);
                cur_folder = folder;   /* a nested call may have changed it */
            }
        }
        else {
            view(&hg_folders[folder].files[sel], 0, NULL);
        }
        if(go_home) break;
    }
    depth--;
    cur_folder = saved;
}

int main(void)
{
    list_t l = { hg_folder_count, folder_name, folder_num, folder_count };
    int sel = 0, top = 0, r;

    while((r = pick(&l, &sel, &top, 1)) != PICK_BACK) {
        go_home = 0;
        if(r == PICK_HOME) continue;           /* already home */
        if(r == PICK_INDEX1) open_index(1);
        else if(r == PICK_INDEX2) open_index(2);
        else if(hg_folders[sel].count == 1) {
            /* a folder with one document opens it directly (an index uses its own page memory) */
            int ik = hg_folders[sel].idx;
            view(&hg_folders[sel].files[0], ik ? index_page[ik] : 0, ik ? &index_page[ik] : NULL);
        }
        else browse_files(sel);
        go_home = 0;                           /* arrived home */
    }
    return 1;
}
