/* The rest of DockAwesome is built for the 68080. This file is built for
   the 68020 and runs as the first libnix constructor, before main and any
   68080 code, so another CPU gets a message instead of an illegal
   instruction. The startup and libnix around it are 68000 code.
   ApolloOS and AmigaOS 3.2.3 on the Vampire set AttnFlags bit 10
   ($a47f and $847f measured on core 12856). */
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/execbase.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <stabs.h>
#include <stdlib.h>

/* A test build can require a bit that is not set to try the refusal. */
#ifndef DOCK_CPU_GUARD_BIT
#define DOCK_CPU_GUARD_BIT 10
#endif

static const char refusal[] = "DockAwesome needs an Apollo 68080 (Vampire).";

void dock_cpu_guard(void);
void dock_cpu_guard(void)
{
    /* Local bases: libnix opens its own in a later constructor. */
    struct DosLibrary *DOSBase;
    struct IntuitionBase *IntuitionBase;
    BPTR out;
    if (SysBase->AttnFlags & (1U<<DOCK_CPU_GUARD_BIT)) return;
    out = 0;
    if ((DOSBase = (struct DosLibrary *)OpenLibrary((STRPTR)"dos.library",36))) {
        out = Output();
        if (out) {
            Write(out,(APTR)refusal,sizeof(refusal)-1);
            Write(out,(APTR)"\n",1);
        }
        CloseLibrary((struct Library *)DOSBase);
    }
    /* Started from Workbench: no Shell to write to. */
    if (!out && (IntuitionBase = (struct IntuitionBase *)OpenLibrary((STRPTR)"intuition.library",36))) {
        struct EasyStruct req = { sizeof(req), 0, (STRPTR)"DockAwesome", (STRPTR)refusal, (STRPTR)"OK" };
        EasyRequestArgs(NULL,&req,NULL,NULL);
        CloseLibrary((struct Library *)IntuitionBase);
    }
    exit(RETURN_FAIL);
}
/* Not static: the init list refers to the function by its symbol. */
ADD2INIT(dock_cpu_guard,-128);
