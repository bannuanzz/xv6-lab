// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);  //xun huan bian li suo you ye
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP) 
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}



//#################
#define NULL 0


struct mrun{
  struct mrun *next;
  uint size;
};

struct{
  struct spinlock lock;
  struct mrun *freelist;
}mmem;




void minit(){  //only 1 block just need init 1 time
  printf("------------------------\n");
  printf(">>>>begin first and next fit malloc init\n");

  initlock(&mmem.lock,"mmem");
  uint size=NEWSTOP-PHYSTOP;
  memset((void*)PHYSTOP,1,size);

  struct mrun *b;
  b=(struct mrun*)PHYSTOP;
  b->size=size;
  b->next=NULL;
  // printf("bsize: %d\n",b->size);
  acquire(&mmem.lock);
  mmem.freelist=b;
  release(&mmem.lock);

  printf("total size is %d B\n",mmem.freelist->size);
  printf("-------------------------\n");
}


void showFreeList(){

  printf("--------------------------\n");
  printf(">>>>free list\n");
  struct mrun*current=mmem.freelist;
  while(current->size>0){
    printf("address:%p\n",current);
    printf("size:%dB\n",current->size);
    if(current->next==NULL){
      break;
    }
    current=current->next;
  }
  printf("-------------------------\n");

}

void mergeFreeList(){
  struct mrun *b=mmem.freelist;
  printf("--------------------------\n");
  while(b){
    if(((uint)b+b->size)==(uint)(b->next)){  //fang fa
      printf(">>>>merge\n");
      printf("address1:%p\n",b);
      printf("size1:%dB\n",b->size);
      printf("address2:%p\n",b->next);
      printf("size2:%dB\n",b->next->size);

      printf("sumsize:%dB\n",(b->size+b->next->size));
      b->size=b->size+b->next->size;
      b->next=b->next->next;
      continue;   //he bing wan yi hou ke neng hai he xia yi kuai he bing
    }
    b=b->next;
  }
  printf("-------------------------\n");
  printf("\n");
  }


void free(void *ptr){
  printf("-------------------------\n");
  printf(">>>>>begin free\n");

  if(ptr==NULL){
    printf("no address,free fail\n");
    return;
  }
  if((uint)ptr>NEWSTOP||(uint)ptr<PHYSTOP)
    {
      printf("address out of range,free fail\n");
      return;
    }
  
  struct mrun *b=(struct mrun *)ptr;
  printf("free address:%p\n",b);
  printf("free size:%dB\n",b->size);

  acquire(&mmem.lock);
  struct mrun *i=mmem.freelist;
  // first fit
  if((uint)i>(uint)b)
  {
  
    b->next=i;
    mmem.freelist=b;
    release(&mmem.lock);
    mergeFreeList();
    printf("free success!!!\n");
    printf("------------------------\n");
    return;
  }
  // int flag=0;
  for(;i->next!=NULL;i=i->next){
    if((uint)i<(uint)b&&(uint)(i->next)>(uint)b)
    {
      b->next=i->next;
      i->next=b;

      // flag=1;

      mergeFreeList();
      release(&mmem.lock);
      printf("free success!!!\n");
      printf("----------------------\n");
      return;

    }
  }
  // if(!flag){
    i->next=b;
    b->next=NULL;
  // }
  mergeFreeList();
  release(&mmem.lock);
  printf("free success\n");
  printf("--------------------------\n");
  printf("\n");
}


void *Mymalloc(uint size){
  struct mrun *b = mmem.freelist;
  struct mrun * pre;
  // printf("%d \n", mmem.freelist->size);
  printf("--------------------------\n");
  printf(">>>>begin malloc %dB\n",size);
  // printf("%d",b->size);
  // printf("111111");
  acquire(&mmem.lock);
  
  for(;b;b=b->next){
    // printf("11111\n");
    // printf("%p\n",&b);
    // printf("%d\n",b->size);
    if((b->size) >= size){
      //  printf("2222222\n");
      if((b->next==NULL)&&( b->size == size))  //bu neng wan quan fen yao gei zhi zhen liu wei zhi 
          break;
      printf("malloc address:%p\n",b);
      printf("free size is %dB\n",b->size);
      struct mrun *new=(struct mrun*)((uint)b+size);
      new->size=b->size-size;
      new->next=b->next;
      b->size=size;
      if(b==mmem.freelist){
        mmem.freelist=new;
      }else{
        pre->next=new;   //   
      }
      release(&mmem.lock);
      printf("malloc %dB success!!!\n",size);
      printf("--------------------------\n");
      printf("\n");
      return b;
    }
    pre=b;

  }
  printf("malloc %dB failed\n",size);
  release(&mmem.lock);
  printf("-----------------------------\n");
  return NULL;
}

