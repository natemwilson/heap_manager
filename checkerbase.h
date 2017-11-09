/*--------------------------------------------------------------------*/
/* checkerbase.h                                                      */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#ifndef CHECKERBASE_INCLUDED
#define CHECKERBASE_INCLUDED

#include "chunkbase.h"

/* Return 1 (TRUE) if the heap is in a valid state, or 0 (FALSE)
   otherwise. The heap is defined by parameters oHeapStart (the address
   of the start of the heap), oHeapEnd (the address immediately 
   beyond the end of the heap), and oFreeList (a list containing free
   chunks). */

int Checker_isValid(Chunk_T oHeapStart, Chunk_T oHeapEnd,
   Chunk_T oFreeList);

#endif
