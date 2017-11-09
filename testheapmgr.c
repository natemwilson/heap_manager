/*--------------------------------------------------------------------*/
/* testheapmgr.c                                                      */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#include "heapmgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#ifndef S_SPLINT_S
#include <sys/resource.h>
#endif

#define __USE_XOPEN_EXTENDED
#include <unistd.h>

/* In lieu of a boolean data type. */
enum {FALSE, TRUE};

/*--------------------------------------------------------------------*/

/* These arrays are too big for the stack section, so store
   them in the bss section. */

/* The maximum allowable number of calls of HeapMgr_malloc(). */
enum {MAX_CALLS = 1000000};

/* Memory chunks allocated by HeapMgr_malloc(). */
static char *apcChunks[MAX_CALLS];

/* Randomly generated chunk sizes.  */
static int aiSizes[MAX_CALLS];

/*--------------------------------------------------------------------*/

/* Function declarations. */

/* Get command-line arguments *piTestNum, *piCount, and *piSize,
   from argument vector argv. argc is the number of used elements
   in argv. Exit if any of the arguments is invalid.  */
static void getArgs(int argc, char *argv[],
   int *piTestNum, int *piCount, int *piSize);

/* Set the process's "CPU time" resource limit. After the CPU
   time limit expires, the OS will send a SIGKILL signal to the
   process. */
static void setCpuTimeLimit(void);

/* Allocate and free iCount memory chunks, each of size iSize, in
   last-in-first-out order. */
static void testLifoFixed(int iCount, int iSize);

/* Allocate and free iCount memory chunks, each of size iSize, in
   first-in-first-out order. */
static void testFifoFixed(int iCount, int iSize);

/* Allocate and free iCount memory chunks, each of some random size
   less than iSize, in last-in-first-out order. */
static void testLifoRandom(int iCount, int iSize);

/* Allocate and free iCount memory chunks, each of some random size
   less than iSize, in first-in-first-out order. */
static void testFifoRandom(int iCount, int iSize);

/* Allocate and free iCount memory chunks, each of size iSize, in
   a random order. */
static void testRandomFixed(int iCount, int iSize);

/* Allocate and free iCount memory chunks, each of some random size
   less than iSize, in a random order. */
static void testRandomRandom(int iCount, int iSize);

/* Allocate and free iCount memory chunks, each of some size less
   than iSize, in the worst possible order for a HeapMgr that is
   implemented using a single linked list. */
static void testWorst(int iCount, int iSize);

/*--------------------------------------------------------------------*/

/* apcTestName is an array containing the names of the tests. */

static char *apcTestName[] =
{
   "LifoFixed", "FifoFixed", "LifoRandom", "FifoRandom",
   "RandomFixed", "RandomRandom", "Worst"
};

/*--------------------------------------------------------------------*/


/* An object of type TestFunction is a pointer to a function that
   accepts two ints and returns nothing. */

typedef void (*TestFunction)(int, int);

/* apfTestFunction is an array containing pointers to the test
   functions. Each pointer corresponds, by position, to a test name
   in apcTestName. */

static TestFunction apfTestFunction[] =
{
   testLifoFixed, testFifoFixed, testLifoRandom, testFifoRandom,
   testRandomFixed, testRandomRandom, testWorst
};

/*--------------------------------------------------------------------*/

/* Test the HeapMgr_malloc() and HeapMgr_free() functions.

   As always, argc is the command-line argument count.

   argv[1] indicates which test to run:
      LifoFixed: LIFO with fixed size chunks,
      FifoFixed: FIFO with fixed size chunks,
      LifoRandom: LIFO with random size chunks,
      FifoRandom: FIFO with random size chunks,
      RandomFixed: random order with fixed size chunks,
      RandomRandom: random order with random size chunks,
      Worst: worst case for single linked list implementation.

   argv[2] is the number of calls of HeapMgr_malloc() and HeapMgr_free()
   to execute. argv[2] cannot be greater than MAX_CALLS.

   argv[3] is the (maximum) size of each memory chunk.

   If the NDEBUG macro is not defined, then initialize and check
   the contents of each memory chunk.

   At the end of the process, write the heap memory and CPU time
   consumed to stdout, and return 0. */

