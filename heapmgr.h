/*--------------------------------------------------------------------*/
/* heapmgr.h                                                          */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#ifndef HEAPMGR_INCLUDED
#define HEAPMGR_INCLUDED

#include <stddef.h>

/*--------------------------------------------------------------------*/

/* Allocate and return the address of a region of memory that is large
   enough to hold an object whose size (as measured by the sizeof
   operator) is uBytes bytes. The region is guaranteed to be properly
   aligned for data of any type. Return NULL if uBytes is 0 or the
   request cannot be satisfied. The region is uninitialized. */

void *HeapMgr_malloc(size_t uBytes);

/*--------------------------------------------------------------------*/

/* Deallocate the region of memory pointed to by pv. pv should point
   to a region that was allocated by HeapMgr_malloc(). Do nothing if
   pv is NULL. */

void HeapMgr_free(void *pv);

#endif
