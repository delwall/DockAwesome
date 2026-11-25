#include "dock_prefs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int read_text(const char *text, DockPrefs *prefs)
{
    FILE *file = tmpfile();
    int ok;
    assert(file);
    assert(fputs(text,file) != EOF);
    rewind(file);
    ok = dock_prefs_read(file,prefs);
    assert(fclose(file) == 0);
    return ok;
}

static void test_roundtrip(void)
{
    DockPrefs before, after;
    FILE *file = tmpfile();
    char json[4096];
    size_t length;
    assert(file);
    dock_prefs_defaults(&before);
    before.edge = DOCK_EDGE_RIGHT;
    before.zoom = 70;
    before.pip_x_offset = -128;
    before.pip_y_offset = 128;
    before.count = 3;
    strcpy(before.paths[0],"SYS:Tools/A quoted \"tool\"\\here");
    strcpy(before.paths[1],"RAM:control\t\x01\x7f\xe5\xff");
    memset(before.paths[2],'x',DOCK_PREFS_PATH_CAP-1);
    assert(dock_prefs_write(file,&before));
    rewind(file);
    length = fread(json,1,sizeof(json)-1,file);
    json[length] = 0;
    assert(strstr(json,"\\\"tool\\\"\\\\here"));
    assert(strstr(json,"\\u0009\\u0001\\u007f\\u00e5\\u00ff"));
    assert(!strstr(json,"\"folder\""));
    assert(!strstr(json,"snapshot") && strstr(json,"\"pip\": true"));
    rewind(file);
    assert(dock_prefs_read(file,&after));
    assert(!memcmp(&before,&after,sizeof(before)));
    assert(fclose(file) == 0);
}

static void test_edited_file(void)
{
    DockPrefs prefs;
    assert(read_text(" {\n  \"paths\" : [ \"RAM:\xc3\xa5\", \"SYS:Prefs\\/\" ],"
        "\r\n\"edge\":\"left\", \"pip\":true, \"version\": 1 }\t\n",&prefs));
    assert(prefs.count == 2);
    assert(!strcmp(prefs.paths[0],"RAM:\xe5"));
    assert(!strcmp(prefs.paths[1],"SYS:Prefs/"));
    assert(prefs.edge == DOCK_EDGE_LEFT);
    assert(prefs.size == dock_settings_default().base_size);
    assert(read_text("{\"version\":1,\"paths\":[]}",&prefs));
    assert(prefs.count == 0 && !prefs.folder[0]);
    /* Files from before PiP only still load; their backend keys are ignored. */
    assert(read_text("{\"version\":1,\"snapshot\":true,\"pip\":true,\"edge\":\"right\"}",&prefs));
    assert(prefs.edge == DOCK_EDGE_RIGHT);
    /* A 1.0 beta 5 file with its Folder Stack still parses. */
    assert(read_text("{\"version\":1,\"folder\":\"SYS:My \\\"folder\\\"/F\\u00f6r\",\"paths\":[\"SYS:One\"]}",&prefs));
    assert(!strcmp(prefs.folder,"SYS:My \"folder\"/F\xf6r") && prefs.count == 1);
}

static int written_has_folder(const DockPrefs *prefs)
{
    FILE *file = tmpfile();
    char json[4096];
    size_t length;
    assert(file);
    assert(dock_prefs_write(file,prefs));
    rewind(file);
    length = fread(json,1,sizeof(json)-1,file);
    json[length] = 0;
    assert(fclose(file) == 0);
    return strstr(json,"\"folder\"") != NULL;
}

/* The old Folder Stack becomes the last pin unless it is one already;
   "folder" is cleared and never written again. */
