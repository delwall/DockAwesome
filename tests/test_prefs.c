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
        "{\"version\":1} trailing"
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
    puts("JSON preferences: ordered paths, escapes, bounds, unknown keys and transactional read PASS");
    return 0;
}
