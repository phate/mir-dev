#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <math.h>

#include "mir_public_int.h"
#include "helper.h"

#define OPR_SCALE (42)
#define SLEEP_MS 0
#define LOOP_CNT 10
//#define ENABLE_FAULT_IN 

uint64_t** buffer = NULL;
int num_tasks = (2<<12);
int buf_sz = (2<<15);

size_t g_sum = 0;

long get_usecs(void)
{/*{{{*/
    struct timeval t;
    gettimeofday(&t, ((void *) 0));
    return t.tv_sec * 1000000 + t.tv_usec;
}/*}}}*/

void map_init()
{/*{{{*/
    PMSG("Init ... \n");
    PDBG("Allocating memory ... \n");
    PMSG("Memory required = %lu MB\n", (buf_sz*sizeof(uint64_t)*num_tasks)/(1024*1024));
    buffer = malloc (sizeof(uint64_t*) * num_tasks);
    if(!buffer)
        PABRT("Cannot allocate memory!\n");
    for(uint64_t j=0; j<num_tasks; j++)
    {
        buffer[j] = mir_mem_pol_allocate(sizeof(uint64_t) * buf_sz);
        if(!buffer[j])
            PABRT("Cannot allocate memory!\n");
    }

#ifdef ENABLE_FAULT_IN
    PDBG("Initializing memory ... \n");
    for(uint64_t i=0; i<num_tasks; i++)
        for(uint64_t j=0; j<buf_sz; j++)
            buffer[i][j] = i+1;
#endif
}/*}}}*/

void map(uint64_t* in, uint64_t* out)
{/*{{{*/
    size_t sum = 0;
    for(uint64_t j=0; j<LOOP_CNT; j++)
        for(uint64_t i=0; i<buf_sz; i++)
        {
            out[i] = OPR_SCALE * in[i];
            sum += (int)sqrt((double)(out[i])); 
        }

    __sync_fetch_and_add(&g_sum, sum);
    mir_sleep_ms(SLEEP_MS);
}/*}}}*/

struct map_wrapper_arg_t 
{/*{{{*/
    uint64_t* in;
    uint64_t* out;
};/*}}}*/

void map_wrapper(void* arg)
{/*{{{*/
    struct map_wrapper_arg_t* warg = (struct map_wrapper_arg_t*)(arg);
    map(warg->in, warg->out);
}/*}}}*/

void for_task(uint64_t start, uint64_t end/*, struct mir_twc_t* twc*/)
{/*{{{*/
    struct mir_twc_t* twc = mir_twc_create();
    for(uint64_t j=start; j<=end; j++)
    {/*{{{*/
        // Create task
        {
            // Data env
            struct map_wrapper_arg_t arg;
            arg.in = buffer[j];
            arg.out = buffer[j];
            PDBG("Task %lu reads buf %p and writes buf %p\n", j, buffer[j], buffer[j]);

            // Data footprint
            struct mir_data_footprint_t footprint;
            footprint.base = (void*) buffer[j];
            footprint.start = 0;
            footprint.end = buf_sz - 1;
            footprint.row_sz = 1;
            footprint.type = sizeof(uint64_t);
            footprint.data_access = MIR_DATA_ACCESS_READ;
            footprint.part_of = NULL;

            struct mir_task_t* task = mir_task_create((mir_tfunc_t) map_wrapper, &arg, sizeof(struct map_wrapper_arg_t), twc, 1, &footprint, NULL);
        }
    }/*}}}*/
    mir_twc_wait(twc);
}/*}}}*/

struct for_task_wrapper_arg_t
{/*{{{*/
    uint64_t start;
    uint64_t end;
    /*struct mir_twc_t* twc;*/
};/*}}}*/

void for_task_wrapper(void* arg)
{/*{{{*/
    struct for_task_wrapper_arg_t* warg = (struct for_task_wrapper_arg_t*) arg;
    for_task(warg->start, warg->end/*, warg->twc*/);
}/*}}}*/

void map_par()
{/*{{{*/
    PMSG("Parallel exec ... \n");

    struct mir_twc_t* twc = mir_twc_create();

    // Split the task creation load among workers
    // Same action as worksharing omp for
    uint32_t num_workers = mir_get_num_threads();
    uint64_t num_iter = num_tasks / num_workers;
    uint64_t num_tail_iter = num_tasks % num_workers;
    if(num_iter > 0)
    {
        // Create prologue tasks
        for(uint32_t k=0; k<num_workers; k++)
        {
            uint64_t start = k * num_iter;
            uint64_t end = start + num_iter - 1;
            {
                struct for_task_wrapper_arg_t arg;
                arg.start = start;
                arg.end = end;
                /*arg.twc = twc;*/

                struct mir_task_t* task = mir_task_create((mir_tfunc_t) for_task_wrapper, &arg, sizeof(struct for_task_wrapper_arg_t), twc, 0, NULL, NULL);
            }
        }
    }
    if(num_tail_iter > 0)
    {
        // Create epilogue task
        uint64_t start = num_workers * num_iter;
        uint64_t end = start + num_tail_iter - 1;
        {
            struct for_task_wrapper_arg_t arg;
            arg.start = start;
            arg.end = end;
            /*arg.twc = twc;*/

            struct mir_task_t* task = mir_task_create((mir_tfunc_t) for_task_wrapper, &arg, sizeof(struct for_task_wrapper_arg_t), twc, 0, NULL, NULL);
        }
    }

    mir_twc_wait(twc);
    PDBG(" ... done! \n");
}/*}}}*/

int map_check()
{/*{{{*/
    PMSG("Check ... \n");
    return TEST_NOT_PERFORMED;
}/*}}}*/

void map_deinit()
{/*{{{*/
    PMSG("Deinit ... \n");
    for(uint64_t j=0; j<num_tasks; j++)
        //free(buffer[j]/*, sizeof(uint64_t) * buf_sz*/);
        mir_mem_pol_release(buffer[j], sizeof(uint64_t) * buf_sz);
    free(buffer/*, sizeof(uint64_t*) * num_tasks*/);
}/*}}}*/

int main(int argc, char *argv[])
{/*{{{*/
    if (argc > 3)
        PABRT("Usage: %s num_tasks buf_size_K\n", argv[0]);

    // Init the runtime
    mir_create();

    if(argc == 2)
        num_tasks = atoi(argv[1]);

    if(argc == 3)
    {
        num_tasks = atoi(argv[1]);
        buf_sz = atoi(argv[2]) * 1024;
    }

    map_init();

    long par_time_start = get_usecs();
    map_par();
    long par_time_end = get_usecs();
    double par_time = (double)( par_time_end - par_time_start) / 1000000;

    int check = TEST_NOT_PERFORMED;
#ifdef CHECK_RESULT 
    check = map_check();
#endif

    map_deinit();

    PMSG("Sum = %lu\n", g_sum);
    PMSG("%s(%d,%d),check=%d in %s,time=%f secs\n", argv[0], num_tasks, buf_sz, check, TEST_ENUM_STRING, par_time);
    PALWAYS("%fs\n", par_time);

    // Pull down the runtime
    mir_destroy();

    return 0;
}/*}}}*/