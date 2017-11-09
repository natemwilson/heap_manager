/*--------------------------------------------------------------------*/
/* heapmgr1.c                                                         */
/* Author: Nate Wilson                                                */
/* Author: Narek Galstyan                                             */
/*--------------------------------------------------------------------*/

#include "heapmgr.h"
#include "checker1.h"
#include "chunk.h"
#include <stddef.h>
#include <assert.h>

#define __USE_XOPEN_EXTENDED
#include <unistd.h>

/* In lieu of a boolean data type. */
enum {FALSE, TRUE};

/* Minimum size of the free (logical) chunk to split */
enum {SPLIT_THRESHOLD = 3};

/* The minimum number of units to request of the OS. */
enum {MIN_UNITS_FROM_OS = 512};

/*--------------------------------------------------------------------*/

/* The state of the HeapMgr. */

/* The address of the start of the heap. */
static Chunk_T oHeapStart = NULL;

/* The address immediately beyond the end of the heap. */
static Chunk_T oHeapEnd = NULL;

/* The free list is a list of all free Chunks. It is kept in
   ascending order by memory address. */
static Chunk_T oFreeList = NULL;

/*--------------------------------------------------------------------*/
/* Request more memory from the operating system -- enough to store
   uUnits units. Create a new chunk, and and return it. */
static Chunk_T HeapMgr_getMoreMemory(size_t uUnits)
{
   Chunk_T oChunk; /* new chunk to build and return */
   Chunk_T oNewHeapEnd; /* temp variable needed so that we can error 
                           check before re-assigning oHeapEnd*/
   size_t uBytes; /* requested number of bytes */

   /* always use at least 512 units */
   if (uUnits < (size_t)MIN_UNITS_FROM_OS)
      uUnits = (size_t)MIN_UNITS_FROM_OS;

   /* Convert units to bytes. */
   uBytes = Chunk_unitsToBytes(uUnits);
   
   /* calculate address of potential new heap end*/
   oNewHeapEnd = (Chunk_T)((char*)oHeapEnd + uBytes);

   /* Check for overflow */
   if (oNewHeapEnd < oHeapEnd)
      return NULL;

   /* system call: move the program break and error check*/
   if (brk(oNewHeapEnd) == -1)
      return NULL;

   /* select new chunk for returning */
   oChunk = oHeapEnd;

   /* update global var heap end */
   oHeapEnd = oNewHeapEnd;

   /* Set the fields of the new chunk. */
   Chunk_setUnits(oChunk, uUnits);
   Chunk_setNextInList(oChunk, NULL);
   Chunk_setPrevInList(oChunk, NULL);
   
   return oChunk;
}

/* Add oChunk to the front of the Free list ASSUMING
 * its status bit is already set correctly
 */
static void HeapMgr_addToList(Chunk_T oChunk)
{
   Chunk_T oOldFront; /* oChunk is the new front of the list, 
                         oOldFront is the old front of the list*/

   /* only let valid chunks be added to the list */
   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));
   
   /* clear fwd and bkwd links in oChunk */
   Chunk_setNextInList(oChunk, NULL);
   Chunk_setPrevInList(oChunk, NULL);

   /* initialize the freelist if nessecary */
   if (oFreeList == NULL)
   {
      oFreeList = oChunk;
      return;
   }

   /* assign roles to chunks of interest */
   oOldFront = oFreeList;
   oFreeList = oChunk;

   /* update links to reflect those roles */
   Chunk_setNextInList(oFreeList, oOldFront);
   Chunk_setPrevInList(oFreeList, NULL);
   Chunk_setPrevInList(oOldFront, oFreeList);

   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));
   /* not correct to assert checker is valid because we havent had 
      the chance to coalesce yet */
   return;
}


/* Remove then return oChunk from free list assuming its status bit 
 * has been set to INUSE
 */