static void test_folder_migration(void)
{
    DockPrefs prefs;
    int i;
    assert(read_text("{\"version\":1,\"folder\":\"Work:Stack\",\"paths\":[\"SYS:One\",\"SYS:Two\"]}",&prefs));
    assert(written_has_folder(&prefs) == 0);
    assert(dock_prefs_migrate(&prefs) == 1);
    assert(prefs.count == 3 && !strcmp(prefs.paths[2],"Work:Stack") && !prefs.folder[0]);
    assert(!strcmp(prefs.paths[0],"SYS:One") && !strcmp(prefs.paths[1],"SYS:Two"));
    assert(!written_has_folder(&prefs));
    assert(dock_prefs_migrate(&prefs) == 0 && prefs.count == 3);
    /* Already a pin, in another case: not added twice. */
    assert(read_text("{\"version\":1,\"folder\":\"work:STACK\",\"paths\":[\"Work:Stack\",\"SYS:Two\"]}",&prefs));
    assert(dock_prefs_migrate(&prefs) == 1);
    assert(prefs.count == 2 && !strcmp(prefs.paths[0],"Work:Stack") && !prefs.folder[0]);
    /* No folder: nothing to do. */
    assert(read_text("{\"version\":1,\"paths\":[\"SYS:One\"]}",&prefs));
    assert(dock_prefs_migrate(&prefs) == 0 && prefs.count == 1 && !prefs.folder[0]);
    assert(read_text("{\"version\":1,\"folder\":\"\",\"paths\":[]}",&prefs));
    assert(dock_prefs_migrate(&prefs) == 0 && prefs.count == 0);
    /* A full dock has no room: the folder is dropped, the pins stay. */
    dock_prefs_defaults(&prefs);
    for (i = 0; i < DOCK_MAX_ITEMS; ++i) snprintf(prefs.paths[i],sizeof(prefs.paths[i]),"SYS:Pin%d",i);
    prefs.count = DOCK_MAX_ITEMS;
    strcpy(prefs.folder,"Work:Stack");
    assert(dock_prefs_migrate(&prefs) == 1);
    assert(prefs.count == DOCK_MAX_ITEMS && !prefs.folder[0]);
    assert(!strcmp(prefs.paths[DOCK_MAX_ITEMS-1],"SYS:Pin31"));
    assert(!written_has_folder(&prefs));
    assert(dock_prefs_migrate(NULL) == 0);
}

static void test_scale(void)
{
    DockPrefs before, after;
    FILE *file = tmpfile();
    assert(file);
    assert(read_text("{\"version\":1,\"scale_percent\":133,\"size\":16}",&before));
    assert(before.scale_percent == 133 && before.zoom == 21);
    assert(dock_prefs_write(file,&before));
    rewind(file);
    assert(dock_prefs_read(file,&after));
    assert(after.scale_percent == 133 && after.zoom == 21);
    assert(fclose(file) == 0);
    assert(dock_scaled_size(41,195) == 80);
    assert(dock_scaled_size(50,195) == 98);
    assert(dock_scaled_size(96,200) == 192);
}

static void written_json(const DockPrefs *prefs, char *json, size_t capacity)
{
    FILE *file = tmpfile();
    size_t length;
    assert(file);
    assert(dock_prefs_write(file,prefs));
    rewind(file);
    length = fread(json,1,capacity-1,file);
    json[length] = 0;
    assert(fclose(file) == 0);
}

/* Each stack's sort is kept by its path: only stacks not sorted by name
   are written, files without "sorts" read as name, and the order of
   "paths" (a reorder, a removal, an edit by hand) never moves a sort to
   another stack. */
