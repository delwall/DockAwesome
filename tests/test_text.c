#include "dock_text.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int names(const char *title, const char *path)
{
    return dock_title_names_document(title,path,strlen(path));
}

/* A title names a document as a whole part between chrome labels or in
   quotes, in any case; a longer name that starts the same does not. */
static void test_title_names(void)
{
    assert(names("Work:Text/Notes.txt","Work:Text/Notes.txt"));
    assert(names("work:text/NOTES.TXT","Work:Text/Notes.txt"));
    assert(names("Editor - Work:Text/Notes.txt","Work:Text/Notes.txt"));
    assert(names("Work:Text/Notes.txt | Editor","Work:Text/Notes.txt"));
    assert(names("Editor: Work:Text/Notes.txt","Work:Text/Notes.txt"));
    assert(names("Editor  Work:Text/Notes.txt\tmodified","Work:Text/Notes.txt"));
    assert(names("\"Work:My File\" 50%","Work:My File"));
    assert(names("View - \"Work:My File\"","Work:My File"));
    /* A single space is a legal path character, not a separator. */
    assert(!names("View \"Work:My File\" 50%","Work:My File"));
    assert(names("Work:My File - Viewer","Work:My File"));
    assert(names("R\xc4KNA.txt","r\xe4kna.txt"));
    assert(!names("Work:Text/Notes.txt.bak","Work:Text/Notes.txt"));
    assert(!names("Work:Text/Notes.txtx - Editor","Work:Text/Notes.txt"));
    assert(!names("Editor","Work:Text/Notes.txt"));
    assert(!names("","Work:Text/Notes.txt"));
    assert(!names("xWork:Text/Notes.txt","Work:Text/Notes.txt"));
}

/* Only a qualified path (a volume, then the pin's file name) is offered,
   and never one that does not fit. */
static void test_title_alias(void)
{
    char candidate[32];
    size_t n = dock_title_document_alias("Editor - RAM:Copy/Notes.txt","Work:Text/Notes.txt",
        candidate,sizeof(candidate));
    assert(n == strlen("RAM:Copy/Notes.txt") && !strcmp(candidate,"RAM:Copy/Notes.txt"));
    n = dock_title_document_alias("\"RAM:My Dir/Notes.txt\" 50%","Work:Notes.txt",candidate,sizeof(candidate));
    assert(n && !strcmp(candidate,"RAM:My Dir/Notes.txt"));
    n = dock_title_document_alias("ram:notes.TXT","Work:Notes.txt",candidate,sizeof(candidate));
    assert(n && !strcmp(candidate,"ram:notes.TXT"));
    n = dock_title_document_alias("RAM:A - B/Notes.txt","Work:Notes.txt",candidate,sizeof(candidate));
    assert(n && !strcmp(candidate,"RAM:A - B/Notes.txt"));
    assert(!dock_title_document_alias("Notes.txt - Editor","Work:Notes.txt",candidate,sizeof(candidate)));
    assert(!dock_title_document_alias("RAM:Notes.txt.bak","Work:Notes.txt",candidate,sizeof(candidate)));
    assert(!dock_title_document_alias("RAM:MyNotes.txt","Work:Notes.txt",candidate,sizeof(candidate)));
    assert(!dock_title_document_alias("Editor: Notes.txt","Work:Notes.txt",candidate,sizeof(candidate)));
    assert(!dock_title_document_alias("RAM:A/Very/Long/Drawer/Path/That/Is/Long/Notes.txt","Work:Notes.txt",
        candidate,sizeof(candidate)));
    assert(!dock_title_document_alias("RAM:Notes.txt","Work:",candidate,sizeof(candidate)));
    assert(!dock_title_document_alias("","Work:Notes.txt",candidate,sizeof(candidate)));
}

