// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock biglock;
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];//double linked list
} bcache;

extern int hash(int);

void
binit(void)
{
  struct buf *b;
  initlock(&bcache.biglock, "bcache.biglock");
  for (int i = 0; i < NBUCKET; i++) {
    initlock(&bcache.lock[i], "bcache");
  }

  // Create linked list of buffers for each bucket
  for (int i = 0; i < NBUCKET; i++) {//initial  *prev & *next point to itself -> null node
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){//Head insertion method
    //Each time through the loop, a new buf is added to the beginning of the linked list 
    //and the existing elements of the list update their prev pointers
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b, *bnew = 0;
  int i = hash(blockno);
  int min_ticks = 0;
  acquire(&bcache.lock[i]);

  // Is the block already cached?
  for(b = bcache.head[i].next; b != &bcache.head[i]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[i]);
      acquiresleep(&b->lock);
      return b;
    }
  }

 release(&bcache.lock[i]);
// not cached, in order to avoid mutiple concurrent bget() requests
// cheak again
 acquire(&bcache.biglock);
 acquire(&bcache.lock[i]);
  for(b = bcache.head[i].next; b != &bcache.head[i]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[i]);
      release(&bcache.biglock);
      acquiresleep(&b->lock);
      return b;
    }
  }

// still not cached
// we are now only holding biglock, none of the bucket locks are held by us.
// find a LRU block from current bucket
for (b = bcache.head[i].next; b != &bcache.head[i]; b = b->next) {
    if (b->refcnt == 0 && (bnew == 0 || b->lastuse < min_ticks)) {
        min_ticks = b->lastuse;
        bnew = b;
    }
}

if (bnew) {
    bnew->dev = dev;
    bnew->blockno = blockno;
    bnew->refcnt++;
    bnew->valid = 0;
    release(&bcache.lock[i]);
    release(&bcache.biglock);
    acquiresleep(&bnew->lock);
    return bnew;
}



// find block from the other buckets
int j;
for ( j = hash(i + 1); j != i; j = hash(j + 1)) {
    acquire(&bcache.lock[j]);
    for (b = bcache.head[j].next; b != &bcache.head[j]; b = b->next) {
        if (b->refcnt == 0 && (bnew == 0 || b->lastuse < min_ticks)) {
            min_ticks = b->lastuse;
            bnew = b;
        }
    }

    if (bnew){
        bnew->dev = dev;
        bnew->blockno = blockno;
        bnew->valid = 0;
        bnew->refcnt = 1;
        //remove block from its original bucket
        bnew->next->prev = bnew->prev;
        bnew->prev->next = bnew->next;
        release(&bcache.lock[j]);
        //add block
        bnew->next = bcache.head[i].next;
        bnew->prev = &bcache.head[i];
        bcache.head[i].next->prev = bnew;
        bcache.head[i].next = bnew;
        release(&bcache.lock[i]);
        release(&bcache.biglock);
        acquiresleep(&bnew->lock);
        return bnew;
    }
    release(&bcache.lock[j]);
}
  release(&bcache.lock[i]);
  release(&bcache.biglock);
  panic("bget: no buffers");
}


// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

int
hash(int blockno)
{
   return blockno % NBUCKET;
}


// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int i = hash(b->blockno);
  acquire(&bcache.lock[i]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // // no one is waiting for it.
    // b->next->prev = b->prev;
    // b->prev->next = b->next;
    // //insert it at the begining of the buffer list
    // b->next = bcache.head.next;
    // b->prev = &bcache.head;
    // bcache.head.next->prev = b;
    // bcache.head.next = b;
    b->lastuse = ticks;
  }
  
  release(&bcache.lock[i]);
}

void
bpin(struct buf *b) {
  int i = hash(b->blockno);
  acquire(&bcache.lock[i]);
  b->refcnt++;
  release(&bcache.lock[i]);
}

void
bunpin(struct buf *b) {
  int i = hash(b->blockno);
  acquire(&bcache.lock[i]);
  b->refcnt--;
  release(&bcache.lock[i]);
}


