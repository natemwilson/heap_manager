/*--------------------------------------------------------------------*/
/* chunkbase.h                                                        */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#ifndef CHUNKBASE_INCLUDED
#define CHUNKBASE_INCLUDED

#include <stddef.h>

/* A Chunk is a sequence of units. The first unit is a header that
   contains the number of units in the Chunk and, if the Chunk is
   free, the address of the next Chunk in the free list. The 
   subsequent units are the payload. */

typedef struct Chunk *Chunk_T;

/*--------------------------------------------------------------------*/

/* The minimum number of units that a Chunk can contain. */

enum {MIN_UNITS_PER_CHUNK = 2};

/*--------------------------------------------------------------------*/

/* Translate uBytes, a number of bytes, to units. Return the result. */

size_t Chunk_bytesToUnits(size_t uBytes);

/*--------------------------------------------------------------------*/

/* Translate uUnits, a number of units, to bytes. Return the result. */

size_t Chunk_unitsToBytes(size_t uUnits);

/*--------------------------------------------------------------------*/

/* Return the address of the payload of oChunk. */

void *Chunk_toPayload(Chunk_T oChunk);

/*--------------------------------------------------------------------*/

/* Return the Chunk whose payload is pointed to by pv. */

Chunk_T Chunk_fromPayload(void *pv);

/*--------------------------------------------------------------------*/

/* Return oChunk's number of units. */

size_t Chunk_getUnits(Chunk_T oChunk);

/*--------------------------------------------------------------------*/

/* Set oChunk's number of units to uUnits. */

void Chunk_setUnits(Chunk_T oChunk, size_t uUnits);

/*--------------------------------------------------------------------*/

/* Return oChunk's next Chunk in the free list, or NULL if there
   is no next Chunk. */

Chunk_T Chunk_getNextInList(Chunk_T oChunk);

/*--------------------------------------------------------------------*/

/* Set oChunk's next Chunk in the free list to oNextChunk. */

void Chunk_setNextInList(Chunk_T oChunk, Chunk_T oNextChunk);

/*--------------------------------------------------------------------*/

/* Return oChunk's next Chunk in memory, or NULL if there is no
   next Chunk. Use oHeapEnd to determine if there is no next
   Chunk. oChunk's number of units must be set properly for this
   function to work. */

Chunk_T Chunk_getNextInMem(Chunk_T oChunk, Chunk_T oHeapEnd);

/*--------------------------------------------------------------------*/

/* Return 1 (TRUE) if oChunk is valid, notably with respect to
   oHeapStart and oHeapEnd, or 0 (FALSE) otherwise. */

int Chunk_isValid(Chunk_T oChunk,
                  Chunk_T oHeapStart, Chunk_T oHeapEnd);

#endif

