#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <alloca.h>

#include "mir_public_int.h"

#define CHECK_RESULT 1

#define TEST_NOT_PERFORMED 2
#define TEST_NOT_APPLICABLE 2
#define TEST_UNSUCCESSFUL 1
#define TEST_SUCCESSFUL 0

static int solutions[] = 
{/*{{{*/
    1,
    0,
    0,
    2,
    10, /* 5 */
    4,
    40,
    92,
    352,
    724, /* 10 */
    2680,
    14200,
    73712,
    365596,
};/*}}}*/
#define MAX_SOLUTIONS sizeof(solutions)/sizeof(int)

int total_count;
int cutoff_value = 3;

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

int ok(int n, char *a)
{/*{{{*/
/*
 * <a> contains array of <n> queen positions.  Returns 1
 * if none of the queens conflict, and returns 0 otherwise.
 */
    int i, j;
    char p, q;

    for (i = 0; i < n; i++) 
    {
        p = a[i];

        for (j = i + 1; j < n; j++) 
        {
            q = a[j];
            if (q == p || q == p - (j - i) || q == p + (j - i))
                return 0;
        }
    }
    return 1;
}/*}}}*/

void nqueens_ser (int n, int j, char *a, int *solutions)
{/*{{{*/
    int res;
    int i;

    if (n == j) 
    {
        /* good solution, count it */
        *solutions = 1;
        return;
    }

    *solutions = 0;

    /* try each possible position for queen <j> */
    for (i = 0; i < n; i++) 
    {
        /* allocate a temporary array and copy <a> into it */
        a[j] = (char) i;
        if (ok(j + 1, a)) 
        {
            nqueens_ser(n, j + 1, a,&res);
            *solutions += res;
        }
    }
}/*}}}*/

struct  nanos_args_0_t
{/*{{{*/
  int n;
  int j;
  char *a;
  int depth;
  int *csols;
  int i;
};/*}}}*/

void nqueens(int n, int j, char *a, int *solutions, int depth);
static void smp_ol_nqueens_0_unpacked(int *const n, int *const j, char **const a, int *const depth, int **const csols, int *const i)
{/*{{{*/
  {
    {
      char *b = alloca((*n) * sizeof(char));
      memcpy(b, (*a), (*j) * sizeof(char));
      b[(*j)] = (char)(*i);
      if (ok((*j) + 1, b))
        nqueens((*n), (*j) + 1, b, &(*csols)[(*i)], (*depth) + 1);
    }
  }
}/*}}}*/

static void smp_ol_nqueens_0(struct nanos_args_0_t *const args)
{/*{{{*/
  {
    smp_ol_nqueens_0_unpacked(&((*args).n), &((*args).j), &((*args).a), &((*args).depth), &((*args).csols), &((*args).i));
  }
}/*}}}*/

void nqueens(int n, int j, char *a, int *solutions, int depth)
{/*{{{*/
    int *csols;
    int i;

    if (n == j) 
    {
        /* good solution, count it */
        *solutions = 1;
        return;
    }

    *solutions = 0;
    csols = alloca(n*sizeof(int));
    memset(csols,0,n*sizeof(int));

    // The task wait counter
    struct mir_twc_t* twc = mir_twc_create();

    /* try each possible position for queen <j> */
    for (i = 0; i < n; i++) 
    {
        if ( depth < cutoff_value ) 
        {
                // Create task0
                struct nanos_args_0_t imm_args;
                imm_args.n = n;
                imm_args.j = j;
                imm_args.a = a;
                imm_args.depth = depth;
                imm_args.csols = csols;
                imm_args.i = i;

                struct mir_task_t* task_0 = mir_task_create((mir_tfunc_t) smp_ol_nqueens_0, (void*) &imm_args, sizeof(struct nanos_args_0_t), twc, 0, NULL, NULL);
        } 
        else 
        {
            a[j] = (char) i;
            if (ok(j + 1, a))
                nqueens_ser(n, j + 1, a,&csols[i]);
        }
    }

    // Wait for tasks to finish
    mir_twc_wait(twc);

    for ( i = 0; i < n; i++) 
        *solutions += csols[i];
}/*}}}*/

void find_queens (int size)
{/*{{{*/
    total_count=0;
    char *a;
    a = alloca(size * sizeof(char));
    nqueens(size, 0, a, &total_count,0);
}/*}}}*/

int verify_queens (int size)
{/*{{{*/
    if ( size > MAX_SOLUTIONS ) return TEST_NOT_APPLICABLE;
    if ( total_count == solutions[size-1]) return TEST_SUCCESSFUL;
    return TEST_UNSUCCESSFUL;
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc > 2)
    {
        printf("Usage: nqueens board-size\n");
        exit(0);
    }

    // Init the runtime
    mir_create();


    int size = 14;

    if(argv[1])
        size = atoi(argv[1]);
    printf("Computing nqueens %d ... \n", size);

    long par_time_start = get_usecs();
    find_queens(size);
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
    if (CHECK_RESULT)
    {
        check = verify_queens(size);
    }

    printf("%s(%d,%d),check=%d in [SUCCESSFUL, UNSUCCESSFUL, NOT_APPLICABLE, NOT_PERFORMED],time=%f secs\n", argv[0], size, cutoff_value, check, par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/