static void test_sorts(void)
{
    DockPrefs before, after;
    char json[4096];
    FILE *file;
    dock_prefs_defaults(&before);
    before.count = 3;
    strcpy(before.paths[0],"Work:Downloads");
    strcpy(before.paths[1],"SYS:Prefs");
    strcpy(before.paths[2],"Work:Games \"new\"");
    /* All by name: byte for byte the file of 1.0 beta 5's writer. */
    written_json(&before,json,sizeof(json));
    assert(!strstr(json,"sorts"));
    assert(strstr(json,"\"Work:Games \\\"new\\\"\"\n  ]\n}\n"));
    before.sorts[0] = DOCK_SORT_DATE;
    before.sorts[2] = DOCK_SORT_KIND;
    written_json(&before,json,sizeof(json));
    assert(strstr(json,"\n  ],\n  \"sorts\": {\n    \"Work:Downloads\": \"date\",\n"
        "    \"Work:Games \\\"new\\\"\": \"kind\"\n  }\n}\n"));
    assert(!strstr(json,"\"SYS:Prefs\": "));
    file = tmpfile();
    assert(file && dock_prefs_write(file,&before));
    rewind(file);
    assert(dock_prefs_read(file,&after));
    assert(fclose(file) == 0);
    assert(!memcmp(&before,&after,sizeof(before)));
    /* "sorts" before "paths", a path in another case, a key for no pin,
       a value from a newer build, a value that is no string. */
    assert(read_text("{\"version\":1,\"sorts\":{\"work:DOWNLOADS\":\"date\",\"RAM:Gone\":\"kind\","
        "\"SYS:Prefs\":\"size\",\"Work:C\":{\"by\":\"kind\"},\"Work:D\":3},"
        "\"paths\":[\"SYS:Prefs\",\"Work:Downloads\",\"Work:C\",\"Work:D\"]}",&after));
    assert(after.count == 4);
    assert(after.sorts[0] == DOCK_SORT_NAME && after.sorts[1] == DOCK_SORT_DATE);
    assert(after.sorts[2] == DOCK_SORT_NAME && after.sorts[3] == DOCK_SORT_NAME);
    /* A path listed twice: the later entry counts. */
    assert(read_text("{\"version\":1,\"paths\":[\"Work:A\"],\"sorts\":{\"Work:A\":\"kind\",\"work:a\":\"date\"}}",&after));
    assert(after.sorts[0] == DOCK_SORT_DATE);
    assert(read_text("{\"version\":1,\"paths\":[\"Work:A\"],\"sorts\":{}}",&after));
    assert(after.sorts[0] == DOCK_SORT_NAME);
    /* A newer build's "sorts" in another shape is skipped. */
    assert(read_text("{\"version\":1,\"paths\":[\"Work:A\"],\"sorts\":[\"date\"]}",&after));
    assert(after.count == 1 && after.sorts[0] == DOCK_SORT_NAME);
    /* save_config writes the items in their new order with their sorts:
       swapped, each path keeps its own. */
    before.count = 2;
    strcpy(before.paths[0],"Work:Games");
    strcpy(before.paths[1],"Work:Downloads");
    before.sorts[0] = DOCK_SORT_KIND;
    before.sorts[1] = DOCK_SORT_DATE;
    before.sorts[2] = DOCK_SORT_NAME;
    written_json(&before,json,sizeof(json));
    assert(read_text(json,&after));
    assert(!strcmp(after.paths[1],"Work:Downloads") && after.sorts[1] == DOCK_SORT_DATE);
    assert(!strcmp(after.paths[0],"Work:Games") && after.sorts[0] == DOCK_SORT_KIND);
    /* A removed stack takes its sort along; the rest keep theirs. */
    before.count = 1;
    strcpy(before.paths[0],"Work:Downloads");
    before.sorts[0] = DOCK_SORT_DATE;
    written_json(&before,json,sizeof(json));
    assert(!strstr(json,"Work:Games") && read_text(json,&after));
    assert(after.count == 1 && after.sorts[0] == DOCK_SORT_DATE);
    /* An edit by hand that reorders "paths" alone. */
    assert(read_text("{\"version\":1,\"paths\":[\"Work:B\",\"Work:A\"],\"sorts\":{\"Work:A\":\"kind\"}}",&after));
    assert(after.sorts[0] == DOCK_SORT_NAME && after.sorts[1] == DOCK_SORT_KIND);
    /* The migrated Folder Stack is sorted by name; the others keep theirs. */
    assert(read_text("{\"version\":1,\"folder\":\"Work:Stack\",\"paths\":[\"Work:A\"],"
        "\"sorts\":{\"Work:A\":\"date\"}}",&after));
    after.sorts[1] = DOCK_SORT_KIND; /* stale value past count */
    assert(dock_prefs_migrate(&after) == 1);
    assert(after.count == 2 && after.sorts[0] == DOCK_SORT_DATE && after.sorts[1] == DOCK_SORT_NAME);
    /* An out-of-range sort is not written. */
    before.sorts[0] = DOCK_SORT_COUNT;
    file = tmpfile();
    assert(file && !dock_prefs_write(file,&before));
    assert(fclose(file) == 0);
}

