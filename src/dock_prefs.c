#include "dock_prefs.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

enum { VERSION, SIZE, ZOOM, SCALE_PERCENT, LEVELS, EDGE, SNAPSHOT, PIP,
       PIP_X_OFFSET, PIP_Y_OFFSET, PATHS, FOLDER, SORTS, CLICKS, FIELD_COUNT };

static const char *const keys[FIELD_COUNT] = {
    "version", "size", "zoom", "scale_percent", "levels", "edge", "snapshot", "pip",
    "pip_x_offset", "pip_y_offset", "paths", "folder", "sorts", "clicks"
};

/* "sorts" and "clicks" may come before "paths": their entries wait here
   until the whole object is read. */
typedef struct {
    char path[DOCK_PREFS_PATH_CAP];
    int value;
} PendingEntry;
typedef struct {
    PendingEntry entry[DOCK_MAX_ITEMS];
    int count;
} PendingMap;
typedef struct { PendingMap sorts, clicks; } PendingMaps;
/* A map's value for a key; 0 for a key this build does not know. */
typedef int (*ParseKey)(const char *key, int *value);

static const char *const click_keys[DOCK_CLICK_COUNT] = {"stack", "folder"};

static const char *const edge_names[] = {"bottom", "left", "right"};

DockClick dock_click_default(const char *path)
{
    size_t length = strlen(path);
    return length && path[length-1] == ':' ? DOCK_CLICK_FOLDER : DOCK_CLICK_STACK;
}

void dock_prefs_defaults(DockPrefs *prefs)
{
    DockSettings defaults = dock_settings_default();
    memset(prefs, 0, sizeof(*prefs));
    prefs->size = defaults.base_size;
    prefs->zoom = defaults.max_size;
    prefs->levels = defaults.levels;
    prefs->edge = DOCK_EDGE_BOTTOM;
    prefs->pip_x_offset = 17;
    prefs->pip_y_offset = -1;
}

static int valid_prefs(const DockPrefs *prefs)
{
    DockSettings settings = dock_settings_default();
    int i;
    settings.base_size = prefs->size;
    settings.max_size = prefs->zoom;
    settings.levels = prefs->levels;
    if (!dock_settings_valid(&settings) ||
        (prefs->scale_percent && dock_scaled_size(prefs->size,prefs->scale_percent) != prefs->zoom) ||
        (unsigned)prefs->edge >= sizeof(edge_names)/sizeof(*edge_names) ||
        prefs->pip_x_offset < -128 || prefs->pip_x_offset > 128 ||
        prefs->pip_y_offset < -128 || prefs->pip_y_offset > 128 ||
        (unsigned)prefs->count > DOCK_MAX_ITEMS ||
        !memchr(prefs->folder,0,sizeof(prefs->folder))) return 0;
    for (i = 0; i < prefs->count; ++i)
        if (!prefs->paths[i][0] ||
            !memchr(prefs->paths[i], 0, DOCK_PREFS_PATH_CAP) ||
            (unsigned)prefs->sorts[i] >= DOCK_SORT_COUNT ||
            (unsigned)prefs->clicks[i] >= DOCK_CLICK_COUNT) return 0;
    return 1;
}

static int next_token(FILE *file)
{
    int c;
    do c = fgetc(file); while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
    return c;
}

static int hex_digit(int c)
{
    if (c >= '0' && c <= '9') return c-'0';
    if (c >= 'a' && c <= 'f') return c-'a'+10;
    if (c >= 'A' && c <= 'F') return c-'A'+10;
    return -1;
}

static int read_escape(FILE *file)
{
    int i, value = 0, c = fgetc(file);
    switch (c) {
    case '"': case '\\': case '/': return c;
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'u':
        for (i = 0; i < 4; ++i) {
            int digit = hex_digit(fgetc(file));
            if (digit < 0) return -1;
            value = value*16+digit;
        }
        return value > 0 && value <= 255 ? value : -1;
    default: return -1;
    }
}

/* Amiga path names are byte strings. Encode them as Latin-1 code points
   and accept both JSON escapes and their literal UTF-8 spelling. */
static int read_string(FILE *file, char *buffer, size_t capacity)
{
    size_t length = 0;
    int c;
    if (next_token(file) != '"') return 0;
    while ((c = fgetc(file)) != '"') {
        if (c == '\\') c = read_escape(file);
        else if (c == 0xc2 || c == 0xc3) {
            int tail = fgetc(file);
            if (tail < 0x80 || tail > 0xbf) return 0;
            c = (c-0xc2)*64+tail;
        } else if (c < 32 || c >= 0x80) return 0;
        if (c <= 0 || length+1 >= capacity) return 0;
        buffer[length++] = (char)c;
    }
    buffer[length] = 0;
    return 1;
}

