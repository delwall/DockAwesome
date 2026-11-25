/* Workbench-style launches for desktops that are not Workbench.
   workbench.library's OpenWorkbenchObjectA needs the Workbench process;
   Directory Opus 5 on 68k (AmiKit) replaces Workbench without patching it.
   This starts tools and projects the way Workbench does: LoadSeg, a new
   process with the icon's stack, and a WBStartup message whose reply says
   the program has ended. The program's code stays loaded until then. */
#include "amiga_app.h"
#include "dock_text.h"
#include <rexx/storage.h>
#include <proto/rexxsyslib.h>
#include "amiga_folder.h"

struct RxsLib *RexxSysBase;

enum { NAME_CAP = 108, MIN_STACK = 4096 };

struct DockLaunched {
    struct WBStartup startup; /* first: the reply is this message */
    struct WBArg args[2];
    char names[2][NAME_CAP];
    char path[PATH_CAP];      /* what the dock was asked to open */
    struct DockLaunched *next;
};

/* The file `name` names, from `dir` unless it names a volume. */
static BPTR lock_in(BPTR dir, const char *name)
{
    BPTR old = CurrentDir(dir), lock = Lock((STRPTR)name,ACCESS_READ);
    CurrentDir(old);
    return lock;
}

/* A default tool by bare name, searched as Workbench would: next to the
   project, then Workbench's search path (Directory Opus answers this on
   68k too), then the usual system drawers. */
static BPTR find_tool(BPTR project_dir, const char *tool)
{
    static const char *const fallback[] = { "C:", "SYS:Utilities", "SYS:Tools", "SYS:System" };
    BPTR found = 0, path = 0;
    struct TagItem get[] = {{WBCTRLA_DuplicateSearchPath,(ULONG)&path},{TAG_DONE,0}};
    unsigned int i;
    if (strchr(tool,':') || strchr(tool,'/') || (found = lock_in(project_dir,tool)))
        return found ? found : lock_in(project_dir,tool);
    if (WorkbenchControlA(NULL,get) && path) {
        BPTR node;
        struct TagItem release[] = {{WBCTRLA_FreeSearchPath,(ULONG)path},{TAG_DONE,0}};
        /* A BCPL path list: {next, lock} per entry. */
        for (node = path; node && !found; node = ((BPTR *)BADDR(node))[0])
            found = lock_in(((BPTR *)BADDR(node))[1],tool);
        WorkbenchControlA(NULL,release);
        if (found) return found;
    }
    for (i = 0; i < sizeof(fallback)/sizeof(fallback[0]) && !found; ++i) {
        BPTR dir = Lock((STRPTR)fallback[i],ACCESS_READ);
        if (dir) { found = lock_in(dir,tool); UnLock(dir); }
    }
    return found;
}

/* Shows or clears the running dot that our own launches give `path`. */
static int mark_items(App *app, const char *path, int running)
{
    int i, changed = 0;
    for (i = 0; i < app->count; ++i) {
        Item *item = &app->items[i];
        int self = !!(item->running & RUNNING_SELF);
        if (Stricmp((STRPTR)item->path,(STRPTR)path) || self == running) continue;
        item->running ^= RUNNING_SELF;
        changed = 1;
    }
    return changed;
}

/* ARexx to Directory Opus 5 (port DOPUS.1). Commands from the main task
   are sent without waiting; their replies arrive on launch_port. The status
   worker asks and waits for answers. */
static struct RexxMsg *rexx_message(struct MsgPort *reply, const char *command, LONG action)
{
    struct RexxMsg *msg;
    if (!RexxSysBase || !(msg = CreateRexxMsg(reply,NULL,NULL))) return NULL;
    if (!(msg->rm_Args[0] = (STRPTR)CreateArgstring((CONST_STRPTR)command,strlen(command)))) {
        DeleteRexxMsg(msg);
        return NULL;
    }
    msg->rm_Action = action;
    return msg;
}

static void rexx_free(struct RexxMsg *msg)
{
    if ((msg->rm_Action & RXFF_RESULT) && !msg->rm_Result1 && msg->rm_Result2)
        DeleteArgstring((UBYTE *)msg->rm_Result2);
    DeleteArgstring((UBYTE *)msg->rm_Args[0]);
    DeleteRexxMsg(msg);
}

