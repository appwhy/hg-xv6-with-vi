#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)

SYSCALL(hg)
SYSCALL(hg1)
SYSCALL(hg2)
SYSCALL(hg3)

SYSCALL(setCursorPos)
SYSCALL(copyFromTextToScreen)
SYSCALL(clearScreen)
SYSCALL(writeAt)
SYSCALL(setBufferFlag)
SYSCALL(setShowAtOnce)
SYSCALL(getCursorPos)
SYSCALL(getCharOnCursor)



