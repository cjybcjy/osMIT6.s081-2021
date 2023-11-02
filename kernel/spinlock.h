#ifndef SPINLOCK_H
#define SPINLOCK_H

// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.     Stores the descriptive name of the lock
  struct cpu *cpu;   // The cpu holding the lock.
};

#endif