/* Shell quotes: * escapes " and itself, and nothing is written past size. */
static void test_shell_quote(void)
{
    char out[32], small[8], guarded[10];
    strcpy(out,"Execute ");
    assert(dock_shell_quote(out,sizeof(out),"RAM:My \"S*\""));
    assert(!strcmp(out,"Execute \"RAM:My *\"S***\"\""));
    out[0] = 0;
    assert(dock_shell_quote(out,sizeof(out),"") && !strcmp(out,"\"\""));
    small[0] = 0;
    assert(dock_shell_quote(small,sizeof(small),"abcd") && !strcmp(small,"\"abcd\""));
    small[0] = 0;
    assert(!dock_shell_quote(small,sizeof(small),"abcdef") && !small[0]);
    /* The string already fills all but two bytes: no room for two quotes. */
    strcpy(small,"abcdef");
    assert(!dock_shell_quote(small,sizeof(small),"") && !strcmp(small,"abcdef"));
    strcpy(small,"abcde");
    assert(dock_shell_quote(small,sizeof(small),"") && !strcmp(small,"abcde\"\""));
    memset(guarded,'#',sizeof(guarded));
    guarded[0] = 0;
    assert(!dock_shell_quote(guarded,8,"\"\"\"\"\"\""));
    assert(guarded[8] == '#' && guarded[9] == '#');
}

/* Directory Opus 5 reads the path unquoted, spaces and quotes included. */
static void test_dopus_lister_new(void)
{
    char out[32], guarded[20];
    assert(dock_dopus_lister_new(out,sizeof(out),"Personal:Audio"));
    assert(!strcmp(out,"lister new Personal:Audio"));
    assert(dock_dopus_lister_new(out,sizeof(out),"RAM:T/a \"b\"") &&
        !strcmp(out,"lister new RAM:T/a \"b\""));
    assert(!dock_dopus_lister_new(out,sizeof(out),"") && !out[0]);
    assert(!dock_dopus_lister_new(out,sizeof(out),"RAM:a\nlister close all") && !out[0]);
    /* "lister new " + "RAM:abcd" + NUL is exactly 20 bytes. */
    memset(guarded,'#',sizeof(guarded));
    assert(dock_dopus_lister_new(guarded,20,"RAM:abcd") && !strcmp(guarded,"lister new RAM:abcd"));
    memset(guarded,'#',sizeof(guarded));
    assert(!dock_dopus_lister_new(guarded,19,"RAM:abcd") && !guarded[0] && guarded[19] == '#');
    memset(guarded,'#',sizeof(guarded));
    assert(!dock_dopus_lister_new(guarded,0,"RAM:") && guarded[0] == '#');
}

/* What may run as a script: text only. */
static void test_text_bytes(void)
{
    static const unsigned char hunk[] = {0,0,3,0xf3,0,0,0,0};
    static const unsigned char jpeg[] = {0xff,0xd8,0xff,0xe0,0,16,'J','F','I','F'};
    static const unsigned char script[] = "; start\nAssign T: RAM:\n\tEcho \"\xe5\"\r\n";
    static const unsigned char ansi[] = "\x1b[1mEcho\x1b[0m\n";
    assert(!dock_text_bytes(hunk,sizeof(hunk)));
    assert(!dock_text_bytes(jpeg,sizeof(jpeg)));
    assert(dock_text_bytes(script,sizeof(script)-1));
    assert(dock_text_bytes(ansi,sizeof(ansi)-1));
    assert(!dock_text_bytes(script,sizeof(script))); /* with its NUL */
    assert(!dock_text_bytes(script,0));
}

static void test_strnicmp(void)
{
    assert(!dock_strnicmp("Hello","hELLO",5));
    assert(!dock_strnicmp("Hello","help",3));
    assert(dock_strnicmp("Hello","help",4) < 0);
    assert(dock_strnicmp("b","A",1) > 0);
    assert(!dock_strnicmp("abc","abc",99));
    assert(dock_strnicmp("\xd7","\xf7",1)); /* multiply and divide signs are not a pair */
    assert(!dock_strnicmp("","x",0));
}

int main(void)
{
    test_title_names();
    test_title_alias();
    test_shell_quote();
    test_dopus_lister_new();
    test_text_bytes();
    test_strnicmp();
    puts("text rules: title matching, aliases, Shell quotes, DOpus lister paths, text detection PASS");
    return 0;
}
