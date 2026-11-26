#include "dock_sort.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static DockFolderEntry entry(const char *name, int drawer, unsigned long days, unsigned long ticks)
{
    DockFolderEntry e;
    memset(&e,0,sizeof(e));
    strcpy(e.name,name);
    e.drawer = drawer;
    e.stamp[0] = days;
    e.stamp[1] = ticks;
    return e;
}

static void expect_order(const DockFolderEntry *entries, size_t count, const char *const *names)
{
    size_t i;
    for (i = 0; i < count; ++i) {
        if (strcmp(entries[i].name,names[i])) {
            fprintf(stderr,"position %u: %s, expected %s\n",(unsigned)i,entries[i].name,names[i]);
            assert(0);
        }
    }
}

/* Letters fold to capitals as Stricmp does: '_' (0x5f) lies after 'Z',
   so "_tmp" follows "zebra"; Latin-1 letters fold too. */
static void test_name_fold(void)
{
    static const char *const order[] = {"Apple", "apple", "banana", "Cherry", "zebra", "_tmp", "\xc4rlig", "\xe5ra"};
    DockFolderEntry e[8];
    e[0] = entry("\xe5ra",0,0,0);
    e[1] = entry("_tmp",0,0,0);
    e[2] = entry("zebra",0,0,0);
    e[3] = entry("banana",1,0,0);
    e[4] = entry("apple",0,0,0);
    e[5] = entry("Cherry",0,0,0);
    e[6] = entry("Apple",0,0,0);
    e[7] = entry("\xc4rlig",0,0,0);
    dock_sort_entries(e,8,DOCK_SORT_NAME);
    expect_order(e,8,order);
    assert(dock_sort_compare_names("README","readme") == 0);
    assert(dock_sort_compare_names("\xe5","\xc5") == 0);
    assert(dock_sort_compare_names("\xf7","\xd7") != 0); /* division and multiplication signs */
    assert(dock_sort_compare_names("a","ab") < 0 && dock_sort_compare_names("ab","a") > 0);
    assert(dock_sort_compare_names("","") == 0);
}

/* Newest first by the entry's own date; the .info's date does not count;
   equal dates by name. */
static void test_date_order(void)
{
    static const char *const order[] = {"new", "Same.a", "same.b", "mid", "old"};
    DockFolderEntry e[5];
    e[0] = entry("old",0,100,0);
    e[1] = entry("same.b",0,200,50);
    e[2] = entry("mid",1,200,49);
    e[3] = entry("new",0,201,0);
    e[4] = entry("Same.a",0,200,50);
    e[0].stamp[2] = 9999; /* a newer icon */
    dock_sort_entries(e,5,DOCK_SORT_DATE);
    expect_order(e,5,order);
}

/* Drawers first by name, then files by extension (none first), then by name. */
static void test_kind_order(void)
{
    static const char *const order[] = {
        "Archive.lha", "Tools", "zDrawer", ".hidden", "C", "name.", "Shell",
        "A.Guide", "b.guide", "c.guide", "x.lha", "song.MOD", "a.tar.png", "pic.png"
    };
    DockFolderEntry e[14];
    e[0] = entry("pic.png",0,0,0);
    e[1] = entry("song.MOD",0,0,0);
    e[2] = entry("Shell",0,0,0);
    e[3] = entry("zDrawer",1,0,0);
    e[4] = entry("c.guide",0,0,0);
    e[5] = entry("A.Guide",0,0,0);
    e[6] = entry("x.lha",0,0,0);
    e[7] = entry("Archive.lha",1,0,0); /* a drawer stays a drawer, extension or not */
    e[8] = entry("Tools",1,0,0);
    e[9] = entry("b.guide",0,0,0);
    e[10] = entry(".hidden",0,0,0);
    e[11] = entry("C",0,0,0);
    e[12] = entry("name.",0,0,0);
    e[13] = entry("a.tar.png",0,0,0);
    dock_sort_entries(e,14,DOCK_SORT_KIND);
    expect_order(e,14,order);
}

/* The result depends only on the entries, never on their starting order
   (qsort is not stable): every permutation of a small set sorts alike. */
static void test_stable(void)
{
    DockFolderEntry base[5], work[5], first[5];
    int sort, rotation, swap;
    base[0] = entry("a.txt",0,5,1);
    base[1] = entry("A.txt",0,5,1);
    base[2] = entry("b",1,5,1);
    base[3] = entry("B.TXT",0,5,1);
    base[4] = entry("c",0,4,0);
    for (sort = 0; sort < DOCK_SORT_COUNT; ++sort) {
        memcpy(first,base,sizeof(base));
        dock_sort_entries(first,5,(DockSort)sort);
        for (rotation = 0; rotation < 5; ++rotation)
            for (swap = 0; swap < 5; ++swap) {
                int i;
                DockFolderEntry t;
                for (i = 0; i < 5; ++i) work[i] = base[(i+rotation)%5];
                t = work[0]; work[0] = work[swap]; work[swap] = t;
                dock_sort_entries(work,5,(DockSort)sort);
                for (i = 0; i < 5; ++i) assert(!strcmp(work[i].name,first[i].name));
            }
    }
    /* Byte order breaks a folded tie: "A.txt" before "a.txt". */
    memcpy(work,base,sizeof(base));
    dock_sort_entries(work,5,DOCK_SORT_NAME);
    assert(!strcmp(work[0].name,"A.txt") && !strcmp(work[1].name,"a.txt"));
    /* Out of range falls back to name; tiny inputs are left alone. */
    memcpy(first,base,sizeof(base));
    dock_sort_entries(first,5,(DockSort)DOCK_SORT_COUNT);
    for (rotation = 0; rotation < 5; ++rotation) assert(!strcmp(first[rotation].name,work[rotation].name));
    dock_sort_entries(NULL,3,DOCK_SORT_DATE);
    dock_sort_entries(work,1,DOCK_SORT_DATE);
    assert(!strcmp(work[0].name,"A.txt"));
}

static void test_keys(void)
{
    DockSort sort = DOCK_SORT_KIND;
    assert(!strcmp(dock_sort_key(DOCK_SORT_NAME),"name"));
    assert(!strcmp(dock_sort_key(DOCK_SORT_DATE),"date"));
    assert(!strcmp(dock_sort_key(DOCK_SORT_KIND),"kind"));
    assert(dock_sort_key(DOCK_SORT_COUNT) == NULL);
    assert(dock_sort_parse("date",&sort) && sort == DOCK_SORT_DATE);
    assert(dock_sort_parse("name",&sort) && sort == DOCK_SORT_NAME);
    assert(!dock_sort_parse("Date",&sort) && sort == DOCK_SORT_NAME);
    assert(!dock_sort_parse("size",&sort) && !dock_sort_parse("",&sort));
}

int main(void)
{
    test_name_fold();
    test_date_order();
    test_kind_order();
    test_stable();
    test_keys();
    puts("dock sort tests passed");
    return 0;
}