static void test_bad_file(void)
{
    static const char *const bad[] = {
        "", "{}", "{\"version\":2}", "{\"version\":1,\"version\":1}",
        "{\"version\":1,\"paths\":[\"SYS:ok\",",
        "{\"version\":1,\"paths\":[\"SYS:ok\",]}",
        "{\"version\":1,\"paths\":[\"\"]}",
        "{\"version\":1,\"folder\":[\"SYS:One\",\"SYS:Two\"]}",
        "{\"version\":1,\"folder\":\"SYS:One\",\"folder\":\"SYS:Two\"}",
        "{\"version\":1,\"paths\":[\"\\u0000\"]}",
        "{\"version\":1,\"paths\":[\"\\ud800\"]}",
        "{\"version\":1,\"paths\":[\"\\u00gg\"]}",
        "{\"version\":1,\"paths\":[\"\xff\"]}",
        "{\"version\":1,\"paths\":[\"SYS:\nno\"]}",
        "{\"version\":1,\"size\":04}",
        "{\"version\":1,\"size\":2147483649}",
        "{\"version\":1,\"zoom\":31}",
        "{\"version\":1,\"scale_percent\":0}",
        "{\"version\":1,\"scale_percent\":99}",
        "{\"version\":1,\"size\":96,\"scale_percent\":201}",
        "{\"version\":1,\"zoom\":64,\"scale_percent\":200}",
        "{\"version\":1,\"snapshot\":1}",
        "{\"version\":1,\"pip_x_offset\":129}",
        "{\"version\":1,\"edge\":\"top\"}",
        "{\"version\":1,}",
        "{\"version\":1} trailing",
        "{\"version\":1,\"sorts\":{\"SYS:One\"}}",
        "{\"version\":1,\"sorts\":{\"SYS:One\":\"date\",}}",
        "{\"version\":1,\"sorts\":{\"SYS:One\":\"date\"},\"sorts\":{}}",
        "{\"version\":1,\"sorts\":{\"SYS:One\":\"date\"}"
    };
    DockPrefs current, original;
    char long_path[DOCK_PREFS_PATH_CAP+64];
    FILE *file;
    size_t i;
    dock_prefs_defaults(&original);
    original.count = 1;
    strcpy(original.paths[0],"SYS:Survivor");
    current = original;
    for (i = 0; i < sizeof(bad)/sizeof(*bad); ++i) {
        assert(!read_text(bad[i],&current));
        assert(!memcmp(&current,&original,sizeof(current)));
    }
    strcpy(long_path,"{\"version\":1,\"paths\":[\"");
    i = strlen(long_path);
    memset(long_path+i,'x',DOCK_PREFS_PATH_CAP);
    strcpy(long_path+i+DOCK_PREFS_PATH_CAP,"\"]}");
    assert(!read_text(long_path,&current));
    assert(!memcmp(&current,&original,sizeof(current)));

    file = tmpfile();
    assert(file);
    assert(fputs("{\"version\":1,\"paths\":[",file) != EOF);
    for (i = 0; i <= DOCK_MAX_ITEMS; ++i)
        assert(fputs(i ? ",\"SYS:Extra\"" : "\"SYS:First\"",file) != EOF);
    assert(fputs("]}",file) != EOF);
    rewind(file);
    assert(!dock_prefs_read(file,&current));
    assert(!memcmp(&current,&original,sizeof(current)));
    assert(fclose(file) == 0);
}

/* A key from a newer build is skipped with its value, whatever its type,
   and a byte order mark is accepted: neither costs the user's dock. */
static void test_unknown_keys(void)
{
    static const char *const good[] = {
        "{\"version\":1,\"future\":\"a \\\" quote \xe5\",\"size\":40,\"zoom\":80,\"paths\":[\"SYS:One\"]}",
        "{\"future\":-12.5e3,\"version\":1,\"size\":40,\"zoom\":80,\"paths\":[\"SYS:One\"]}",
        "{\"version\":1,\"size\":40,\"zoom\":80,\"paths\":[\"SYS:One\"],\"future\":[1,[true,null],{\"a\":\"b\"}]}",
        "{\"version\":1,\"a_rather_long_key_name_from_a_later_build\":{},\"b\":[],\"size\":40,\"zoom\":80,\"paths\":[\"SYS:One\"]}",
        "\xef\xbb\xbf{\"version\":1,\"size\":40,\"zoom\":80,\"paths\":[\"SYS:One\"]}"
    };
    static const char *const bad[] = {
        "{\"version\":1,\"future\":}", "{\"version\":1,\"future\":\"open}",
        "{\"version\":1,\"future\":[1,}", "{\"version\":1,\"future\":{\"a\"}}",
        "{\"version\":1,\"future\":[[[[[[[[[[1]]]]]]]]]]}", "\xef\xbb{\"version\":1}",
        "{\"version\":1,\"future\":1 2}"
    };
    DockPrefs prefs, original;
    size_t i;
    for (i = 0; i < sizeof(good)/sizeof(*good); ++i) {
        dock_prefs_defaults(&prefs);
        assert(read_text(good[i],&prefs));
        assert(prefs.size == 40 && prefs.zoom == 80 && prefs.count == 1);
        assert(!strcmp(prefs.paths[0],"SYS:One"));
    }
    dock_prefs_defaults(&original);
    prefs = original;
    for (i = 0; i < sizeof(bad)/sizeof(*bad); ++i) {
        assert(!read_text(bad[i],&prefs));
        assert(!memcmp(&prefs,&original,sizeof(prefs)));
    }
}

int main(void)
{
    test_unknown_keys();
    test_roundtrip();
    test_edited_file();
    test_scale();
    test_bad_file();
    test_folder_migration();
    test_sorts();
    puts("JSON preferences: ordered paths, escapes, bounds, unknown keys and transactional read PASS");
    return 0;
}