static int read_integer(FILE *file, int *result)
{
    int c = next_token(file), negative = c == '-';
    unsigned int value = 0, limit = (unsigned int)INT_MAX+negative;
    if (negative) c = fgetc(file);
    if (c < '0' || c > '9') return 0;
    if (c == '0') c = fgetc(file);
    else do {
        unsigned int digit = (unsigned int)(c-'0');
        if (value > (limit-digit)/10) return 0;
        value = value*10+digit;
        c = fgetc(file);
    } while (c >= '0' && c <= '9');
    if (c != EOF && ungetc(c,file) == EOF) return 0;
    if (negative) *result = value == limit ? INT_MIN : -(int)value;
    else *result = (int)value;
    return 1;
}

static int read_boolean(FILE *file, int *result)
{
    int c = next_token(file);
    const char *tail;
    if (c == 't') tail = "rue";
    else if (c == 'f') tail = "alse";
    else return 0;
    while (*tail) if (fgetc(file) != *tail++) return 0;
    *result = c == 't';
    return 1;
}

static int read_edge(FILE *file, DockEdge *edge)
{
    char name[12];
    unsigned int i;
    if (!read_string(file,name,sizeof(name))) return 0;
    for (i = 0; i < sizeof(edge_names)/sizeof(*edge_names); ++i) {
        if (strcmp(name,edge_names[i])) continue;
        *edge = (DockEdge)i;
        return 1;
    }
    return 0;
}

/* A value of a key this build does not know: a newer build wrote it. */
static int skip_value(FILE *file, int depth)
{
    int c = next_token(file), close;
    if (c == '"') {
        while ((c = fgetc(file)) != '"') {
            if (c == '\\') c = fgetc(file);
            if (c == EOF) return 0;
        }
        return 1;
    }
    if (c == '[' || c == '{') {
        close = c == '[' ? ']' : '}';
        if (depth >= 8) return 0;
        c = next_token(file);
        if (c == close) return 1;
        if (c == EOF || ungetc(c,file) == EOF) return 0;
        do {
            if (close == '}' && (!skip_value(file,depth+1) || next_token(file) != ':')) return 0;
            if (!skip_value(file,depth+1)) return 0;
            c = next_token(file);
        } while (c == ',');
        return c == close;
    }
    /* A number, true, false or null. */
    if (c != '-' && !(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'z')) return 0;
    do c = fgetc(file);
    while (c == '-' || c == '+' || c == '.' || (c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'z') || c == 'E');
    return c == EOF || ungetc(c,file) != EOF;
}

static int read_paths(FILE *file, DockPrefs *prefs)
{
    int c;
    if (next_token(file) != '[') return 0;
    c = next_token(file);
    if (c == ']') return 1;
    if (c == EOF || ungetc(c,file) == EOF) return 0;
    do {
        if (prefs->count >= DOCK_MAX_ITEMS ||
            !read_string(file,prefs->paths[prefs->count],DOCK_PREFS_PATH_CAP)) return 0;
        ++prefs->count;
        c = next_token(file);
    } while (c == ',');
    return c == ']';
}

static int parse_sort(const char *key, int *value)
{
    DockSort sort;
    if (!dock_sort_parse(key,&sort)) return 0;
    *value = (int)sort;
    return 1;
}

static int parse_click(const char *key, int *value)
{
    int i;
    for (i = 0; i < DOCK_CLICK_COUNT; ++i)
        if (!strcmp(key,click_keys[i])) { *value = i; return 1; }
    return 0;
}

/* An object of path: key ("sorts", "clicks"). Keys this build does not
   know, values that are not strings, and entries past DOCK_MAX_ITEMS are
   skipped: they read as the path's default. Not an object at all: a newer
   build's format, skipped too. */