static int rexx_put(struct RexxMsg *msg)
{
    struct MsgPort *port;
    Forbid();
    if ((port = amiga_caps_dopus_port())) PutMsg(port,&msg->rm_Node);
    Permit();
    return port != NULL;
}

static int rexx_send(struct MsgPort *reply, const char *command)
{
    struct RexxMsg *msg = rexx_message(reply,command,RXCOMM);
    if (msg && !rexx_put(msg)) { rexx_free(msg); msg = NULL; }
    return msg != NULL;
}

/* Sends and waits, from the status worker. The result string, if any, goes
   to `result`. Returns the ARexx return code, or -1 when Directory Opus is
   not there. Directory Opus answers at once; one that did not kept the
   worker waiting for good (no status, and a dock that could not quit), so
   the reply gets two seconds. After that the message and its port are left
   behind, ignored, and Directory Opus is not asked again. */
static LONG rexx_call(const char *command, char *result, size_t size)
{
    enum { REPLY_TICKS = 2*TICKS_PER_SECOND };
    static int silent;
    struct MsgPort *reply = silent ? NULL : CreateMsgPort();
    struct RexxMsg *msg = reply ? rexx_message(reply,command,RXCOMM|RXFF_RESULT) : NULL;
    LONG rc = -1;
    if (size) result[0] = 0;
    if (msg && rexx_put(msg)) {
        int ticks;
        struct Message *answer = NULL;
        for (ticks = 0; !(answer = GetMsg(reply)) && ticks < REPLY_TICKS; ++ticks) Delay(1);
        if (!answer) {
            Forbid();
            if (!(answer = GetMsg(reply))) {
                reply->mp_Flags = PA_IGNORE; FreeSignal(reply->mp_SigBit);
                silent = 1;
            }
            Permit();
            if (!answer) return -1;
        }
        rc = msg->rm_Result1;
        if (!rc && msg->rm_Result2 && size) {
            strncpy(result,(const char *)msg->rm_Result2,size-1);
            result[size-1] = 0;
        }
        rexx_free(msg);
    } else if (msg) rexx_free(msg);
    if (reply) DeleteMsgPort(reply);
    return rc;
}

/* Fills args[n] with the parent of `file` and its name; takes `file`. */
static int set_arg(struct DockLaunched *launch, int n, BPTR file)
{
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB,NULL);
    BPTR parent = ParentDir(file);
    int ok = fib && parent && Examine(file,fib);
    if (ok) {
        strncpy(launch->names[n],(const char *)fib->fib_FileName,NAME_CAP-1);
        launch->args[n].wa_Lock = parent;
        launch->args[n].wa_Name = (STRPTR)launch->names[n];
    } else if (parent) UnLock(parent);
    if (fib) FreeDosObject(DOS_FIB,fib);
    UnLock(file);
    return ok;
}

/* Argument n as a drawer lock (taken over) and a name in it. */
static void set_arg_named(struct DockLaunched *launch, int n, BPTR drawer, const char *name)
{
    strncpy(launch->names[n],name,NAME_CAP-1);
    launch->args[n].wa_Lock = drawer;
    launch->args[n].wa_Name = (STRPTR)launch->names[n];
}

/* The drawer path names, locked. */
static BPTR drawer_lock(const char *path)
{
    char drawer[PATH_CAP];
    size_t n = (size_t)((const char *)PathPart((STRPTR)path)-path);
    if (n >= sizeof(drawer)) return 0;
    memcpy(drawer,path,n); drawer[n] = 0;
    return Lock((STRPTR)drawer,ACCESS_READ);
}

static void launch_free(struct DockLaunched *launch)
{
    int i;
    if (launch->startup.sm_Segment) UnLoadSeg(launch->startup.sm_Segment);
    for (i = 0; i < 2; ++i) if (launch->args[i].wa_Lock) UnLock(launch->args[i].wa_Lock);
    FreeVec(launch);
}

static LONG icon_stack(const struct DiskObject *icon)
{
    return icon ? icon->do_StackSize : 0;
}

static int has_tool_type(const struct DiskObject *icon, const char *type)
{
    return icon && FindToolType((CONST_STRPTR *)icon->do_ToolTypes,(CONST_STRPTR)type) != NULL;
}

/* A "$STACK:<bytes>" string in the program's hunks, which Workbench
   honours since V45 (wb release notes, 45.165). */