int main(int argc, char *argv[])
{
   int iTestNum = 0;
   int iCount = 0;
   int iSize = 0;
   clock_t iInitialClock;
   clock_t iFinalClock;
   char *pcInitialBreak;
   char *pcFinalBreak;
   unsigned int uiMemoryConsumed;
   double dTimeConsumed;

   /* Get the command-line arguments. */
   getArgs(argc, argv, &iTestNum, &iCount, &iSize);

   /* Start printing the results. */
   printf("%16s %12s %7d %6d ", argv[0], argv[1], iCount, iSize);
   fflush(stdout);

   /* Save the initial clock and program break. */
   iInitialClock = clock();
   pcInitialBreak = sbrk(0);

   /* Set the process's CPU time limit. */
   setCpuTimeLimit();

   /* Call the specified test function. */
   (*(apfTestFunction[iTestNum]))(iCount, iSize);

   /* Save the final clock and program break. */
   pcFinalBreak = sbrk(0);
   iFinalClock = clock();

   /* Use the initial and final clocks and program breaks to compute
      CPU time and heap memory consumed. */
   uiMemoryConsumed = (unsigned int)(pcFinalBreak - pcInitialBreak);
   dTimeConsumed =
      ((double)(iFinalClock - iInitialClock)) / CLOCKS_PER_SEC;

   /* Finish printing the results. */
   printf("%6.2f %10u\n", dTimeConsumed, uiMemoryConsumed);
   return 0;
}

/*--------------------------------------------------------------------*/

/* Get command-line arguments *piTestNum, *piCount, and *piSize,
   from argument vector argv. argc is the number of used elements
   in argv. Exit if any of the arguments is invalid.  */

static void getArgs(int argc, char *argv[],
   int *piTestNum, int *piCount, int *piSize)
{
   int i;
   int iTestCount;

   assert(argv != NULL);
   assert(piTestNum != NULL);
   assert(piCount != NULL);
   assert(piSize != NULL);

   if (argc != 4)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      exit(EXIT_FAILURE);
   }

   /* Get the test number. */
   iTestCount = (int)(sizeof(apcTestName) / sizeof(apcTestName[0]));
   for (i = 0; i < iTestCount; i++)
      if (strcmp(argv[1], apcTestName[i]) == 0)
      {
         *piTestNum = i;
         break;
      }
   if (i == iTestCount)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Valid testnames:\n");
      for (i = 0; i < iTestCount; i++)
         fprintf(stderr, " %s", apcTestName[i]);
      fprintf(stderr, "\n");
      exit(EXIT_FAILURE);
   }

   /* Get the count. */
   if (sscanf(argv[2], "%d", piCount) != 1)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Count must be numeric\n");
      exit(EXIT_FAILURE);
   }
   if (*piCount <= 0)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Count must be positive\n");
      exit(EXIT_FAILURE);
   }
   if (*piCount > MAX_CALLS)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Count cannot be greater than %d\n", MAX_CALLS);
      exit(EXIT_FAILURE);
   }

   /* Get the size. */
   if (sscanf(argv[3], "%d", piSize) != 1)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Size must be numeric\n");
      exit(EXIT_FAILURE);
   }
   if (*piSize <= 0)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Size must be positive\n");
      exit(EXIT_FAILURE);
   }
}

/*--------------------------------------------------------------------*/

#ifndef S_SPLINT_S
/* Set the process's "CPU time" resource limit. After the CPU
   time limit expires, the OS will send a SIGKILL signal to the
   process. */

static void setCpuTimeLimit(void)
{
   enum {CPU_TIME_LIMIT_IN_SECONDS = 300};
   struct rlimit sRlimit;
   sRlimit.rlim_cur = CPU_TIME_LIMIT_IN_SECONDS;
   sRlimit.rlim_max = CPU_TIME_LIMIT_IN_SECONDS;
   setrlimit(RLIMIT_CPU, &sRlimit);
}
#endif

