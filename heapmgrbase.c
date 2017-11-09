/*--------------------------------------------------------------------*/
/* heapmgrbase.c                                                      */
/* Author: Bob Dondero (similar to K&R version)                       */
/*--------------------------------------------------------------------*/

#include "heapmgr.h"
#include "checkerbase.h"
#include "chunkbase.h"
#include <stddef.h>
#include <assert.h>

#define __USE_XOPEN_EXTENDED
#include <unistd.h>

/* In lieu of a boolean data type. */
enum {FALSE, TRUE};

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
   uUnits units. Create a new chunk, and either append it to the
   free list after oPrevChunk or increase the size of oPrevChunk.
   Return the address of the new (or enlarged) chunk. */

static Chunk_T HeapMgr_getMoreMemory(Chunk_T oPrevChunk,
   size_t uUnits)
{
   Chunk_T oChunk;
   Chunk_T oNewHeapEnd;
   size_t uBytes;

   if (uUnits < (size_t)MIN_UNITS_FROM_OS)
      uUnits = (size_t)MIN_UNITS_FROM_OS;

   /* Move the program break. */
   uBytes = Chunk_unitsToBytes(uUnits);
   oNewHeapEnd = (Chunk_T)((char*)oHeapEnd + uBytes);
   if (oNewHeapEnd < oHeapEnd)  /* Check for overflow */
      return NULL;
   if (brk(oNewHeapEnd) == -1)
      return NULL;
   oChunk = oHeapEnd;
   oHeapEnd = oNewHeapEnd;

   /* Set the fields of the new chunk. */
   Chunk_setUnits(oChunk, uUnits);
   Chunk_setNextInList(oChunk, NULL);

   /* Add the new chunk to the end of the free list. */
   if (oPrevChunk == NULL)
      oFreeList = oChunk;
   else
      Chunk_setNextInList(oPrevChunk, oChunk);

   /* Coalesce the new chunk and the previous one if appropriate. */
   if (oPrevChunk != NULL)
      if (Chunk_getNextInMem(oPrevChunk, oHeapEnd) == oChunk)
      {
         Chunk_setUnits(oPrevChunk,
            Chunk_getUnits(oPrevChunk) + uUnits);
         Chunk_setNextInList(oPrevChunk, NULL);
         oChunk = oPrevChunk;
      }

   return oChunk;
}

/*--------------------------------------------------------------------*/

/* If oChunk is close to the right size (as specified by uUnits),
   then splice oChunk out of the free list (using oPrevChunk to do
   so), and return oChunk. If oChunk is too big, split it and return
   the address of the tail end.  */

static Chunk_T HeapMgr_useChunk(Chunk_T oChunk,
   Chunk_T oPrevChunk, size_t uUnits)
{
   Chunk_T oNewChunk;
   size_t uChunkUnits;

   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));

   uChunkUnits = Chunk_getUnits(oChunk);
   
   /* If oChunk is close to the right size, then use it. */
   if (uChunkUnits < uUnits + (size_t)MIN_UNITS_PER_CHUNK)
   {
      if (oPrevChunk == NULL)
         oFreeList = Chunk_getNextInList(oChunk);
      else
         Chunk_setNextInList(oPrevChunk, Chunk_getNextInList(oChunk));
      return oChunk;
   }

   /* oChunk is too big, so use the tail end of it. */
   Chunk_setUnits(oChunk, uChunkUnits - uUnits);
   oNewChunk = Chunk_getNextInMem(oChunk, oHeapEnd);
   Chunk_setUnits(oNewChunk, uUnits);
   return oNewChunk;
}

/*--------------------------------------------------------------------*/

void *HeapMgr_malloc(size_t uBytes)
{
   Chunk_T oChunk;
   Chunk_T oPrevChunk;
   Chunk_T oPrevPrevChunk;
   size_t uUnits;

   if (uBytes == 0)
      return NULL;

   /* Step 1: Initialize the heap manager if this is the first call. */
   if (oHeapStart == NULL)
   {
      oHeapStart = (Chunk_T)sbrk(0);
      oHeapEnd = oHeapStart;
   }

   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));

   /* Step 2: Determine the number of units the new chunk should
      contain. */
   uUnits = Chunk_bytesToUnits(uBytes);

   /* Step 3: For each chunk in the free list... */
   oPrevPrevChunk = NULL;
   oPrevChunk = NULL;
   for (oChunk = oFreeList;
        oChunk != NULL;
        oChunk = Chunk_getNextInList(oChunk))
   {
      /* If oChunk is big enough, then use it. */
      if (Chunk_getUnits(oChunk) >= uUnits)
      {
         oChunk = HeapMgr_useChunk(oChunk, oPrevChunk, uUnits);
         assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
         return Chunk_toPayload(oChunk);
      }

      oPrevPrevChunk = oPrevChunk;
      oPrevChunk = oChunk;
   }

   /* Step 4: Ask the OS for more memory, and create a new chunk (or
      expand the existing chunk) at the end of the free list. */
   oChunk =  HeapMgr_getMoreMemory(oPrevChunk, uUnits);
   if (oChunk == NULL)
   {
      assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
      return NULL;
   }

   /* If the new large chunk was coalesced with the previous chunk,
      then reset the previous chunk. */
   if (oChunk == oPrevChunk)
      oPrevChunk = oPrevPrevChunk;

   /* Step 5: oChunk is big enough, so use it. */
   oChunk = HeapMgr_useChunk(oChunk, oPrevChunk, uUnits);
   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
   return Chunk_toPayload(oChunk);
}

/*--------------------------------------------------------------------*/

void HeapMgr_free(void *pv)
{
   Chunk_T oChunk;
   Chunk_T oNextChunk;
   Chunk_T oPrevChunk;

   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));

   if (pv == NULL)
      return;

   oChunk = Chunk_fromPayload(pv);

   assert(Chunk_isValid(oChunk, oHeapStart, oHeapEnd));

   /* Step 1: Traverse the free list to find the correct spot for
      oChunk. (The free list is kept in ascending order by memory
      address.) */
   oPrevChunk = NULL;
   oNextChunk = oFreeList;
   while ((oNextChunk != NULL) && (oNextChunk < oChunk))
   {
      oPrevChunk = oNextChunk;
      oNextChunk = Chunk_getNextInList(oNextChunk);
   }
   
   /* Step 2: Insert oChunk into the free list. */
   if (oPrevChunk == NULL)
      oFreeList = oChunk;
   else
      Chunk_setNextInList(oPrevChunk, oChunk);
   Chunk_setNextInList(oChunk, oNextChunk);  
   
   /* Step 3: If appropriate, coalesce the given chunk and the next
      one. */
   if (oNextChunk != NULL)
      if (Chunk_getNextInMem(oChunk, oHeapEnd) == oNextChunk)
      {
         Chunk_setUnits(oChunk,
            Chunk_getUnits(oChunk) + Chunk_getUnits(oNextChunk));
         Chunk_setNextInList(oChunk,
            Chunk_getNextInList(oNextChunk));
      }

   /* Step 4: If appropriate, coalesce the given chunk and the previous
      one. */
   if (oPrevChunk != NULL)
      if (Chunk_getNextInMem(oPrevChunk, oHeapEnd) == oChunk)
      {
         Chunk_setUnits(oPrevChunk,
            Chunk_getUnits(oPrevChunk) + Chunk_getUnits(oChunk));
         Chunk_setNextInList(oPrevChunk,
            Chunk_getNextInList(oChunk));
      }

   assert(Checker_isValid(oHeapStart, oHeapEnd, oFreeList));
}