static LONG embedded_stack(BPTR seglist)
{
    static const char key[] = "$STACK:";
    BPTR seg;
    for (seg = seglist; seg; seg = *(const BPTR *)BADDR(seg)) {
        const UBYTE *mem = (const UBYTE *)BADDR(seg);
        ULONG size = ((const ULONG *)mem)[-1], i;
        /* The allocation holds its size before and the next pointer at
           the start; data runs from mem+4 to the end of the allocation. */
        if (size < 16+sizeof(key)) continue;
        for (i = 4; i+sizeof(key)+4 < size-4; ++i) {
            LONG value;
            if (mem[i] == '$' && !memcmp(mem+i,key,sizeof(key)-1) &&
                StrToLong((STRPTR)mem+i+sizeof(key)-1,&value) > 0) return value;
        }
    }
    return 0;
}

/* Icons set to "Start from Shell" (tool type CLI) or "Start from ARexx"
   (REXX) run as Shell commands in the object's drawer, with an output
   window that opens only when the command writes. */
static int shell_launch(const char *path, const struct DiskObject *icon, LONG *error)
{
    char command[PATH_CAP*4+32], window[PATH_CAP+48];
    BPTR file, dir, old, input;
    LONG stack = icon_stack(icon), result;
    int fits;
    command[0] = 0;
    if (!icon) {
        /* A script without its own icon, as Coffin's WBDock runs them. */
        strcpy(command,"Execute ");
        fits = dock_shell_quote(command,sizeof(command),path);
    } else if (has_tool_type(icon,"REXX")) {
        /* RX runs a quoted argument as ARexx source, not as a file, so a
           one-line program calls the script by its full name. */
        char call[PATH_CAP*2+16] = "call '";
        size_t n = strlen(call);
        const char *c;
        for (c = path; *c && n+4 < sizeof(call); ++c) {
            if (*c == '\'') call[n++] = '\'';
            call[n++] = *c;
        }
        call[n++] = '\''; call[n] = 0;
        strcpy(command,"RX ");
        fits = !*c && dock_shell_quote(command,sizeof(command),call);
    } else if (icon->do_Type == WBPROJECT && icon->do_DefaultTool && icon->do_DefaultTool[0]) {
        fits = dock_shell_quote(command,sizeof(command),(const char *)icon->do_DefaultTool);
        if (fits) strcat(command," ");
        fits = fits && dock_shell_quote(command,sizeof(command),path);
    } else fits = dock_shell_quote(command,sizeof(command),path);
    if (!fits) { *error = ERROR_LINE_TOO_LONG; return 0; }
    file = Lock((STRPTR)path,ACCESS_READ);
    dir = file ? ParentDir(file) : 0;
    if (file) UnLock(file);
    if (!dir) { *error = IoErr() ? IoErr() : ERROR_OBJECT_NOT_FOUND; return 0; }
    snprintf(window,sizeof(window),"CON:0/50//130/%s/AUTO/CLOSE/WAIT",FilePart((STRPTR)path));
    if (!(input = Open((STRPTR)window,MODE_OLDFILE))) input = Open((STRPTR)"NIL:",MODE_OLDFILE);
    if (stack < MIN_STACK) stack = MIN_STACK;
    old = CurrentDir(dir);
    result = SystemTags((STRPTR)command, SYS_Input,(ULONG)input, SYS_Output,0,
        SYS_Asynch,TRUE, SYS_UserShell,TRUE, NP_StackSize,(stack+3)&~3L, TAG_DONE);
    CurrentDir(old);
    UnLock(dir);
    if (result == -1) {
        *error = IoErr() ? IoErr() : ERROR_NO_FREE_STORE;
        if (input) Close(input);
        return 0;
    }
    return 1;
}

/* A plain file without its own icon that is a script: it has the script
   bit, or (`pinned`) it is text and not a loadable program. Workbench would
   give the latter a default text icon and open it in an editor; a dock pin
   such as ShapeShifter's Launch or C:WOMyFiles means to run it. A file
   picked in the Folder Stack is a document unless it has the script bit. */
