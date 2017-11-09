/*--------------------------------------------------------------------*/
/* heapmgrbase.c                                                      */
/* Author: Nate Wilson                                                */
/* Author: Narek Galstyan                                             */
/*--------------------------------------------------------------------*/

#include "heapmgr.h"
#include "checker2.h"
#include "chunk.h"
#include <stddef.h>
#include <assert.h>

#define __USE_XOPEN_EXTENDED
#include <unistd.h>

/* In lieu of a boolean data type. */
enum {FALSE, TRUE};

/* Minimum size of the free (logical) chunk to split */
enum {   SPLIT_THRESHOLD = 3 };

/* The minimum number of units to request of the OS. */
enum { MIN_UNITS_FROM_OS = 512 };

/* number of bins in freelist array */
enum {IBINCOUNT = 1024};


/*--------------------------------------------------------------------*/

/* The state of the HeapMgr. */

/* The address of the start of the heap. */
static Chunk_T oHeapStart = NULL;

/* The address immediately beyond the end of the heap. */
static Chunk_T oHeapEnd = NULL;

/* an array of pointers to structres like oFreeList each of which 
 * stores free memory chunks of a particular size or a range of sizes*/
static Chunk_T aoBins[IBINCOUNT]; /*in bss so init. all 0*/

/*--------------------------------------------------------------------*/
/* Request more memory from the operating system -- enough to store
   uUnits units. Create a new chunk, and and return it. */
static Chunk_T HeapMgr_getMoreMemory(size_t uUnits)
{
   Chunk_T oChunk;
   Chunk_T oNewHeapEnd;
   size_t uBytes;

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

   /*system call: move the program break and error check*/
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
   Chunk_T oOldFront; /* chunk to store the old front of the list */
   size_t  uIndex = Chunk_getUnits(oChunk); /* use to index into bin */

   /* ceil index @ 1023 */
   if(uIndex > (size_t) IBINCOUNT - 1) uIndex = (size_t) IBINCOUNT - 1;

   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));

   /* clear chunk links */
   Chunk_setNextInList(oChunk, NULL);
   Chunk_setPrevInList(oChunk, NULL);

   /* initialize a free list if nessecary */
   if (aoBins[uIndex] == NULL)
   {
      aoBins[uIndex] = oChunk;
      return;
   }

   /* select nodes of interest */
   oOldFront = aoBins[uIndex];
   aoBins[uIndex] = oChunk;

   /* set links */
   Chunk_setNextInList(aoBins[uIndex], oOldFront);
   Chunk_setPrevInList(aoBins[uIndex], NULL);
   Chunk_setPrevInList(oOldFront, aoBins[uIndex]);

   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));
   return;
}

/* Remove oChunk from free list without changing the status bit
 * Return the removed Chunk which is the same as oChunk
 */
static Chunk_T HeapMgr_removeFromList(Chunk_T oChunk)
{
   Chunk_T oPrevChunk;
   Chunk_T oNextChunk;
   Chunk_T oNewFront;
   size_t  uIndex = Chunk_getUnits(oChunk);
   if(uIndex > (size_t) IBINCOUNT - 1) uIndex = (size_t) IBINCOUNT - 1;
   assert(aoBins[uIndex] != NULL);
   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));

   /* case for removing front of list*/
   if (oChunk == aoBins[uIndex])
   {
      oNewFront = Chunk_getNextInList(oChunk);
      /* case for removing chunk from length one list */
      if (oNewFront == NULL)
      {
         aoBins[uIndex] = NULL;
         return oChunk;
      }
      aoBins[uIndex] = oNewFront;
      Chunk_setPrevInList(aoBins[uIndex], NULL);
      Chunk_setNextInList(oChunk, NULL);
      return oChunk;
   }

   /* get prev and next chunk */
   oPrevChunk = Chunk_getPrevInList(oChunk);
   oNextChunk = Chunk_getNextInList(oChunk);
   assert((oPrevChunk != NULL) || (oNextChunk != NULL));
   /* case for removing end of list */
   if (oNextChunk == NULL)
   {
      Chunk_setNextInList(oPrevChunk, NULL);
      Chunk_setPrevInList(oChunk, NULL);
      return oChunk;
   }
   
   Chunk_setNextInList(oPrevChunk, oNextChunk);
   Chunk_setPrevInList(oNextChunk, oPrevChunk);
   Chunk_setNextInList(oChunk, NULL);
   Chunk_setPrevInList(oChunk, NULL);

   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));


   return oChunk; 
}


