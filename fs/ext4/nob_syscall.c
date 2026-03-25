/* nob_syscall.c
 * NobLSM: check_commit, is_committed syscall implementation
 */
#include <linux/syscalls.h>
#include "nob_commit_tracker.h"

SYSCALL_DEFINE1(nob_check_commit, unsigned long, ino)
{
    return nob_pending_add(ino);
}

SYSCALL_DEFINE1(nob_is_committed, unsigned long, ino)
{
    return nob_is_committed(ino) ? 1 : 0;
}