/*--------------------------------------------------------------------*/

#ifndef NDEBUG

#define ASSURE(i) assure(i, __LINE__)

/* If !iSuccessful, print an error message indicating that the test
   at line iLineNum failed. */

static void assure(int iSuccessful, int iLineNum)
{
   if (! iSuccessful)
      fprintf(stderr, "Test at line %d failed.\n", iLineNum);
}

#endif

/*--------------------------------------------------------------------*/

/* Allocate and free iCount memory chunks, each of size iSize, in
   last-in-first-out order. */

static void testLifoFixed(int iCount, int iSize)
{
   int i;

   /* Call HeapMgr_malloc() repeatedly to fill apcChunks. */
   for (i = 0; i < iCount; i++)
   {
      apcChunks[i] = (char*)HeapMgr_malloc((size_t)iSize);
      if (apcChunks[i] == NULL)
      {
         printf("Malloc returned NULL.\n");
         exit(0);
      }

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of i.
            So later, given i, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < iSize; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
   }

   /* Call HeapMgr_free() repeatedly to free the chunks in
      LIFO order. */
   for (i = iCount - 1; i >= 0; i--)
   {
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < iSize; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif

      HeapMgr_free(apcChunks[i]);
   }
}

/*--------------------------------------------------------------------*/

/* Allocate and free iCount memory chunks, each of size iSize, in
   first-in-first-out order. */

static void testFifoFixed(int iCount, int iSize)
{
   int i;

   /* Call HeapMgr_malloc() repeatedly to fill apcChunks. */
   for (i = 0; i < iCount; i++)
   {
      apcChunks[i] = (char*)HeapMgr_malloc((size_t)iSize);
      if (apcChunks[i] == NULL)
      {
         printf("Malloc returned NULL.\n");
         exit(0);
      }

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of i.
            So later, given i, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < iSize; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
   }

   /* Call HeapMgr_free() repeatedly to free the chunks in
      FIFO order. */
   for (i = 0; i < iCount; i++)
   {
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < iSize; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif

      HeapMgr_free(apcChunks[i]);
   }
}

/*--------------------------------------------------------------------*/

/* Allocate and free iCount memory chunks, each of some random size
   less than iSize, in last-in-first-out order. */

static void testLifoRandom(int iCount, int iSize)
{
   int i;

   /* Fill aiSizes, an array of random integers in the range 1 to
      iSize. */
   for (i = 0; i < iCount; i++)
      aiSizes[i] = (rand() % iSize) + 1;

   /* Call HeapMgr_malloc() repeatedly to fill apcChunks. */
   for (i = 0; i < iCount; i++)
   {
      apcChunks[i] = (char*)HeapMgr_malloc((size_t)aiSizes[i]);
      if (apcChunks[i] == NULL)
      {
         printf("Malloc returned NULL.\n");
         exit(0);
      }

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of i.
            So later, given i, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < aiSizes[i]; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
   }

   /* Call HeapMgr_free() repeatedly to free the chunks in
      LIFO order. */
   for (i = iCount - 1; i >= 0; i--)
   {
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < aiSizes[i]; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif

      HeapMgr_free(apcChunks[i]);
   }
}

/*--------------------------------------------------------------------*/

/* Allocate and free iCount memory chunks, each of some random size
   less than iSize, in first-in-first-out order. */

static void testFifoRandom(int iCount, int iSize)
{
   int i;

   /* Fill aiSizes, an array of random integers in the range 1 to
      iSize. */
   for (i = 0; i < iCount; i++)
      aiSizes[i] = (rand() % iSize) + 1;

   /* Call HeapMgr_malloc() repeatedly to fill apcChunks. */
   for (i = 0; i < iCount; i++)
   {
      apcChunks[i] = (char*)HeapMgr_malloc((size_t)aiSizes[i]);
      if (apcChunks[i] == NULL)
      {
         printf("Malloc returned NULL.\n");
         exit(0);
      }

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of i.
            So later, given i, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < aiSizes[i]; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
   }

   /* Call HeapMgr_free() repeatedly to free the chunks in
      FIFO order. */
   for (i = 0; i < iCount; i++)
   {
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < aiSizes[i]; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif

      HeapMgr_free(apcChunks[i]);
   }
}