/* Split oChunk into two valid logical Chunks first of which has 
 * length uUnits and the second has the rest of physical chunks in it
 * The status bits of the two Chunks are undefined.
 * The lengths of the two Chunks are set accordingly so 
 * Chunk_getNextInMemory with the first split Chunk will 
 * return the second split Chunk
 */
static Chunk_T HeapMgr_splitGetTail(Chunk_T oChunk, size_t uUnits)
{
   /* oChunk is used as an alias for oFront after oTail is allocated*/
   Chunk_T oTail  = NULL;
   size_t  uBytes;
   size_t  uTotalUnits;
   
   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));
   
   uBytes = Chunk_unitsToBytes(uUnits);
   uTotalUnits = Chunk_getUnits(oChunk);
   oTail = (Chunk_T)((char*)oChunk + uBytes);
   assert(uTotalUnits > uUnits);
   Chunk_setUnits(oTail, uTotalUnits - uUnits);

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

/* Assume the Chunk which is next to oChunk in memory is free
 * Coalesce the two and add the merged chunk instead of the 
 * two old ones to the Free list. Return the coalesced chunk
 * as a result.
 */
static Chunk_T HeapMgr_coalesceForward(Chunk_T oChunk)
{
   Chunk_T oNext = NULL;
   size_t  uChunkUnits;
   size_t  uNextUnits;
   size_t  uTotalUnits;
   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));

   /* get adjacent chunk*/
   oNext = Chunk_getNextInMem(oChunk, oHeapEnd);
   assert(Chunk_isValid(oNext, oHeapStart, oHeapEnd));
   assert(Chunk_getStatus(oNext) == CHUNK_FREE);

   /* compute total units */
   uChunkUnits = Chunk_getUnits(oChunk);
   uNextUnits = Chunk_getUnits(oNext);
   uTotalUnits = uChunkUnits + uNextUnits;

   /* rm both from list */
   (void)HeapMgr_removeFromList(oChunk);
   (void)HeapMgr_removeFromList(oNext);

   /* make chunk valid */
   Chunk_setUnits(oChunk, uTotalUnits);
   Chunk_setStatus(oChunk, CHUNK_FREE);
   /* add back to list */
   HeapMgr_addToList(oChunk);
   return oChunk;
}

/* Assume the Chunk which is the previous of oChunk in memory is free
 * Coalesce the two and add the merged chunk instead of the 
 * two old ones to the Free list. Return the coalesced chunk as
 * as a result.
 */
static Chunk_T HeapMgr_coalesceBackward(Chunk_T oChunk)
{
   Chunk_T oPrev = NULL;
   size_t  uChunkUnits;
   size_t  uPrevUnits;
   size_t  uTotalUnits;

   /* get chunk adjacent in memory */
   oPrev = Chunk_getPrevInMem(oChunk, oHeapStart);

   /* compute total units */
   uChunkUnits = Chunk_getUnits(oChunk);
   uPrevUnits = Chunk_getUnits(oPrev);
   uTotalUnits = uChunkUnits + uPrevUnits;

   /* rm both chunks from list */
   (void)HeapMgr_removeFromList(oChunk);
   (void)HeapMgr_removeFromList(oPrev);

   /* make chunk valid */
   oChunk = oPrev;
   Chunk_setUnits(oChunk, uTotalUnits);
   Chunk_setStatus(oChunk, CHUNK_FREE);

   /* add back to list */
   HeapMgr_addToList(oChunk);
   return oChunk;
}

