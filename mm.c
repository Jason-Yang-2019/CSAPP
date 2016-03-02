#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define MAX_DESCRIPTOR 2048
#define NDESCRIPTORS 512
#define MOREBYTE 4096
#define hash(p, t) (((unsigned long) (p) >> 3) & (sizeof(t) / sizeof ((t)[0]) - 1))


struct descriptor{
        struct descriptor *free;
        struct descriptor *link;
        const void *ptr;
        long size;
        const char *file;
        int line;
}*htab[MAX_DESCRIPTOR];

static struct descriptor freelist = {&freelist};
static int nleft;
static struct descriptor *avail;
/*
 *accoding to the address given, find the descriptor
 *根据给定的地址，寻找对应的描述符
 */
static struct descriptor *find(const void *ptr){
        struct descriptor *bp = htab[hash(ptr, htab)];
        while(bp && bp->ptr != ptr)
          bp = bp->link;
        return bp;
}

/*
 *initilize a descriptor and return it
 *if out of descriptor, the allocate some more
 *一次性申请256个描述符
 */
static struct descriptor *dalloc(void *ptr, long size){
        if(nleft <= 0){
                avail = (struct descriptor *)mem_sbrk(ALIGN(NDESCRIPTORS * sizeof(*avail)));
                if(avail == NULL)
                  return NULL;
                nleft = NDESCRIPTORS;
        }
        avail->ptr = ptr;
        avail->size = size;
        avail->free = NULL;
        avail->link = NULL;
        nleft--;
        return avail++;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
        freelist.free = &freelist;
        avail = NULL;
        nleft = 0;
        memset(htab, 0, sizeof(htab));
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 *先检查空闲链表，没有的话再申请
 */
void *mm_malloc(size_t size)
{
    void *ptr = NULL;
        struct descriptor *bp;
        size = ALIGN(size);
        for(bp = freelist.free;bp;bp=bp->free){
                if(bp->size > size){
                        bp->size -= size;
                        ptr = bp->ptr;
                        bp->ptr += size;
                        //bp ->free = NULL;
                        if((bp = dalloc(ptr, size)) != NULL){
                                unsigned h = hash(ptr, htab);
                                bp->link = htab[h];
                                htab[h] = bp;
                        }
                        else{
                                printf("can not allocate new descriptor\n");
                                return NULL;
                        }
                }
                if(bp == &freelist){
                        struct descriptor *newbp;
                        ptr = mem_sbrk(ALIGN(size + MOREBYTE));
                        newbp = dalloc(ptr, ALIGN(size + MOREBYTE));
                        newbp->free = freelist.free;
                        freelist.free = newbp;
                }
        }
        return ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
        if(ptr){
                struct descriptor *bp;
                if((bp = find(ptr)) == NULL || bp->free){
                        printf("invalid pointer\n");
                        return;
                }
                bp->free = freelist.free;
                freelist.free = bp;
        }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
        struct descriptor *bp;
        void *newptr;
        if(!ptr)
          return mm_malloc(size);
        if(!size){
          mm_free(ptr);
          return NULL;
        }
        size = ALIGN(size);
        if((bp = find(ptr)) == NULL || bp->free){
                printf("invalid porinter\n");
                return NULL;
        }
        if(size <= bp->size){
                newptr = (char *)ptr + size;
                struct descriptor *newbp;
                newbp = dalloc(newptr, bp->size - size);
                bp->size -= size;
                newbp ->free = freelist.free;
                freelist.free = newbp;
        }
        else{
                newptr = mm_malloc(size);
                memcpy(newptr, ptr, bp->size);
                mm_free(ptr);
        }
        return newptr;
}
