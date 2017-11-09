/*--------------------------------------------------------------------*/
/* chunkbase.c                                                        */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#include "chunkbase.h"
#include <stddef.h>
#include <stdio.h>
#include <assert.h>

/*--------------------------------------------------------------------*/

/* Physically a Chunk is a structure consisting of a number of units
   and an address. Logically a Chunk consists of multiple such 
   structures. */
   
struct Chunk
{
   /* The number of units in the Chunk. */
   size_t uUnits;

   /* The address of the next Chunk. */
   Chunk_T oNextChunk;
};

/*--------------------------------------------------------------------*/

size_t Chunk_bytesToUnits(size_t uBytes)
{
   size_t uUnits;
   uUnits = ((uBytes - 1) / sizeof(struct Chunk)) + 1;
   uUnits++;  /* Allow room for a header. */
   return uUnits;
}

/*--------------------------------------------------------------------*/

size_t Chunk_unitsToBytes(size_t uUnits)
{
   return uUnits * sizeof(struct Chunk);
}

/*--------------------------------------------------------------------*/

void *Chunk_toPayload(Chunk_T oChunk)
{
   assert(oChunk != NULL);

   return (void*)(oChunk + 1);
}

/*--------------------------------------------------------------------*/

Chunk_T Chunk_fromPayload(void *pv)
{
   assert(pv != NULL);

   return (Chunk_T)pv - 1;
}

/*--------------------------------------------------------------------*/

size_t Chunk_getUnits(Chunk_T oChunk)
{
   assert(oChunk != NULL);

   return oChunk->uUnits;
}

/*--------------------------------------------------------------------*/

void Chunk_setUnits(Chunk_T oChunk, size_t uUnits)
{
   assert(oChunk != NULL);
   assert(uUnits >= MIN_UNITS_PER_CHUNK);

   oChunk->uUnits = uUnits;
}

/*--------------------------------------------------------------------*/

Chunk_T Chunk_getNextInList(Chunk_T oChunk)
{
   assert(oChunk != NULL);

   return oChunk->oNextChunk;
}

/*--------------------------------------------------------------------*/

void Chunk_setNextInList(Chunk_T oChunk, Chunk_T oNextChunk)
{
   assert(oChunk != NULL);

   oChunk->oNextChunk = oNextChunk;
}

/*--------------------------------------------------------------------*/

Chunk_T Chunk_getNextInMem(Chunk_T oChunk, Chunk_T oHeapEnd)
{
   Chunk_T oNextChunk;

   assert(oChunk != NULL);
   assert(oHeapEnd != NULL);
   assert(oChunk < oHeapEnd);

   oNextChunk = oChunk + Chunk_getUnits(oChunk);
   assert(oNextChunk <= oHeapEnd);

   if (oNextChunk == oHeapEnd)
      return NULL;
   return oNextChunk;
}

/*--------------------------------------------------------------------*/

int Chunk_isValid(Chunk_T oChunk,
                  Chunk_T oHeapStart, Chunk_T oHeapEnd)
{
   assert(oChunk != NULL);
   assert(oHeapStart != NULL);
   assert(oHeapEnd != NULL);

   if (oChunk < oHeapStart)
   {  fprintf(stderr, "A chunk starts before the heap start\n");
      return 0;
   }
   if (oChunk >= oHeapEnd)
   {  fprintf(stderr, "A chunk starts after the heap end\n");
      return 0;
   }
   if (oChunk + Chunk_getUnits(oChunk) > oHeapEnd)
   {  fprintf(stderr, "A chunk ends after the heap end\n");
      return 0;
   }
   if (Chunk_getUnits(oChunk) == 0)
   {  fprintf(stderr, "A chunk has zero units\n");
      return 0;
   }
   if (Chunk_getUnits(oChunk) < (size_t)MIN_UNITS_PER_CHUNK)
   {  fprintf(stderr, "A chunk has too few units\n");
      return 0;
   }
   return 1;
}

