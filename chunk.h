/*--------------------------------------------------------------------*/
/* chunk.h                                                            */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#ifndef CHUNK_INCLUDED
#define CHUNK_INCLUDED

#include <stddef.h>

/* A Chunk can be either free or in use. */
enum ChunkStatus {CHUNK_FREE, CHUNK_INUSE};

/* A Chunk is a sequence of Units.  The first Unit is a header that
   indicates the number of Units in the Chunk, whether the Chunk is
   free, and, if the Chunk is free, a pointer to the next Chunk in the
   free list. The last Unit is a footer that indicates the number of
   Units in the Chunk and, if the Chunk is free, a pointer to the
   previous Chunk in the free list. The Units between the header and
   footer are the payload. */

typedef struct Chunk *Chunk_T;

/*--------------------------------------------------------------------*/

/* The minimum number of units that a Chunk can contain. */

enum {MIN_UNITS_PER_CHUNK = 3};

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

/* Return the status of oChunk. */

enum ChunkStatus Chunk_getStatus(Chunk_T oChunk);

/*--------------------------------------------------------------------*/

/* Set the status of oChunk to eStatus. */

void Chunk_setStatus(Chunk_T oChunk, enum ChunkStatus eStatus);

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

/* Return oChunk's previous Chunk in the free list, or NULL if there
   is no previous Chunk. */

Chunk_T Chunk_getPrevInList(Chunk_T oChunk);

/*--------------------------------------------------------------------*/

/* Set oChunk's previous Chunk in the free list to oPrevChunk. */

void Chunk_setPrevInList(Chunk_T oChunk, Chunk_T oPrevChunk);

/*--------------------------------------------------------------------*/

/* Return oChunk's next Chunk in memory, or NULL if there is no
   next Chunk. Use oHeapEnd to determine if there is no next
   Chunk. oChunk's number of units must be set properly for this
   function to work. */

Chunk_T Chunk_getNextInMem(Chunk_T oChunk, Chunk_T oHeapEnd);

/*--------------------------------------------------------------------*/

/* Return oChunk's previous Chunk in memory, or NULL if there is no
   previous Chunk. Use oHeapStart to determine if there is no
   previous Chunk. The previous Chunk's number of units must be set
   properly for this function to work. */

Chunk_T Chunk_getPrevInMem(Chunk_T oChunk, Chunk_T oHeapStart);

/*--------------------------------------------------------------------*/

/* Return 1 (TRUE) if oChunk is valid, notably with respect to
   oHeapStart and oHeapEnd, or 0 (FALSE) otherwise. */

int Chunk_isValid(Chunk_T oChunk,
                  Chunk_T oHeapStart, Chunk_T oHeapEnd);

#endif
