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

#define WSIZE 4
#define DSIZE 8
#define HSIZE 16
#define CHUNKSIZE (1<<12)
#define MINSIZE (1<<8)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size)|(alloc))
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define HDRP(bp) ((char *)(bp) -  WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


static char *heap_listp;
static char *frelist;
static void *extend_heap(size_t size);
static void insert_free_list(char *p);
static void *find_fit(size_t size);
static void place(void *p, size_t size);
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
        heap_listp = NULL;
        if((heap_listp = (char *)mem_sbrk(10*WSIZE)) == (void *)-1)
          return -1;
        PUT(heap_listp, 0);
        PUT(heap_listp + (1*WSIZE), PACK(2*DSIZE, 0)); //free list header 
        PUT(heap_listp + (2*WSIZE), 0);                //free list pred
        PUT(heap_listp + (3*WSIZE), 0);                //free list next
        PUT(heap_listp + (4*WSIZE), PACK(2*DSIZE, 0)); //free list footer
        PUT(heap_listp + (5*WSIZE), PACK(2*DSIZE, 1)); //heap_listp header
        PUT(heap_listp + (8*WSIZE), PACK(2*DSIZE, 1)); //heap_listp footer
        PUT(heap_listp + (9*WSIZE), PACK(0, 1));
        frelist = heap_listp + 2*WSIZE;
        heap_listp += 6*WSIZE;
        if(extend_heap(CHUNKSIZE) == NULL)
          return -1;
    return 0;
}

/*
 *extend the heap by size bytes
 */
static void*extend_heap(size_t  size){
        int newsize = ALIGN(size + HSIZE);
        char *p = mem_sbrk(newsize);
        if(p == (void *)-1)
          return NULL;
        PUT(HDRP(p), PACK(newsize, 0));
        PUT(FTRP(p), PACK(newsize, 0));
        PUT(HDRP(NEXT_BLKP(p)), PACK(0, 1));
        insert_free_list(p);
        return p;
}

/*
 *insert the new free block into the free list
 */
static void insert_free_list(char *p){
        PUT(p, *((unsigned int *)frelist));
        PUT(frelist, p);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
        void *bp = NULL;
    int newsize = ALIGN(size);
        //newsize = MAX(newsize, MINSIZE);
        if((bp = find_fit(newsize)) != NULL){
                place(bp, newsize);
                return bp;
        }
        size_t extend_size = MAX(newsize, CHUNKSIZE) + HSIZE;
        if((bp = extend_heap(extend_size)) == NULL)
          return NULL;

        place(bp, newsize);
        return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
        size_t size = GET_SIZE(HDRP(ptr));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
        insert_free_list(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
        void *newptr = NULL;
        char *oldptr = (char *)ptr;
        if(!oldptr)
          mm_malloc(size);
        if(!size)
          mm_free(ptr);
        size_t oldsize = GET_SIZE(oldptr) - 2*WSIZE;
        size = ALIGN(size);
        if(size == oldsize || (oldsize > size && oldsize - size <= 4*WSIZE))
          return oldptr;
        else if(size < oldsize){
                PUT(HDRP(oldptr), PACK(size + 2*WSIZE, 1));
                PUT(oldptr + size, PACK(size + 2*WSIZE, 1));
                PUT(oldptr + size + WSIZE, PACK(oldsize - size, 0));
                PUT(oldptr + oldsize, PACK(oldsize - size, 0));
                insert_free_list(oldptr + size + 2*WSIZE);
                newptr = oldptr;
        }
        else{
                newptr = mm_malloc(size);
                memcpy(newptr, oldptr, oldsize);
                mm_free(oldptr);
        }
        return newptr;
}

/*
 *
 */
static void place(void *p, size_t size){
        size_t oldsize = GET_SIZE(HDRP(p)) - 2*WSIZE;
        if(oldsize == size || (oldsize > size && oldsize - size <= 4*WSIZE)){
                PUT(HDRP(p), PACK(oldsize + 2*WSIZE, 1));
                PUT(FTRP(p), PACK(oldsize + 2*WSIZE, 1));
                return;
        }
        else{
                PUT(HDRP(p), PACK(size + 2*WSIZE, 1));
                PUT((char *)p + size, PACK(size + 2*WSIZE, 1));
                PUT((char *)p + size + WSIZE, PACK(oldsize - size, 0));
                PUT((char *)p + oldsize, PACK(oldsize - size, 0));
                insert_free_list((char *)p + size + 2*WSIZE);
        }
}

/*
 *
 */
static void *find_fit(size_t  size){
        char *ptr = (char *)(*(unsigned int *)(frelist + WSIZE));
        while(ptr && GET_SIZE(ptr) < size)
          ptr = (char *)(*(unsigned int *)(ptr + WSIZE));
        return ptr;
}
