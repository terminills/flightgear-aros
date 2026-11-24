/* fgaros_no_requesters(): a file FlightGear looks for under a name that AROS reads as a volume ("/Aircraft/x" is
   Aircraft:x) is a file that is not there, not a request to insert that volume. Threads started afterwards
   inherit the setting. */
#include <proto/exec.h>
#include <dos/dosextens.h>

void fgaros_no_requesters(void)
{
    struct Process *me = (struct Process *)FindTask(NULL);

    if (me->pr_Task.tc_Node.ln_Type == NT_PROCESS)
        me->pr_WindowPtr = (APTR)-1;
}

/* The stack fgfs is run on (AROS's start-up code reads __stack): the Shell's default is too small for it. */
ULONG __stack = 4000000;
