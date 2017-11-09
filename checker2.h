/*--------------------------------------------------------------------*/
/* checker2.h                                                         */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#ifndef CHECKER2_INCLUDED
#define CHECKER2_INCLUDED

#include "chunk.h"

/* Return 1 (TRUE) if the heap is in a valid state, or 0 (FALSE)
   otherwise. The heap is defined by parameters oHeapStart (the address
   of the start of the heap), oHeapEnd (the address immediately 
   beyond the end of the heap), and aoBins (an array of iBinCount bins,
   where each bin contains free chunks). */

int Checker_isValid(Chunk_T oHeapStart, Chunk_T oHeapEnd,
   Chunk_T aoBins[], int iBinCount);

#endif
