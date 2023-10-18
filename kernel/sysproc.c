#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/*setting up traces for system calls. by updating the tracemask of the current process. 
The process will be tracked on which system calls it invokes.*/

uint64 sys_trace(void)
{
    int trace_sys_mask;
    if (argint(0, &trace_sys_mask) < 0) {// trace_sys_mask = The first argument of the system call
        return -1;
    }
    (myproc()->tracemask) |= trace_sys_mask; /*Bitwise OR with trace_sys_mask. This means that if the user wishes to trace a new system call,
     the corresponding bit of the system call in tracemask is set.*/
     return 0;
}

uint64
sys_sysinfo(void) {
    struct proc *my_proc = myproc();
    uint64 p;
    if (argaddr(0, &p) < 0) {// &P = argraw(0) = p->trapframe->a0;
        return -1;
    }
    //construct in kernel first
    struct sysinfo s;
    s.freemem = kfreemem();
    s.nproc = used_procmem();
    //copy to user space
    if (copyout(my_proc->pagetable, p, (char*)&s, sizeof(s)) < 0) {
        return -1;
    }
    return 0;
}