static int script_without_icon(const char *path, int pinned)
{
    char info[PATH_CAP+8];
    struct FileInfoBlock *fib;
    BPTR lock;
    UBYTE head[64];
    int script = 0;
    if (strlen(path)+6 > sizeof(info)) return 0;
    sprintf(info,"%s.info",path);
    if ((lock = Lock((STRPTR)info,ACCESS_READ))) { UnLock(lock); return 0; }
    if (!(lock = Lock((STRPTR)path,ACCESS_READ))) return 0;
    fib = AllocDosObject(DOS_FIB,NULL);
    if (fib && Examine(lock,fib) && fib->fib_DirEntryType < 0) {
        if (fib->fib_Protection & FIBF_SCRIPT) script = 1;
        else if (pinned && fib->fib_Size > 0) {
            BPTR file = OpenFromLock(lock);
            if (file) {
                LONG got = Read(file,head,sizeof(head));
                lock = 0;
                script = dock_text_bytes(head,(size_t)(got > 0 ? got : 0));
                Close(file);
            }
        }
    }
    if (fib) FreeDosObject(DOS_FIB,fib);
    if (lock) UnLock(lock);
    return script;
}

int launch_script(const char *path, int pinned, LONG *error)
{
    *error = 0;
    return script_without_icon(path,pinned) ? shell_launch(path,NULL,error) : -1;
}

