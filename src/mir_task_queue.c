#include "mir_defines.h"
#include "mir_lock.h"
#include "mir_memory.h"
#include "mir_task_queue.h"
#include "mir_utils.h"
#include "mir_task.h"
#include "mir_runtime.h"

void* mir_task_queue_create(uint32_t capacity)
{ /*{{{*/
    MIR_ASSERT(capacity > 0);

    struct mir_task_queue_t* queue = mir_cmalloc_int(sizeof(struct mir_task_queue_t));
    MIR_CHECK_MEM(queue != NULL);

    queue->buffer = mir_cmalloc_int(capacity * sizeof(void*));
    MIR_CHECK_MEM(queue->buffer != NULL);

    queue->capacity = capacity;
    queue->size = 0;
    queue->in = 0;
    queue->out = 0;

    mir_lock_create(&(queue->enq_lock));
    mir_lock_create(&(queue->deq_lock));

    return queue;
} /*}}}*/

void mir_task_queue_destroy(struct mir_task_queue_t* queue)
{ /*{{{*/
    MIR_ASSERT(queue != NULL);

    mir_lock_destroy(&(queue->enq_lock));
    mir_lock_destroy(&(queue->deq_lock));
    mir_free_int(queue->buffer, queue->capacity * sizeof(void*));
    queue->buffer = NULL;
    mir_free_int(queue, sizeof(struct mir_task_queue_t));
    queue = NULL;
} /*}}}*/

int mir_task_queue_push(struct mir_task_queue_t* queue, struct mir_task_t* data)
{ /*{{{*/
    MIR_ASSERT(queue != NULL);
    MIR_ASSERT(data != NULL);

    mir_lock_set(&(queue->enq_lock));

    if (TASK_QUEUE_FULL(queue)) {
        TQ_DBG("queue full!", queue);
        mir_lock_unset(&(queue->enq_lock));
        return 0;
    }

    queue->buffer[queue->in] = data;
    queue->in++;
    if (queue->in >= queue->capacity)
        queue->in -= queue->capacity;
    __sync_fetch_and_add(&(queue->size), 1);

    mir_lock_unset(&(queue->enq_lock));

    return 1;
} /*}}}*/

struct mir_task_t* mir_task_queue_pop(struct mir_task_queue_t* queue)
{ /*{{{*/
    MIR_ASSERT(queue != NULL);
    struct mir_task_t* task = NULL;

    mir_lock_set(&(queue->deq_lock));
    if (TASK_QUEUE_EMPTY(queue)) {
        TQ_DBG("queue empty!", queue);
        goto cleanup;
    }
    task = queue->buffer[queue->out];
    MIR_ASSERT(task != NULL);
#ifdef MIR_GPL
    struct mir_worker_t* worker = mir_worker_get_context();
    if (!runtime->single_parallel_block && task->team) {
        // The private queue is FIFO ordered. We have either already
        // executed the parallel block or will execute it right now.
        //
        // If we are stealing from another queue, ensure we have executed
        // our parallel block first.
        if ((void *)queue == worker->private_queue)
            task->team->parallel_block_flag[worker->id] = 1;
        else if (task->team->parallel_block_flag[worker->id] == 0) {
            task = NULL;
            goto cleanup;
	}
    }
#endif

    __sync_fetch_and_sub(&(queue->size), 1);
    queue->out++;
    if (queue->out >= queue->capacity)
        queue->out -= queue->capacity;

cleanup:
    mir_lock_unset(&(queue->deq_lock));

    return task;
} /*}}}*/

uint32_t mir_task_queue_size(const struct mir_task_queue_t* queue)
{ /*{{{*/
    MIR_ASSERT(queue != NULL);

    return queue->size;
} /*}}}*/