static int read_map(FILE *file, PendingMap *pending, ParseKey parse)
{
    char path[DOCK_PREFS_PATH_CAP], key[64];
    int c = next_token(file);
    if (c == EOF || ungetc(c,file) == EOF) return 0;
    if (c != '{') return skip_value(file,0);
    next_token(file);
    c = next_token(file);
    if (c == '}') return 1;
    if (c == EOF || ungetc(c,file) == EOF) return 0;
    do {
        int value;
        if (!read_string(file,path,sizeof(path)) || next_token(file) != ':') return 0;
        c = next_token(file);
        if (c == EOF || ungetc(c,file) == EOF) return 0;
        if (c != '"') {
            if (!skip_value(file,1)) return 0;
        } else if (!read_string(file,key,sizeof(key))) return 0;
        else if (parse(key,&value) && pending->count < DOCK_MAX_ITEMS) {
            strcpy(pending->entry[pending->count].path,path);
            pending->entry[pending->count++].value = value;
        }
        c = next_token(file);
    } while (c == ',');
    return c == '}';
}

static int read_object(FILE *file, DockPrefs *prefs, PendingMaps *pending)
{
    unsigned int seen = 0;
    int c;
    c = next_token(file);
    /* A UTF-8 byte order mark from an editor on another system. */
    if (c == 0xef) c = fgetc(file) == 0xbb && fgetc(file) == 0xbf ? next_token(file) : EOF;
    if (c != '{') return 0;
    do {
        char key[64];
        int field, ok = 0, version, ignored;
        if (!read_string(file,key,sizeof(key)) || next_token(file) != ':') return 0;
        for (field = 0; field < FIELD_COUNT; ++field)
            if (!strcmp(key,keys[field])) break;
        if (field == FIELD_COUNT) {
            /* Going back from a newer build must not cost the user's dock. */
            if (!skip_value(file,0)) return 0;
            c = next_token(file);
            continue;
        }
        if (seen & (1U<<field)) return 0;
        seen |= 1U<<field;
        switch (field) {
        case VERSION: ok = read_integer(file,&version) && version == 1; break;
        case SIZE: ok = read_integer(file,&prefs->size); break;
        case ZOOM: ok = read_integer(file,&prefs->zoom); break;
        case SCALE_PERCENT:
            ok = read_integer(file,&prefs->scale_percent) && prefs->scale_percent >= DOCK_SCALE_ONE;
            break;
        case LEVELS: ok = read_integer(file,&prefs->levels); break;
        case EDGE: ok = read_edge(file,&prefs->edge); break;
        /* Backends from before PiP only: read so older files load. "pip":
           true is still written, so an older build reading it keeps PiP. */
        case SNAPSHOT: case PIP: ok = read_boolean(file,&ignored); break;
        case PIP_X_OFFSET: ok = read_integer(file,&prefs->pip_x_offset); break;
        case PIP_Y_OFFSET: ok = read_integer(file,&prefs->pip_y_offset); break;
        case PATHS: ok = read_paths(file,prefs); break;
        case FOLDER: ok = read_string(file,prefs->folder,sizeof(prefs->folder)); break;
        case SORTS: ok = read_map(file,&pending->sorts,parse_sort); break;
        case CLICKS: ok = read_map(file,&pending->clicks,parse_click); break;
        }
        if (!ok) return 0;
        c = next_token(file);
    } while (c == ',');
    if (seen & (1U<<SCALE_PERCENT)) {
        if (seen & (1U<<ZOOM)) return 0;
        prefs->zoom = dock_scaled_size(prefs->size,prefs->scale_percent);
    }
    return c == '}' && (seen & (1U<<VERSION)) && next_token(file) == EOF &&
        !ferror(file) && valid_prefs(prefs);
}

static int same_path(const char *a, const char *b);

/* The value a map gives path (a path listed twice: the later entry), or
   fallback. */
static int map_value(const PendingMap *map, const char *path, int fallback)
{
    int j;
    for (j = 0; j < map->count; ++j)
        if (same_path(path,map->entry[j].path)) fallback = map->entry[j].value;
    return fallback;
}

int dock_prefs_read(FILE *file, DockPrefs *prefs)
{
    DockPrefs *parsed;
    PendingMaps *pending;
    int ok, i;
    if (!file || !prefs) return 0;
    parsed = malloc(sizeof(*parsed));
    pending = malloc(sizeof(*pending));
    ok = parsed && pending;
    if (ok) {
        dock_prefs_defaults(parsed);
        pending->sorts.count = pending->clicks.count = 0;
        ok = read_object(file,parsed,pending);
    }
    if (ok) {
        for (i = 0; i < parsed->count; ++i) {
            parsed->sorts[i] = (DockSort)map_value(&pending->sorts,parsed->paths[i],DOCK_SORT_NAME);
            parsed->clicks[i] = (DockClick)map_value(&pending->clicks,parsed->paths[i],
                dock_click_default(parsed->paths[i]));
        }
        *prefs = *parsed;
    }
    free(pending);
    free(parsed);
    return ok;
}