static Chunk_T HeapMgr_removeFromList(Chunk_T oChunk)
{
   Chunk_T oPrevChunk; /* previous chunk in list */
   Chunk_T oNextChunk; /* next chunk in list */
   Chunk_T oNewFront; /* oNewFront needed for case of removing front */

   /* cant remove an item from an empty list */
   assert(oFreeList != NULL);
   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));

   /* case for removing front of list*/
   if (oChunk == oFreeList)
   {
      /* assign second element in list as new front */
      oNewFront = Chunk_getNextInList(oChunk);

      /* case for removing chunk from length one list */
      if (oNewFront == NULL)
      {
         oFreeList = NULL;
         return oChunk;
      }
      /* update oFreeList to oNewFront */
      oFreeList = oNewFront;

      /* restring links accordingly */
      Chunk_setPrevInList(oFreeList, NULL);
      Chunk_setNextInList(oChunk, NULL);
      return oChunk;
   }

   /* get prev and next chunk */
   oPrevChunk = Chunk_getPrevInList(oChunk);
   oNextChunk = Chunk_getNextInList(oChunk);

   /* we are now at the case where the length of the list is at least 2
    * this assertion ensures that that is true. */
   assert((oPrevChunk != NULL) || (oNextChunk != NULL));
   
   /* case for removing end of list */
   if (oNextChunk == NULL)
   {
      Chunk_setNextInList(oPrevChunk, NULL);
      Chunk_setPrevInList(oChunk, NULL);
      return oChunk;
   }

   /* most general case, removing a node with non null nodes 
      to the left and right */
   Chunk_setNextInList(oPrevChunk, oNextChunk);
   Chunk_setPrevInList(oNextChunk, oPrevChunk);

   /* clear links in chunk to be removed */
   Chunk_setNextInList(oChunk, NULL);
   Chunk_setPrevInList(oChunk, NULL);

   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));

   return oChunk; 
}


/* Split oChunk into two valid logical Chunks, the first of which has 
 * length uUnits and the second having the rest of the physical chunks.
 * Return tail.
 * The status bits of the two Chunks are undefined.
 * The lengths of the two Chunks are reset.
 */
static Chunk_T HeapMgr_splitGetTail(Chunk_T oChunk, size_t uUnits)
{
   /* oChunk is used as an alias for oFront after oTail is allocated*/
   Chunk_T oTail  = NULL; /* tail end of split oChunk */
   size_t  uBytes; /* distance in bytes between oChunk and oTail */
   size_t  uTotalUnits; /* total units of chunk before split */

   /* only allow for the splitting of valid chunks */
   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));

   /* compute address of oTail */
   uBytes = Chunk_unitsToBytes(uUnits);
   oTail = (Chunk_T)((char*)oChunk + uBytes);

   /* compute and set length of tail */
   uTotalUnits = Chunk_getUnits(oChunk);
   assert(uTotalUnits > uUnits);
   Chunk_setUnits(oTail, uTotalUnits - uUnits);

   /* set new, post split length of oChunk (the front) */
   Chunk_setUnits(oChunk, uUnits);

   /* the split chunks are individually valid */
   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));
   assert(Chunk_isValid(oTail, oHeapStart, oHeapEnd));

   /* Their sizes sum up to the total */
   assert(Chunk_getUnits(oChunk) + Chunk_getUnits(oTail) ==uTotalUnits);

   /* sizes are set correctly and the two are adjacent */
   assert(Chunk_getNextInMem(oChunk, oHeapEnd) == oTail);
   assert(Chunk_getPrevInMem(oTail,oHeapStart) == oChunk);

   return oTail;
}


/* Given a free chunk with another free chunk next to it (forwards 
 * in memory), remove both chunks from the list, merge the chunks, and 
 * then both add to list and return the merged, valid, logical chunk 
 *
 * Assumes that the Chunk next in memory to oChunk is free.
 */