void *HeapMgr_malloc(size_t uBytes)
{
   size_t uUnits; /* units requested by client */
   size_t uIndex; /* used to index into a bin */
   Chunk_T oChunk = NULL; /* chunk pntr to eventually return */
   Chunk_T oTail  = NULL; /* used for splitting case */
   
   if (uBytes == 0)
      return NULL;

   /* (1) initialize */
   if (oHeapStart == NULL)
   {
      oHeapStart = (Chunk_T)sbrk(0);
      oHeapEnd = oHeapStart;
   }
   assert(Checker_isValid(oHeapStart, oHeapEnd, aoBins, IBINCOUNT));
   /* (2) determine units needed */
   uUnits = Chunk_bytesToUnits(uBytes);
   /* assign uIndex properly */
   uIndex = uUnits;
   if ( uIndex > (size_t) IBINCOUNT - 1)
      uIndex = (size_t) IBINCOUNT - 1;

   /* increment if necessary */   
   while(uIndex < ((size_t) IBINCOUNT - 1) && aoBins[uIndex] == NULL)
      uIndex++;

   /* (3) for each chunk in the free list of the correct bin */
   for (oChunk = aoBins[uIndex];
        oChunk != NULL;
        oChunk = Chunk_getNextInList(oChunk))
      {
         /* if the current free list chunk is not big enough 
          * relevant only for bin 1023 
          */
         if(Chunk_getUnits(oChunk) < uUnits) continue;
            
         /* if the chunk is too big, rm from free list */
         if ((Chunk_getUnits(oChunk) - uUnits) >= SPLIT_THRESHOLD)
         {
            (void)HeapMgr_removeFromList(oChunk);
                       
            /* ochunk needs to be a valid logical chunk */
            oTail = HeapMgr_splitGetTail(oChunk, uUnits);
            
            Chunk_setStatus(oTail, CHUNK_FREE);
            HeapMgr_addToList(oTail);
            
            Chunk_setStatus(oChunk, CHUNK_INUSE);
            assert(Checker_isValid(oHeapStart, oHeapEnd,
                                   aoBins, IBINCOUNT));   
            /* given a head, set in use, set address of payload */
            return Chunk_toPayload(oChunk);   
         }
         /* if chunk not too big */
         oChunk = HeapMgr_removeFromList(oChunk);
         Chunk_setStatus(oChunk, CHUNK_INUSE);
         
         assert(Checker_isValid(oHeapStart, oHeapEnd, aoBins, IBINCOUNT));   
         return Chunk_toPayload(oChunk);   
      }
   
   /* (4) get more memory if needed */
   oChunk = HeapMgr_getMoreMemory(uUnits);
   if (oChunk == NULL)
   {
      assert(Checker_isValid(oHeapStart, oHeapEnd, aoBins, IBINCOUNT)); 
      return NULL;
   }
   /*(4.1) set the status add the newly created chunk to the list*/
   Chunk_setStatus(oChunk, CHUNK_FREE);
   HeapMgr_addToList(oChunk);

   /* coalesce backward if needed */
   if((Chunk_getPrevInMem(oChunk, oHeapStart) != NULL) &&
      Chunk_getStatus(Chunk_getPrevInMem(oChunk, oHeapStart))
      == CHUNK_FREE)
      oChunk = HeapMgr_coalesceBackward(oChunk);
   
   assert(Checker_isValid(oHeapStart, oHeapEnd, aoBins, IBINCOUNT));
      
   /* if the chunk is too big, rm from free list */
   if ((Chunk_getUnits(oChunk) - uUnits) >= SPLIT_THRESHOLD)
   {
      oChunk = HeapMgr_removeFromList(oChunk);
      
      oTail = HeapMgr_splitGetTail(oChunk, uUnits);
   
      Chunk_setStatus(oTail, CHUNK_FREE);
      HeapMgr_addToList(oTail); /* addToList sets the status bit*/

      Chunk_setStatus(oChunk, CHUNK_INUSE);
      assert(Checker_isValid(oHeapStart, oHeapEnd, aoBins, IBINCOUNT));
      /* given a head, get address of payload */
      return Chunk_toPayload(oChunk);   
   }
   /* remove chunk from free list, update status, and return */
   (void) HeapMgr_removeFromList(oChunk);
   Chunk_setStatus(oChunk, CHUNK_INUSE);

   /* assert check is valid at trailing edge of malloc */
   assert(Checker_isValid(oHeapStart, oHeapEnd, aoBins, IBINCOUNT));
   return Chunk_toPayload(oChunk);
}

void HeapMgr_free(void *pv)
{
   Chunk_T oChunk = NULL;
   assert(pv != NULL);
   assert(Checker_isValid(oHeapStart, oHeapEnd, aoBins, IBINCOUNT));
   /* (0) get the chunk from payload */
   oChunk = Chunk_fromPayload(pv);
   /* (1) set status of the given chunk to free */
   Chunk_setStatus(oChunk, CHUNK_FREE);

   /* (2) Add to the list */
   HeapMgr_addToList(oChunk); /* addToList sets the status bit*/
   
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

   assert(Checker_isValid(oHeapStart, oHeapEnd, aoBins, IBINCOUNT));
   return;
}
