#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h> 
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

// adjustable macros

#define M_MIN_BLOCK_SIZE    36
#define M_PAGE_SIZE         4096
#define M_PAGE_MASK         0x11111000
#define M_PAGE_MASK_FLIPPED 0x00000111

#define M_DEALLOC_THRESHOLD     M_PAGE_SIZE + M_MIN_BLOCK_SIZE
#define M_PAGE_THRESHOLD        M_PAGE_SIZE - M_MIN_BLOCK_SIZE


// These are the three special states of a block, if it does
// not match any, then it has been memalloc-ed and does not
// carry the virus (i.e. it is not on the top of the stack).
// Special values have been assigned to these states. The
// reason for this is bitwise operations. The right-most
// signifies that the block usage (0 for no, 1 for yes).
// The next one signifies that it is the holder of the virus.
//
// A normal block (memalloc-ed, not on top of the heap) will
// be using a 'size_t' value type (prev_s) which is guaranteed
// to be more than any of the enums (M_MIN_BLOCK_WIDTH). A
// value of 1 makes no sense.

enum { BLOCK_DEALLOC = 0, BLOCK_DEALLOC_V = 2, BLOCK_ALLOC_V = 3 };

// Structure types. The first field in 'memblock ('size_t size')
// containes the size of the block, whether it be memalloc-ed or
// memdealloc-ed. The second is a special value which is used to
// distinguish blocks. If it is memalloc-ed, it will contain the
// previous block's size (via '::_prev_s') unless it is on top of
// the heap (::_prev_s is invalid). In this case it will be
// tagged with the enum 'BLOCK_ALLOV_V'. If is has been
// 'memfree-d', the 'size_t mask' param. will be set to either
// BLOCK_DEALLOC or BLOCK_DEALLOC_V (memdealloc-ed first block).
// It is important to note that offsetoff (struct memblock, prev_s)
// equals offsetof (struct freeblock, mask) so all the programme
// needs to do is check *(block_ptr + offsetoff) to determine the
// underlying type of block.

struct memblock
{
    size_t size;
    size_t prev_s;
};

struct freeblock
{
    size_t size;
    size_t mask;
    struct freeblock* lhs;
    struct freeblock* rhs;
};

struct unknownblock
{
    size_t size;
    size_t mask;
};

// global variables
static size_t _prev_s = BLOCK_DEALLOC_V ;
static void* _programme_brk;
static struct freeblock* _last_block = NULL;

// convenience function
static inline void* __init_block (void * p, size_t bytes)
{
    size_t* ptr = p;

    *ptr++ = bytes;
    *ptr++ = _prev_s;
    _prev_s = bytes;

    return (void*)ptr;
}

// convenience function 2
static inline void __unlink_block (struct freeblock* p)
{
    struct freeblock* l_rela_bl = ((struct freeblock*) p)->lhs;
    struct freeblock* r_rela_bl = ((struct freeblock*) p)->rhs;

    if (l_rela_bl != NULL) l_rela_bl->rhs = r_rela_bl;
    if (r_rela_bl != NULL) r_rela_bl->lhs = l_rela_bl;
    else _last_block = l_rela_bl;
}

void* memalloc (size_t bytes)
{
    if (bytes == 0) return NULL;

    size_t alloc_size = (bytes += sizeof (struct memblock));

    // search if any of the free blocks (first match)
    for (struct freeblock* it = _last_block; it != NULL; it = it->lhs)
    {
        if (it->size >= alloc_size)
        {
            if (it->size - alloc_size <= M_MIN_BLOCK_SIZE)
            {
                __unlink_block (it);
                
                if (it->mask == BLOCK_DEALLOC_V) _prev_s = BLOCK_ALLOC_V;
                return __init_block ((void*)it, it->size);
            }

            else
            {
                it->size -= alloc_size;

                return __init_block ((void*)it + it->size, alloc_size);
            }

        }
    }

    // no room -- must perform system allocation
    
    bool perfect_fit;

    if (alloc_size & M_PAGE_MASK_FLIPPED < M_PAGE_THRESHOLD)
    {
        alloc_size = (alloc_size & M_PAGE_MASK) + M_PAGE_SIZE;
        perfect_fit = false; 
    }
    else
    {
        alloc_size = (alloc_size & M_PAGE_MASK) + M_PAGE_SIZE;
        perfect_fit = true;
    }
    
    void* p = sbrk (alloc_size);
    assert (p != (void *) -1);
    _programme_brk = p;

    // add to end of list
    if (perfect_fit == false)
    {
        struct freeblock* curr = p + bytes;

        curr->size = alloc_size - bytes;
        curr->mask = BLOCK_DEALLOC;
        curr->lhs = _last_block;
        curr->rhs = NULL;

        if (_last_block != NULL) _last_block->rhs = (struct freeblock*) curr;
        _last_block = (struct freeblock*) curr; 
    }

    return __init_block (p, bytes);
}

void memfree (void* p)
{
//    if (p == NULL) return;

    struct freeblock* curr = p;
    struct unknownblock* prev = p - ((struct memblock*) p)->prev_s;
    struct unknownblock* next = p + ((struct memblock*) p)->size;
   
    // merge with next block if previously memfree-d
    if (next != _programme_brk && (!next->mask))
    {
        __unlink_block ((void*) next);

        // update total size
        curr->size += next->size;
    }

    // merge with prev block if previously memfree-d
    if (curr->mask != BLOCK_ALLOC_V && (prev->mask == BLOCK_DEALLOC || prev->mask == BLOCK_DEALLOC_V))
    {
        // update total size
        prev->size += curr->size;
        // make the merged block current (no need to register it)
        curr = (struct freeblock*) prev;
    }
    
    // did not merge with previous block -- it must be registered manually
    else
    {
        curr->mask = (curr->mask == BLOCK_ALLOC_V ? BLOCK_DEALLOC_V : BLOCK_DEALLOC);
        curr->lhs = _last_block;
        curr->rhs = NULL;

        if (_last_block != NULL) _last_block->rhs = (struct freeblock*) curr;
        _last_block = (struct freeblock*) curr; 
    }
    
    // Done. 'curr' is registered and defragmentation took place. This means
    // it is either surrounded by memalloc-ed blocks (ok), it is at the
    // bottom of the heap (in which case it should be deallocated) or is the
    // last block in the heap (it has been jumping if statements until now,
    // it is one page in size and will pass the deallocation test)

    // The bottom-most block will constantly increase it's size until it
    // reaches a certain threshold (one page + M_BLOCK_MIN_SIZE). This is
    // so to reduce unnecessary system calls.
    
    // Important note. The last block (page) will never match this the if
    // statement below, i.e. there will always be at least one page present
    // which is both good and bad.
    
    if ((void*) curr + curr->size == _programme_brk &&
            (_programme_brk - (void*) curr >= M_DEALLOC_THRESHOLD))
    {
        _programme_brk = (void*) (((intptr_t) curr & M_PAGE_MASK) + M_PAGE_SIZE);
        assert (brk (_programme_brk) == 0);
        curr->size = _programme_brk - (void*)curr;
    }
}

#include <time.h>

int main ()
{

    time_t start = clock();

    char* allocs[1024];

    for (int i = 0; i < 1024; i++)
    {
        allocs[i] = memalloc (356);
    }

    for (int i = 0; i < 1024; i++)
    {
        memfree (allocs[i]);
    }

    time_t total = clock() - start;
    printf ("It took me a total of: %u\n", total);

    return 0;
}
