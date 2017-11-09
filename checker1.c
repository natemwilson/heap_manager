/*--------------------------------------------------------------------*/
/* checker1.c                                                         */
/* Author: Nathaniel M. Wilson                                        */
/* Author: Narek Galstyan                                             */
/*--------------------------------------------------------------------*/

#include "checker1.h"
#include <stdio.h>
#include <assert.h>

/*In lieu of a boolean data type. */
enum {FALSE, TRUE};


int Checker_isValid(Chunk_T oHeapStart, Chunk_T oHeapEnd,
                    Chunk_T oFreeList) {

   Chunk_T oChunk;         /* current chunk in traversals */
   Chunk_T oPrevChunk;     /* previous chunk in traversals */
   Chunk_T oNextChunk;     /* next chunk in tranversals*/
   Chunk_T oTortoiseChunk; /* used in cycle detection */
   Chunk_T oHareChunk;     /* used in cycel detection */
   Chunk_T oFreeListEnd;   /* points to the last chunk in LinkedList */

   /* check for an initialized heap */
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
   
   /* Traverse memory through forward links. */
   for (oChunk = oHeapStart;
        oChunk != NULL;
        oChunk = Chunk_getNextInMem(oChunk, oHeapEnd))
   {
      /* Is the chunk valid? */
      if (! Chunk_isValid(oChunk, oHeapStart, oHeapEnd))
      {
         fprintf(stderr, "Traversing memory detected a bad chunk\n");
         return FALSE;
      }  
   }

   /* Traverse memory through backward links. */
   for (oChunk = Chunk_getPrevInMem(oHeapEnd, oHeapStart);
        oChunk != NULL;
        oChunk = Chunk_getPrevInMem(oChunk, oHeapStart))
   {
      /* Is the chunk valid? */
      if (! Chunk_isValid(oChunk, oHeapStart, oHeapEnd))
      {
         fprintf(stderr, "Backward traversing memory"
                 " detected a bad chunk\n");
         return FALSE;
      }

   }

   /* fwd cycle detection */
   oFreeListEnd = NULL;
   oTortoiseChunk = oFreeList;
   oHareChunk = oFreeList;
   if (oHareChunk != NULL) {
      oFreeListEnd = oHareChunk;
      oHareChunk = Chunk_getNextInList(oHareChunk);
   }
      while (oHareChunk != NULL)
   {
      /* by the end of the loop oFreeListEnd stores 
       * the end of the loop so the backward cycles could be found
       */
      oFreeListEnd = oHareChunk;
      if (oTortoiseChunk == oHareChunk)
      {
         fprintf(stderr, "The list has a forward cycle\n");  
         return FALSE;
      }

      /* do List links point to meaningful positions?*/
      if(!Chunk_isValid(oHareChunk, oHeapStart, oHeapEnd))
      {
         fprintf(stderr, "Forward link of some element in free"
                 " list is corrupted\n");
         return FALSE;
      }
      
      /* Move oTortoiseChunk one step. */
      oTortoiseChunk = Chunk_getNextInList(oTortoiseChunk);
      /* Move oHareChunk two steps, if possible. */
      oHareChunk = Chunk_getNextInList(oHareChunk);
      if (oHareChunk != NULL) {
         /* do List links point to meaningful positions?*/
         if(!Chunk_isValid(oHareChunk, oHeapStart, oHeapEnd))
         {
            fprintf(stderr, "Forward link of some element in free"
                    " list is corrupted\n");
            return FALSE;
         }
         
         oFreeListEnd = oHareChunk;
         /* because hare jumps in steps of two */
         oHareChunk = Chunk_getNextInList(oHareChunk);
      }
   }

 /* Is the list devoid of backward cycles? Use Floyd's algorithm to 
    find out.*/
   oTortoiseChunk = oFreeListEnd;
   oHareChunk = oFreeListEnd;

   /* if there is a free list and the length is more than one */
   if (oHareChunk != NULL)
   {
      /* do List links point to meaningful positions?*/
      if(!Chunk_isValid(oHareChunk, oHeapStart, oHeapEnd))
      {
         fprintf(stderr, "Backward link of the last element in the free"
                 " list is corrupted\n");
         return FALSE;
      }
      oHareChunk = Chunk_getPrevInList(oHareChunk);
   }

   /* There is no NULL terminator in the begining of the list so
    * the start of the free list will serve as a terminator */

   while (oHareChunk != NULL)
   {
      
      if (oTortoiseChunk == oHareChunk)
      {
         fprintf(stderr, "The list has a backward cycle\n");  
         return FALSE;
      }
      /* Move oTortoiseChunk one step. */
      oTortoiseChunk = Chunk_getPrevInList(oTortoiseChunk);
      /* Move oHareChunk two steps, if possible. */
      
      /* do List links point to meaningful positions?*/
      if(!Chunk_isValid(oHareChunk, oHeapStart, oHeapEnd))
      {
         fprintf(stderr, "Backward link of some element in free"
                 " list is corrupted\n");
         return FALSE;
      }
      oHareChunk = Chunk_getPrevInList(oHareChunk);
      
      if (oHareChunk != NULL) {
         
         /* do List links point to meaningful positions?*/
         if(!Chunk_isValid(oHareChunk, oHeapStart, oHeapEnd))
         {
            fprintf(stderr, "Backward link of some element in free"
                    " list is corrupted\n");
            return FALSE;
         }
         /* run hare run*/
         oHareChunk = Chunk_getPrevInList(oHareChunk);
      }
   }
   
   
   /* Traverse the free list. */

   oPrevChunk = NULL;
   oNextChunk = NULL;
   for (oChunk = oFreeList;
        oChunk != NULL;
        oChunk = Chunk_getNextInList(oChunk))
   {
      oPrevChunk = Chunk_getPrevInMem(oChunk, oHeapStart);
      oNextChunk = Chunk_getNextInMem(oChunk, oHeapEnd);
      
      /* Is the chunk valid? */
      if (! Chunk_isValid(oChunk, oHeapStart, oHeapEnd))
      {
         fprintf(stderr, "Traversing the list detected a bad chunk\n");
         return FALSE;
      }

      /*ensure status bit set correctly*/
      if (Chunk_getStatus(oChunk) == CHUNK_INUSE)
      {
         fprintf(stderr, "chunk in free list marked as in use.");
         return FALSE;
      }
      if ((oPrevChunk != NULL) &&
          (Chunk_getStatus(oPrevChunk) == CHUNK_FREE))
      {
         fprintf(stderr, "The heap contains contiguous free chunks"
                 " just after a memory in free list\n");
         return FALSE;
      }
    
      /* Is the next chunk in memory in use? */
      if ((oNextChunk != NULL) &&
          (Chunk_getStatus(oNextChunk) == CHUNK_FREE))
      {
         fprintf(stderr, "The heap contains contiguous free chunks"
                 " just before a memory in free list\n");
         return FALSE;
      }
   }

   /*Is the Current node in the linked list the next node of 
    * the previous one and the previous of the next one?*/
   oPrevChunk = NULL;
   oNextChunk = NULL;
   for (oChunk = oFreeList;
        oChunk != NULL;
        oChunk = Chunk_getNextInList(oChunk))
   {
      if(oChunk != oFreeList) /* if not in the first iteration*/
         oPrevChunk = Chunk_getPrevInList(oChunk);
      oNextChunk = Chunk_getNextInList(oChunk);
      /* The next of the previous */ 
      if(oPrevChunk != NULL &&
         Chunk_getNextInList(oPrevChunk) != oChunk)
      {
         fprintf(stderr, "Next of the previous is not the current in"
                 " the Linked List\n");
         return FALSE;
      }
       /* The previous of the next */
      if(oNextChunk != NULL &&
         Chunk_getPrevInList(oNextChunk) != oChunk)
      {
         fprintf(stderr, "Previous of the Next is not the current in"
                 " the Linked List\n");
         return FALSE;
      
       }
   }

   for (oChunk = oHeapStart;
        oChunk != NULL;
        oChunk = Chunk_getNextInMem(oChunk, oHeapEnd))
   {
      
      /* if Chunk is free, it should be in the free list */
      if(Chunk_getStatus(oChunk) == CHUNK_FREE) {
         /*Q:: is this ok?*/
         Chunk_T oCurrent = oFreeList;
         while(oCurrent != NULL && oCurrent != oChunk) {
            oCurrent = Chunk_getNextInList(oCurrent);
         }
         if(oCurrent != oChunk)
         {
            fprintf(stderr, "Status bit of the chunk is set FREE but"
                    " it is not in free list\n");
            return FALSE;
         }
      }
   }   
   
   return TRUE;

}
