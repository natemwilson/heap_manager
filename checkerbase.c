/*--------------------------------------------------------------------*/
/* checkerbase.c                                                      */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#include "checkerbase.h"
#include <stdio.h>
#include <assert.h>

/* In lieu of a boolean data type. */
enum {FALSE, TRUE};

/*--------------------------------------------------------------------*/

int Checker_isValid(Chunk_T oHeapStart, Chunk_T oHeapEnd,
   Chunk_T oFreeList)
{
   Chunk_T oChunk;
   Chunk_T oPrevChunk;
   Chunk_T oTortoiseChunk;
   Chunk_T oHareChunk;

   /* Do oHeapStart and oHeapEnd have non-NULL values? */
   if (oHeapStart == NULL)
   {
      fprintf(stderr, "The heap start is uninitialized\n"); 
      return FALSE; 
   }
   if (oHeapEnd == NULL)
   {
      fprintf(stderr, "The heap end is uninitialized\n");
      return FALSE;
   }

   /* If the heap is empty, is the free list empty too? */
   if (oHeapStart == oHeapEnd)
   {
      if (oFreeList == NULL)
         return TRUE;
      else
      {
         fprintf(stderr, "The heap is empty, but the list is not.\n");
         return FALSE;
      }
   }

   /* Traverse memory. */

   for (oChunk = oHeapStart;
        oChunk != NULL;
        oChunk = Chunk_getNextInMem(oChunk, oHeapEnd))

      /* Is the chunk valid? */
      if (! Chunk_isValid(oChunk, oHeapStart, oHeapEnd))
      {
         fprintf(stderr, "Traversing memory detected a bad chunk\n");
         return FALSE;
      }

   /* Is the list devoid of cycles? Use Floyd's algorithm to find out.
      See the Wikipedia "Cycle detection" page for a description. */

   oTortoiseChunk = oFreeList;
   oHareChunk = oFreeList;
   if (oHareChunk != NULL)
      oHareChunk = Chunk_getNextInList(oHareChunk);
   while (oHareChunk != NULL)
   {
      if (oTortoiseChunk == oHareChunk)
      {
         fprintf(stderr, "The list has a cycle\n");  
         return FALSE;
      }
      /* Move oTortoiseChunk one step. */
      oTortoiseChunk = Chunk_getNextInList(oTortoiseChunk);
      /* Move oHareChunk two steps, if possible. */
      oHareChunk = Chunk_getNextInList(oHareChunk);
      if (oHareChunk != NULL)
         oHareChunk = Chunk_getNextInList(oHareChunk);
   }

   /* Traverse the free list. */

   oPrevChunk = NULL;
   for (oChunk = oFreeList;
        oChunk != NULL;
        oChunk = Chunk_getNextInList(oChunk))
   {
      /* Is the chunk valid? */
      if (! Chunk_isValid(oChunk, oHeapStart, oHeapEnd))
      {
         fprintf(stderr, "Traversing the list detected a bad chunk\n");
         return FALSE;
      }

      /* Is the chunk in the proper place in the list? */
      if ((oPrevChunk != NULL) && (oPrevChunk >= oChunk))
      {
         fprintf(stderr, "The list is unordered\n"); 
         return FALSE;
      }

      /* Is the previous chunk in memory in use? */
      if ((oPrevChunk != NULL) &&
          (Chunk_getNextInMem(oPrevChunk, oHeapEnd) == oChunk))
      {
         fprintf(stderr, "The heap contains contiguous free chunks\n");
         return FALSE;
      }
      
      oPrevChunk = oChunk;
   }

   return TRUE;
}