static int write_string(FILE *file, const char *string)
{
    const unsigned char *p = (const unsigned char *)string;
    if (fputc('"',file) == EOF) return 0;
    for (; *p; ++p) {
        if (*p < 32 || *p >= 127) {
            if (fprintf(file,"\\u%04x",(unsigned int)*p) < 0) return 0;
        } else {
            if ((*p == '"' || *p == '\\') && fputc('\\',file) == EOF) return 0;
            if (fputc(*p,file) == EOF) return 0;
        }
    }
    return fputc('"',file) != EOF;
}

/* ",\n  \"<name>\": {path: key, ...}" for the pins whose key is not NULL
   (not the default); nothing when every pin has the default, so a file
   without the map is the same as before it existed. Older builds skip
   the key. */
static int write_map(FILE *file, const DockPrefs *prefs, const char *name,
    const char *(*key)(const DockPrefs *prefs, int i))
{
    int i, written = 0;
    for (i = 0; i < prefs->count; ++i) {
        const char *value = key(prefs,i);
        if (!value) continue;
        if ((written++ ? fputs(",\n    ",file) == EOF :
                fprintf(file,",\n  \"%s\": {\n    ",name) < 0) ||
            !write_string(file,prefs->paths[i]) ||
            fprintf(file,": \"%s\"",value) < 0) return 0;
    }
    return !written || fputs("\n  }",file) != EOF;
}

static const char *sort_key_at(const DockPrefs *prefs, int i)
{
    return prefs->sorts[i] == DOCK_SORT_NAME ? NULL : dock_sort_key(prefs->sorts[i]);
}

static const char *click_key_at(const DockPrefs *prefs, int i)
{
    return prefs->clicks[i] == dock_click_default(prefs->paths[i]) ? NULL : click_keys[prefs->clicks[i]];
}

int dock_prefs_write(FILE *file, const DockPrefs *prefs)
{
    int i;
    if (!file || !prefs || !valid_prefs(prefs)) return 0;
    if (fprintf(file,
            "{\n  \"version\": 1,\n  \"size\": %d,\n  \"%s\": %d,\n  \"levels\": %d,\n"
            "  \"edge\": \"%s\",\n  \"pip\": true,\n"
            "  \"pip_x_offset\": %d,\n  \"pip_y_offset\": %d,\n",
            prefs->size,prefs->scale_percent ? "scale_percent" : "zoom",
            prefs->scale_percent ? prefs->scale_percent : prefs->zoom,prefs->levels,edge_names[prefs->edge],
            prefs->pip_x_offset,prefs->pip_y_offset) < 0) return 0;
    if (fputs("  \"paths\": [",file) == EOF) return 0;
    for (i = 0; i < prefs->count; ++i) {
        if (fputs(i ? ",\n    " : "\n    ",file) == EOF ||
            !write_string(file,prefs->paths[i])) return 0;
    }
    if (fputs(prefs->count ? "\n  ]" : "]",file) == EOF ||
        !write_map(file,prefs,"sorts",sort_key_at) ||
        !write_map(file,prefs,"clicks",click_key_at)) return 0;
    return fputs("\n}\n",file) != EOF && !ferror(file);
}

/* ASCII letters compare without case, as AmigaDOS names do. */
static int same_path(const char *a, const char *b)
{
    for (;; ++a, ++b) {
        int x = (unsigned char)*a, y = (unsigned char)*b;
        if (x >= 'A' && x <= 'Z') x += 'a'-'A';
        if (y >= 'A' && y <= 'Z') y += 'a'-'A';
        if (x != y) return 0;
        if (!x) return 1;
    }
}

int dock_prefs_migrate(DockPrefs *prefs)
{
    int i;
    if (!prefs || !prefs->folder[0]) return 0;
    for (i = 0; i < prefs->count; ++i) if (same_path(prefs->paths[i],prefs->folder)) break;
    if (i == prefs->count && prefs->count < DOCK_MAX_ITEMS) {
        memcpy(prefs->paths[prefs->count],prefs->folder,sizeof(prefs->folder));
        prefs->sorts[prefs->count] = DOCK_SORT_NAME;
        prefs->clicks[prefs->count++] = DOCK_CLICK_STACK;
    }
    memset(prefs->folder,0,sizeof(prefs->folder));
    return 1;
}
