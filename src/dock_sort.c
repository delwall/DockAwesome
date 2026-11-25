#include "dock_sort.h"
#include <stdlib.h>
#include <string.h>

static const char *const sort_keys[DOCK_SORT_COUNT] = {"name", "date", "kind"};

static int fold(int c)
{
    /* Latin-1 lower-case letters sit 0x20 above their capitals; 0xf7 is
       the division sign and 0xff has no capital in Latin-1. */
    if ((c >= 'a' && c <= 'z') || (c >= 0xe0 && c <= 0xfe && c != 0xf7)) return c-0x20;
    return c;
}

int dock_sort_compare_names(const char *a, const char *b)
{
    const unsigned char *x = (const unsigned char *)a, *y = (const unsigned char *)b;
    for (;; ++x, ++y) {
        int p = fold(*x), q = fold(*y);
        if (p != q) return p < q ? -1 : 1;
        if (!p) return 0;
    }
}

static int compare_name(const void *left, const void *right)
{
    const DockFolderEntry *a = left, *b = right;
    int folded = dock_sort_compare_names(a->name,b->name);
    return folded ? folded : strcmp(a->name,b->name);
}

static int compare_date(const void *left, const void *right)
{
    const DockFolderEntry *a = left, *b = right;
    if (a->stamp[0] != b->stamp[0]) return a->stamp[0] > b->stamp[0] ? -1 : 1;
    if (a->stamp[1] != b->stamp[1]) return a->stamp[1] > b->stamp[1] ? -1 : 1;
    return compare_name(a,b);
}

/* The text after the last dot, unless the name starts with it: "" for
   "Shell", ".hidden" and "name.". */
static const char *extension(const char *name)
{
    const char *dot = strrchr(name,'.');
    return dot && dot != name ? dot+1 : name+strlen(name);
}

static int compare_kind(const void *left, const void *right)
{
    const DockFolderEntry *a = left, *b = right;
    int order;
    if (!a->drawer != !b->drawer) return a->drawer ? -1 : 1;
    if (!a->drawer && (order = dock_sort_compare_names(extension(a->name),extension(b->name))))
        return order;
    return compare_name(a,b);
}

void dock_sort_entries(DockFolderEntry *entries, size_t count, DockSort sort)
{
    static int (*const compare[DOCK_SORT_COUNT])(const void *, const void *) = {
        compare_name, compare_date, compare_kind
    };
    if (!entries || count < 2) return;
    if ((unsigned)sort >= DOCK_SORT_COUNT) sort = DOCK_SORT_NAME;
    qsort(entries,count,sizeof(*entries),compare[sort]);
}

const char *dock_sort_key(DockSort sort)
{
    return (unsigned)sort < DOCK_SORT_COUNT ? sort_keys[sort] : NULL;
}

int dock_sort_parse(const char *key, DockSort *sort)
{
    int i;
    for (i = 0; i < DOCK_SORT_COUNT; ++i)
        if (!strcmp(key,sort_keys[i])) { *sort = (DockSort)i; return 1; }
    return 0;
}