/*--------------------------------------------------------------------*/

/* Allocate and free iCount memory chunks, each of size iSize, in
   a random order. */

static void testRandomFixed(int iCount, int iSize)
{
   int i;
   int iRand;
   int iLogicalArraySize;

   iLogicalArraySize = (iCount / 3) + 1;

   i = 0;
   
   /* Call HeapMgr_malloc() and HeapMgr_free() in a randomly
      interleaved manner. */
   while (i < iCount)
   {
      /* Assign some random integer to iRand. */
      iRand = rand() % iLogicalArraySize;
      
      if (apcChunks[iRand] == NULL)
      {
         apcChunks[iRand] = (char*)HeapMgr_malloc((size_t)iSize);
         if (apcChunks[iRand] == NULL)
         {
            printf("Malloc returned NULL.\n");
            exit(0);
         }

         #ifndef NDEBUG
         {
            /* Fill the newly allocated chunk with some character.
               The character is derived from the last digit of iRand.
               So later, given iRand, we can check to make sure that
               the contents haven't been corrupted. */
            int iCol;
            char c = (char)((iRand % 10) + '0');
            for (iCol = 0; iCol < iSize; iCol++)
               apcChunks[iRand][iCol] = c;
         }
         #endif
         
         i++;
      }

      /* Assign some random integer to iRand. */
      iRand = rand() % iLogicalArraySize;

      /* If apcChunks[iRand] contains a chunk, free it and set
         apcChunks[iRand] to NULL. */
      if (apcChunks[iRand] != NULL)
      {
         #ifndef NDEBUG
         {
            /* Check the chunk that is about to be freed to make sure
               that its contents haven't been corrupted. */
            int iCol;
            char c = (char)((iRand % 10) + '0');
            for (iCol = 0; iCol < iSize; iCol++)
               ASSURE(apcChunks[iRand][iCol] == c);
         }
         #endif

         HeapMgr_free(apcChunks[iRand]);
         apcChunks[iRand] = NULL;
      }
   }

   /* Free the rest of the chunks. */
   for (i = 0; i < iLogicalArraySize; i++)
   {
      if (apcChunks[i] != NULL)
      {
         #ifndef NDEBUG
         {
            /* Check the chunk that is about to be freed to make sure
               that its contents haven't been corrupted. */
            int iCol;
            char c = (char)((i % 10) + '0');
            for (iCol = 0; iCol < iSize; iCol++)
               ASSURE(apcChunks[i][iCol] == c);
         }
         #endif

         HeapMgr_free(apcChunks[i]);
         apcChunks[i] = NULL;
      }
   }
}

/*--------------------------------------------------------------------*/

/* Allocate and free iCount memory chunks, each of some random size
   less than iSize, in a random order. */