int launch_object(App *app, const char *path, LONG *error)
{
    struct DockLaunched *launch;
    struct DiskObject *icon, *tool_icon = NULL;
    LONG stack, embedded, priority = 0;
    BPTR tool_dir, old_dir, file, current, home;
    struct Process *process;
    const char *pri;
    *error = 0;
    if (!app->launch_port) { *error = ERROR_NO_FREE_STORE; return 0; }
    if (!(launch = AllocVec(sizeof(*launch),MEMF_PUBLIC|MEMF_CLEAR))) { *error = ERROR_NO_FREE_STORE; return 0; }
    strncpy(launch->path,path,PATH_CAP-1);
    {
        struct TagItem get[] = {{ICONGETA_FailIfUnavailable,FALSE},{TAG_DONE,0}};
        icon = GetIconTagList((STRPTR)path,get);
    }
    if (icon && icon->do_Type != WBTOOL && icon->do_Type != WBPROJECT) {
        /* Drawers and disks open in the desktop's own windows. */
        char command[PATH_CAP+16];
        if (!dock_dopus_lister_new(command,sizeof(command),path)) *error = ERROR_OBJECT_WRONG_TYPE;
        else if (rexx_send(app->launch_port,command)) ++app->rexx_pending;
        else *error = ERROR_OBJECT_WRONG_TYPE;
        FreeDiskObject(icon);
        launch_free(launch);
        return !*error;
    }
    if (!icon) { *error = ERROR_OBJECT_NOT_FOUND; goto fail; }
    if (has_tool_type(icon,"CLI") || has_tool_type(icon,"REXX")) {
        int started = shell_launch(path,icon,error);
        FreeDiskObject(icon);
        launch_free(launch);
        return started;
    }
    file = Lock((STRPTR)path,ACCESS_READ);
    if (!file && icon->do_Type != WBPROJECT) { *error = IoErr(); goto fail; }
    if (icon->do_Type == WBPROJECT) {
        /* A project can be its icon alone: the AmigaOS 3.2 Shell is
           Shell.info with SYS:System/CLI as its tool ("Could not open",
           AmiKit). Workbench passes its drawer and name then too. */
        BPTR project_dir = file ? ParentDir(file) : drawer_lock(path), tool;
        if (!icon->do_DefaultTool || !icon->do_DefaultTool[0] || !project_dir) {
            *error = ERROR_OBJECT_NOT_FOUND;
            if (file) UnLock(file);
            if (project_dir) UnLock(project_dir);
            goto fail;
        }
        tool = find_tool(project_dir,(const char *)icon->do_DefaultTool);
        if (!tool) { *error = ERROR_OBJECT_NOT_FOUND; if (file) UnLock(file); UnLock(project_dir); goto fail; }
        if (file) {
            UnLock(project_dir);
            if (!set_arg(launch,1,file)) { *error = IoErr(); UnLock(tool); goto fail; }
        } else set_arg_named(launch,1,project_dir,(const char *)FilePart((STRPTR)path));
        file = tool;
    }
    if (!set_arg(launch,0,file)) { *error = IoErr(); goto fail; }
    launch->startup.sm_NumArgs = icon->do_Type == WBPROJECT ? 2 : 1;
    tool_dir = launch->args[0].wa_Lock;
    old_dir = CurrentDir(tool_dir);
    tool_icon = icon->do_Type == WBTOOL ? icon : GetDiskObject((STRPTR)launch->names[0]);
    launch->startup.sm_Segment = LoadSeg((STRPTR)launch->names[0]);
    if (!launch->startup.sm_Segment) *error = IoErr();
    CurrentDir(old_dir);
    if (!launch->startup.sm_Segment) goto fail;
    /* As workbench.library 45+: the opened icon's stack, raised to the
       tool icon's and to a $STACK: in the program when those are larger
       (RKM Libraries ch. 14; wb release notes 45.134, 45.165). TOOLPRI
       comes from the icon opened. */
    stack = icon_stack(icon);
    if (icon_stack(tool_icon) > stack) stack = icon_stack(tool_icon);
    if ((embedded = embedded_stack(launch->startup.sm_Segment)) > stack) stack = embedded;
    if (stack < MIN_STACK) stack = MIN_STACK;
    stack = (stack+3)&~3L;
    if ((pri = (const char *)FindToolType((CONST_STRPTR *)icon->do_ToolTypes,(CONST_STRPTR)"TOOLPRI"))) {
        LONG value;
        if (StrToLong((STRPTR)pri,&value) > 0) priority = dock_clamp(value,-128,127);
    }
    /* The new process owns both locks and frees them when it ends. */
    current = DupLock(tool_dir);
    home = DupLock(tool_dir);
    /* No standard streams, as for a program Workbench starts: dos would
       otherwise open NIL: for Input() and Output() and close them at exit,
       and a program whose startup code also closes them closes a handle
       twice (dos.library 8700000E, AN_FileReclosed, on AmigaOS 3.2.3). */
    process = current && home ? CreateNewProcTags(NP_Input,0, NP_Output,0,
        NP_CloseInput,FALSE, NP_CloseOutput,FALSE, NP_Seglist,(ULONG)launch->startup.sm_Segment,
        NP_FreeSeglist,FALSE, NP_StackSize,stack, NP_Priority,priority,
        NP_Name,(ULONG)launch->names[0], NP_CurrentDir,(ULONG)current,
        NP_HomeDir,(ULONG)home, NP_WindowPtr,0, NP_ConsoleTask,0,
        TAG_DONE) : NULL;
    if (!process) {
        *error = IoErr() ? IoErr() : ERROR_NO_FREE_STORE;
        if (current) UnLock(current);
        if (home) UnLock(home);
        goto fail;
    }
    launch->startup.sm_Message.mn_Node.ln_Type = NT_MESSAGE;
    launch->startup.sm_Message.mn_Node.ln_Name = NULL; /* not a RexxMsg */
    launch->startup.sm_Message.mn_ReplyPort = app->launch_port;
    launch->startup.sm_Message.mn_Length = sizeof(launch->startup);
    launch->startup.sm_Process = &process->pr_MsgPort;
    launch->startup.sm_ArgList = launch->args;
    launch->next = app->launched;
    app->launched = launch;
    PutMsg(&process->pr_MsgPort,&launch->startup.sm_Message);
    mark_items(app,path,1);
    if (tool_icon && tool_icon != icon) FreeDiskObject(tool_icon);
    FreeDiskObject(icon);
    return 1;
fail:
    if (tool_icon && tool_icon != icon) FreeDiskObject(tool_icon);
    if (icon) FreeDiskObject(icon);
    launch_free(launch);
    return 0;
}

int launch_active(const App *app, const char *path)
{
    const struct DockLaunched *launch;
    for (launch = app->launched; launch; launch = launch->next)
        if (!Stricmp((STRPTR)launch->path,(STRPTR)path)) return 1;
    return 0;
}

/* Frees the programs that ended; 1 when a pin's dot went out. */
int launch_reap(App *app)
{
    struct Message *msg;
    int ended = 0;
    if (!app->launch_port) return 0;
    while ((msg = GetMsg(app->launch_port))) {
        struct DockLaunched **link = &app->launched;
        while (*link && &(*link)->startup.sm_Message != msg) link = &(*link)->next;
        if (!*link) {
            struct RexxMsg *rexx = (struct RexxMsg *)msg;
            if (!RexxSysBase || !IsRexxMsg(rexx)) continue;
            if (rexx->rm_Result1 && !app->notice[0])
                snprintf(app->notice,sizeof(app->notice),"Directory Opus could not do\n%s",
                    (const char *)rexx->rm_Args[0]);
            rexx_free(rexx);
            --app->rexx_pending;
            continue;
        }
        {
            struct DockLaunched *launch = *link;
            *link = launch->next;
            if (!launch_active(app,launch->path)) ended |= mark_items(app,launch->path,0);
            launch_free(launch);
        }
    }
    return ended;
}