// Memory run structure

// Memory management structure


#define MAX 24
struct block{
  struct block*next;

};
struct freelist{
  struct block*head;

};
struct freelist freelist[MAX];
void buddy_init(){

  for(int i=0;i<MAX;i++){
    freelist[i].head=NULL;
  }
  // the initial block is a large block
  struct  block *initblock=(struct block *)PHYSTOP;
  initblock->next=NULL;
  freelist[MAX-1].head=initblock;
  
}

void *buddy_alloc(uint size){
  int order;
  for(order=0;order<MAX;order++){
    if((1<<order)>=size){
      break;
    }
  }
  if(order==MAX){
    return;
  }
  for(int i=order;i<MAX; i++){
    if(freelist[i].head!=NULL){
      struct block*block=freelist[i].head;

      freelist[i].head=block->next;
      while(i>order){
        i--;
        struct block*buddy=(struct block*)((uint64)block+(1<<i));
        buddy->next=freelist[i].head;
        freelist[i].head=buddy;
      }
      return (void*)block;
    }
  }
  return NULL;
}

void buddy_free(void *ptr,uint size){
  int order;
  for(order=0;order<MAX;order++){
    if((1<<order)>=size){
      break;
    }
    struct block *block=(struct block *)ptr;
  while(order<MAX-1){
    struct block *buddy=(struct block *)((uint64) block^(1<<order));
    struct block **current=&freelist[order].head;
    while(*current!=NULL&&*current!=buddy){
      current=&((*current)->next);
    }
    if(*current==NULL){
      break;
    }
    *current=(*current)->next;
    block =(struct block *)((uint64)block&(uint64)buddy);
    order++;
  }
  block->next=freelist[order].head;
  freelist[order].head=block;
 
}
}


void buddy_show() {

  for (int i=0;i<MAX;++i) {
    printf("the size of memory is %d ", (1<<i));
    if (freelist[i].head != NULL) {
      struct block *block = freelist[i].head;
      printf("%p ", block);
      while (block->next != NULL) {
        printf("%p ", block->next);
        block = block->next;
      }
    }
    printf("\n");
  }
}




//   struct mrun *m1, *m2, *m3, *m4;
//   printf("\n");
//   printf("\n");
//   printf(">>>>>begin memTest\n");
// //printf("%d\n",mmem.freelist->size);
//   printf("######################################\n");
//   printf(">>>>>malloc and single free\n");
  
//   m1 = Mymalloc(1024*1024);
//   showFreeList();
//   free(m1);
//   showFreeList();
//   printf("\n");
//   printf("\n");
//   printf("\n");
//   printf("######################################\n");
//   printf(">>>>>incorrect malloc and incorrect free \n");
//   m1 = Mymalloc(15 * 1024*1024);
//   free(m1);
// printf("\n");
//   printf("\n");
//   printf("\n");
//    printf("######################################\n");
//   printf(">>>>>malloc and merge in free\n");
//   m1 = Mymalloc(2 * 1024*1024);
//   m2 = Mymalloc(1 * 1024*1024);
//   free(m1);
//   showFreeList();
//   free(m2);
//   showFreeList();
//   printf("\n");
//   printf("\n");
//   printf("\n");
//    printf("######################################\n");
//   printf(">>>>>first fit malloc \n");
//   m1 = Mymalloc(1 * 1024);
//   m2 = Mymalloc(2 * 1024);
//   m3 = Mymalloc(3 * 1024);
//   free(m2);
//   showFreeList();
//   m4 = Mymalloc(1 * 1024);
//   showFreeList();
//   free(m1);
//   free(m4);
//   showFreeList();
//   printf("######################################\n");
//   printf("\n");
void memTest(){
  struct block *m1;
  m1 =buddy_alloc(3*1024);
  buddy_show();
  buddy_free(m1,3*1024);
  buddy_show();
  exit(0);
}