static Chunk_T HeapMgr_coalesceForward(Chunk_T oChunk)
{
   Chunk_T oNext = NULL; /* the next chunk in memory */
   size_t  uChunkUnits;  /* number of units of oChunk */
   size_t  uNextUnits;   /* number of units of oNext */
   size_t  uTotalUnits;  /* sum of units in oChunk and oNext */
   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));
   /* doesnt make sense to assert checker is valid here because we 
      haven't finished coalescing yet*/

   /* Get the next chunk in memory */
   oNext = Chunk_getNextInMem(oChunk, oHeapEnd);

   assert(Chunk_isValid(oNext, oHeapStart, oHeapEnd));
   /* enforce our assumption that oNext is free */
   assert(Chunk_getStatus(oNext) == CHUNK_FREE);

   /* compute total units */
   uChunkUnits = Chunk_getUnits(oChunk);
   uNextUnits = Chunk_getUnits(oNext);
   uTotalUnits = uChunkUnits + uNextUnits;

   /* remove both chunks from list */
   (void)HeapMgr_removeFromList(oChunk);
   (void)HeapMgr_removeFromList(oNext);

   /* configure header and footer of merged chunk */
   Chunk_setUnits(oChunk, uTotalUnits);
   Chunk_setStatus(oChunk, CHUNK_FREE);
   
   /* add merged chunk to list and return it */
   HeapMgr_addToList(oChunk);
   return oChunk;
}

/* Given a free chunk with another free chunk next to it (backwards 
 * in memory), remove both chunks from the list, merge the chunks, and 
 * then both add to list and return the merged, valid, logical chunk 
 *
 * Assumes that the Chunk previous in memory to oChunk is free.
 */
static Chunk_T HeapMgr_coalesceBackward(Chunk_T oChunk)
{
   Chunk_T oPrev = NULL; /* chunk in memory behind oChunk */
   size_t  uChunkUnits; /* unit length of oChunk */
   size_t  uPrevUnits; /* unit length of oPrev */
   size_t  uTotalUnits; /* sum unit length of oChunk and oPrev */

   /* get chunk previous in memory to oChunk*/
   oPrev = Chunk_getPrevInMem(oChunk, oHeapStart);

   /* compute total units */
   uChunkUnits = Chunk_getUnits(oChunk);
   uPrevUnits = Chunk_getUnits(oPrev);
   uTotalUnits = uChunkUnits + uPrevUnits;

   /* remove both chunks from list */
   (void)HeapMgr_removeFromList(oChunk);
   (void)HeapMgr_removeFromList(oPrev);
   /* point oChunk to the start address of merged logical chunk */
   oChunk = oPrev;

   /* properly configure header and footer of merged chunk */
   Chunk_setUnits(oChunk, uTotalUnits);
   Chunk_setStatus(oChunk, CHUNK_FREE);

   /* add merged chunk to list and return it */
   HeapMgr_addToList(oChunk);
   return oChunk;
}