int launch_open(App *app)
{
    RexxSysBase = (struct RxsLib *)OpenLibrary((STRPTR)"rexxsyslib.library",36);
    app->launch_port = CreateMsgPort();
    return app->launch_port != NULL;
}

/* The programs' code is in the dock's memory until they reply: wait for
   every one. The dock's windows are already closed. Directory Opus answers
   ARexx at once; one that never came kept the dock from quitting, so its
   replies get two seconds, then the port is left behind, ignored. */
void launch_close(App *app)
{
    enum { REXX_TICKS = 2*TICKS_PER_SECOND, POLL_TICKS = 5 };
    const struct DockLaunched *launch;
    int ticks = 0, told = 0;
    if (!app->launch_port) return;
    for (launch_reap(app); app->launched || app->rexx_pending; launch_reap(app)) {
        if (!told) {
            for (launch = app->launched; launch; launch = launch->next)
                printf("Waiting for %s, started from the dock, to quit.\n",launch->path);
            told = 1;
        }
        if (app->launched) { WaitPort(app->launch_port); continue; }
        if (ticks >= REXX_TICKS) {
            printf("Directory Opus did not answer %d command(s).\n",app->rexx_pending);
            app->launch_port->mp_Flags = PA_IGNORE;
            app->launch_port = NULL;
            return;
        }
        Delay(POLL_TICKS);
        ticks += POLL_TICKS;
    }
    DeleteMsgPort(app->launch_port);
    app->launch_port = NULL;
    if (RexxSysBase) CloseLibrary((struct Library *)RexxSysBase);
    RexxSysBase = NULL;
}

int workbench_running(void)
{
    int running;
    Forbid();
    running = FindPort((STRPTR)"WORKBENCH") != NULL;
    Permit();
    return running;
}

int dopus_running(void)
{
    int running;
    Forbid();
    running = amiga_caps_dopus_port() != NULL;
    Permit();
    return running;
}

/* Workbench, unless --self-launch, opens what it can; otherwise we do. */
int self_launching(const App *app)
{
    return app->self_launch || !workbench_running();
}

void dopus_close_lister(App *app, ULONG lister)
{
    char command[32];
    snprintf(command,sizeof(command),"lister close %lu",(unsigned long)lister);
    if (rexx_send(app->launch_port,command)) ++app->rexx_pending;
}

/* Status worker: open Directory Opus listers count as open drawers. */
void dopus_drawers(ProgramPoll *poll)
{
    char handles[1024], *cursor, *end;
    void *requesters;
    int i;
    if (rexx_call("lister query all",handles,sizeof(handles)) < 0) return;
    requesters = amiga_requesters_off(); /* a pin on an absent disk */
    for (i = 0; i < poll->count; ++i)
        if (poll->pins[i].kind == PIN_DRAWER) { poll->pins[i].running = 0; poll->pins[i].lister = 0; }
    for (cursor = handles; *cursor; cursor = end) {
        char command[48], path[PATH_CAP];
        ULONG lister = strtoul(cursor,&end,10);
        BPTR place;
        if (end == cursor) break;
        /* A list cut at the buffer's end: its last number is partial. */
        if (!*end && strlen(handles) == sizeof(handles)-1) break;
        snprintf(command,sizeof(command),"lister query %lu path",(unsigned long)lister);
        if (rexx_call(command,path,sizeof(path)) || !(place = Lock((STRPTR)path,ACCESS_READ))) continue;
        for (i = 0; i < poll->count; ++i) {
            BPTR pin;
            if (poll->pins[i].kind != PIN_DRAWER || poll->pins[i].running) continue;
            if (!(pin = Lock((STRPTR)poll->pins[i].path,ACCESS_READ))) continue;
            if (SameLock(pin,place) == LOCK_SAME) {
                poll->pins[i].running = 1;
                poll->pins[i].lister = lister;
            }
            UnLock(pin);
        }
        UnLock(place);
    }
    amiga_requesters_restore(requesters);
    poll->valid |= PIN_DRAWER;
}