static void testRandomRandom(int iCount, int iSize)
{
   int i;
   int iRand;
   int iLogicalArraySize;

   iLogicalArraySize = (iCount / 3) + 1;

   /* Fill aiSizes, an array of random integers in the range 1
      to iSize. */
   for (i = 0; i < iLogicalArraySize; i++)
      aiSizes[i] = (rand() % iSize) + 1;

   i = 0;
   
   /* Call HeapMgr_malloc() and HeapMgr_free() in a randomly
      interleaved manner. */
   while (i < iCount)
   {
      /* Assign some random integer to iRand. */
      iRand = rand() % iLogicalArraySize;
      
      if (apcChunks[iRand] == NULL)
      {
         apcChunks[iRand] = (char*)HeapMgr_malloc((size_t)aiSizes[iRand]);
         if (apcChunks[iRand] == NULL)
         {
            printf("Malloc returned NULL.\n");
            exit(0);
         }

         #ifndef NDEBUG
         {
            /* Fill the newly allocated chunk with some character.
               The character is derived from the last digit of iRand.
               So later, given iRand, we can check to make sure that
               the contents haven't been corrupted. */
            int iCol;
            char c = (char)((iRand % 10) + '0');
            for (iCol = 0; iCol < aiSizes[iRand]; iCol++)
               apcChunks[iRand][iCol] = c;
         }
         #endif
         
         i++;
      }

      /* Assign some random integer to iRand. */
      iRand = rand() % iLogicalArraySize;

      /* If apcChunks[iRand] contains a chunk, free it and set
         apcChunks[iRand] to NULL. */
      if (apcChunks[iRand] != NULL)
      {
         #ifndef NDEBUG
         {
            /* Check the chunk that is about to be freed to make sure
               that its contents haven't been corrupted. */
            int iCol;
            char c = (char)((iRand % 10) + '0');
            for (iCol = 0; iCol < aiSizes[iRand]; iCol++)
               ASSURE(apcChunks[iRand][iCol] == c);
         }
         #endif

         HeapMgr_free(apcChunks[iRand]);
         apcChunks[iRand] = NULL;
      }
   }

   /* Free the rest of the chunks. */
   for (i = 0; i < iLogicalArraySize; i++)
   {
      if (apcChunks[i] != NULL)
      {
         #ifndef NDEBUG
         {
            /* Check the chunk that is about to be freed to make sure
               that its contents haven't been corrupted. */
            int iCol;
            char c = (char)((i % 10) + '0');
            for (iCol = 0; iCol < aiSizes[i]; iCol++)
               ASSURE(apcChunks[i][iCol] == c);
         }
         #endif

         HeapMgr_free(apcChunks[i]);
         apcChunks[i] = NULL;
      }
   }
}

/*--------------------------------------------------------------------*/

/* Allocate and free iCount memory chunks, each of some size less
   than iSize, in the worst possible order for a HeapMgr that is
   implemented using a single linked list. */

static void testWorst(int iCount, int iSize)
{
   int i;
   int iChunkSize;

   /* Make sure iCount is even. */
   if (iCount % 2 != 0)
      iCount++;

   /* Fill the array with chunks of increasing size, each separated by
      a small dummy chunk. */
   i = 0;
   while (i < iCount)
   {
      iChunkSize =
         (int)(((double)i * ((double)iSize / (double)iCount)) + 1.0);
      apcChunks[i] = HeapMgr_malloc((size_t)iChunkSize);
      if ((i != 0) && (apcChunks[i] == NULL))
      {
         printf("Malloc returned NULL.\n");
         exit(0);
      }

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of i.
            So later, given i, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < iChunkSize; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
      i++;
      apcChunks[i] = HeapMgr_malloc((size_t)1);
      if (apcChunks[i] == NULL)
      {
         printf("Malloc returned NULL.\n");
         exit(0);
      }
      i++;
   }

   /* Free the non-dummy chunks in reverse order.  Thus a HeapMgr
      implementation that uses a single linked list will be in a
      worst-case state:  the list will contain chunks in increasing
      order by size. */
   i = iCount;
   while (i >= 2)
   {
      i--;
      i--;
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         iChunkSize =
            (int)(((double)i * ((double)iSize / (double)iCount)) + 1.0);
         for (iCol = 0; iCol < iChunkSize; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif
      HeapMgr_free(apcChunks[i]);
   }

   /* Allocate chunks in decreasing order by size, thus maximizing the
      amount of list traversal required. */
   i = iCount;
   while (i >= 2)
   {
      i--;
      i--;
      iChunkSize =
         (int)(((double)i * ((double)iSize / (double)iCount)) + 1.0);
      apcChunks[i] = HeapMgr_malloc((size_t)iChunkSize);
      if (apcChunks[i] == NULL)
      {
         printf("Malloc returned NULL.\n");
         exit(0);
      }
   }

   /* Free all chunks. */
   for (i = 0; i < iCount; i++)
      HeapMgr_free(apcChunks[i]);
}