void *HeapMgr_malloc(size_t uBytes)
{
   size_t uUnits; /* units requested */
   Chunk_T oChunk = NULL; /* memory chunk to eventually return */
   Chunk_T oTail  = NULL; /* used for coalescing */
   
   /* can't request 0 bytes */
   if (uBytes == 0) return NULL;

   /* (1) initialize */
   if (oHeapStart == NULL)
   {
      oHeapStart = (Chunk_T)sbrk(0);
      oHeapEnd = oHeapStart;
   }
   /* assert checker is valid at leading edge */
   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));   

   /* (2) determine units needed */
   uUnits = Chunk_bytesToUnits(uBytes);
   
   /* (3) for each chunk in the free list */
   for (oChunk = oFreeList;
        oChunk != NULL;
        oChunk = Chunk_getNextInList(oChunk))
   {
      /* if the current free list chunk is big enough */
      if (Chunk_getUnits(oChunk) >= uUnits)
      {
         /* if the chunk is too big, split */
         if ((Chunk_getUnits(oChunk) - uUnits) >= SPLIT_THRESHOLD)
         {
            /* remove from list */
            oChunk = HeapMgr_removeFromList(oChunk);
                       
            /* split */
            oTail = HeapMgr_splitGetTail(oChunk, uUnits);

            /* set tail to free and add to list */
            Chunk_setStatus(oTail, CHUNK_FREE);
            HeapMgr_addToList(oTail);

            /* set head to in use and then return it */
            Chunk_setStatus(oChunk, CHUNK_INUSE);

            /* assert checker is valid at trailing edge of malloc */
            assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
            
            /* return payload */
            return Chunk_toPayload(oChunk);   
         }
         /* else if chunk not too big, rm from list and set status */
         oChunk = HeapMgr_removeFromList(oChunk);
         Chunk_setStatus(oChunk, CHUNK_INUSE);

         /* assert checker is valid at trailing edge of malloc */
         assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));   
         /* return payload */
         return Chunk_toPayload(oChunk);   
      }
   }
   /* (4) get more memory if needed */
   oChunk = HeapMgr_getMoreMemory(uUnits);
   /* error check */
   if (oChunk == NULL)
   {
      /* assert checker is valid at trailing edge of malloc */
      assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));   
      return NULL;
   }
   /*(4.1) set the status add the newly created chunk to the list */
   Chunk_setStatus(oChunk, CHUNK_FREE);
   HeapMgr_addToList(oChunk);

   /* coalesce backward if needed */
   if((Chunk_getPrevInMem(oChunk, oHeapStart) != NULL) &&
      Chunk_getStatus(Chunk_getPrevInMem(oChunk, oHeapStart))
      == CHUNK_FREE)
      oChunk = HeapMgr_coalesceBackward(oChunk);
   
   /* if the chunk is too big, split! */
   if ((Chunk_getUnits(oChunk) - uUnits) >= SPLIT_THRESHOLD)
   {
      /* start the split by removing chunk to split */
      oChunk = HeapMgr_removeFromList(oChunk);

      /* split */
      oTail = HeapMgr_splitGetTail(oChunk, uUnits);

      /* set tail to free and add to list */
      Chunk_setStatus(oTail, CHUNK_FREE);
      HeapMgr_addToList(oTail);

      /* set allocated memory to in use */
      Chunk_setStatus(oChunk, CHUNK_INUSE);

      /* assert checker is valid at trailing edge of malloc */
      assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));

      /* return payload */
      return Chunk_toPayload(oChunk);   
   }
   /* else if the chunk is a good size, allocate it! */
   /* remove from list and set to in use */
   (void) HeapMgr_removeFromList(oChunk);
   Chunk_setStatus(oChunk, CHUNK_INUSE);

   /* assert checker is valid at trailing edge of malloc */
   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));

   /* return payload */
   return Chunk_toPayload(oChunk);
}

void HeapMgr_free(void *pv)
{
   Chunk_T oChunk = NULL; /* logical chunk that owns payload pv */

   /* validate params */
   assert(pv != NULL);
   
   /* assert that heap section is valid at leading edge of free */
   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));   

   /* (0) get the chunk from payload */
   oChunk = Chunk_fromPayload(pv);

   /* (1) set status of the given chunk to free */
   Chunk_setStatus(oChunk, CHUNK_FREE);

   /* (2) Add to the list */
   HeapMgr_addToList(oChunk);
   
   /* coalesce forward if needed */
   if((Chunk_getNextInMem(oChunk, oHeapEnd) != NULL) &&
      Chunk_getStatus(Chunk_getNextInMem(oChunk, oHeapEnd))
       == CHUNK_FREE)
      oChunk = HeapMgr_coalesceForward(oChunk);

   /* coalesce backward if needed */
   if((Chunk_getPrevInMem(oChunk, oHeapStart) != NULL) &&
      Chunk_getStatus(Chunk_getPrevInMem(oChunk, oHeapStart))
      == CHUNK_FREE)
      oChunk = HeapMgr_coalesceBackward(oChunk);
   
   /* assert that heap section is valid at trailing edge of free */
   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
   return;
}
