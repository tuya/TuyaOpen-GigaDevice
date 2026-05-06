/**
 * @file pthread_example.c
 * @brief Example usage of pthread wrapper for FreeRTOS
 * @date 2026-01-28
 */

#include "pthread.h"
#include <stdio.h>

/* Example 1: Simple thread creation and join */
void *simple_thread(void *arg)
{
    int count = *(int *)arg;

    printf("Thread started with count: %d\n", count);

    for (int i = 0; i < count; i++) {
        printf("Thread iteration: %d\n", i);
        /* Simulate some work */
    }

    printf("Thread finished\n");
    return (void *)(long)count;
}

void example_simple_thread(void)
{
    pthread_t thread;
    int count = 5;
    void *retval;

    printf("=== Simple Thread Example ===\n");

    /* Create thread */
    if (pthread_create("simple", &thread, NULL, simple_thread, &count) != 0) {
        printf("Failed to create thread\n");
        return;
    }

    printf("Main: waiting for thread to finish...\n");

    /* Wait for thread to complete */
    pthread_join(thread, &retval);

    printf("Thread returned: %ld\n", (long)retval);
}

/* Example 2: Thread with attributes */
void *attributed_thread(void *arg)
{
    printf("Attributed thread running\n");
    return NULL;
}

void example_thread_attributes(void)
{
    pthread_t thread;
    pthread_attr_t attr;

    printf("\n=== Thread Attributes Example ===\n");

    /* Initialize attributes */
    pthread_attr_init(&attr);

    /* Set custom stack size */
    pthread_attr_setstacksize(&attr, 4096);

    /* Set detached state */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* Create thread with attributes */
    if (pthread_create("attributed", &thread, &attr, attributed_thread, NULL) != 0) {
        printf("Failed to create thread\n");
        pthread_attr_destroy(&attr);
        return;
    }

    /* Clean up attributes */
    pthread_attr_destroy(&attr);

    printf("Detached thread created\n");
}

/* Example 3: Mutex usage */
pthread_mutex_t counter_mutex = NULL;
int shared_counter = 0;

void *increment_thread(void *arg)
{
    int iterations = *(int *)arg;

    for (int i = 0; i < iterations; i++) {
        pthread_mutex_lock(&counter_mutex);
        shared_counter++;
        pthread_mutex_unlock(&counter_mutex);
    }

    return NULL;
}
typedef struct {
    void * task_handle;
    void *(*start_routine)(void *);
    void *arg;
    void *retval;
    uint8_t detached;
    uint8_t joined;
    void *  join_sem;
} pthread_control_block_t;

void example_mutex(void)
{
    pthread_t threads[3];
    int iterations = 1000;
    char name[32];
    pthread_control_block_t *pcb;

    printf("\n=== Mutex Example ===\n");

    /* Initialize mutex */
    pthread_mutex_init(&counter_mutex, NULL);

    shared_counter = 0;

    printf("sizeof(pthread_control_block_t) = %d\n", sizeof(pthread_control_block_t));
    /* Create multiple threads */
    for (int i = 0; i < 3; i++) {
        sprintf(name, "incrementer_%d", i+1);
        pthread_create(name, &threads[i], NULL, increment_thread, &iterations);
        pcb = (pthread_control_block_t *)threads[i];
        printf("Created thread %s with PCB %p, detached: %d, join_sema %p\n", name, pcb, pcb->detached, pcb->join_sem);
    }

    /* Wait for all threads */
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Final counter value: %d (expected: %d)\n", shared_counter, iterations * 3);

    /* Destroy mutex */
    pthread_mutex_destroy(&counter_mutex);
}

/* Example 4: Recursive mutex */
pthread_mutex_t recursive_mutex = NULL;

void recursive_function(int depth)
{
    if (depth <= 0) return;

    pthread_mutex_lock(&recursive_mutex);
    printf("Locked at depth: %d\n", depth);

    recursive_function(depth - 1);

    printf("Unlocked at depth: %d\n", depth);
    pthread_mutex_unlock(&recursive_mutex);
}

void example_recursive_mutex(void)
{
    pthread_mutexattr_t attr;

    printf("\n=== Recursive Mutex Example ===\n");

    /* Create recursive mutex */
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&recursive_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    /* Test recursive locking */
    recursive_function(3);

    /* Destroy mutex */
    pthread_mutex_destroy(&recursive_mutex);
}

/* Example 5: Condition variable */
pthread_mutex_t cond_mutex = NULL;
pthread_cond_t cond_var = NULL;
int data_ready = 0;

void *producer_thread(void *arg)
{
    printf("Producer: preparing data...\n");

    /* Simulate work */
    for (volatile int i = 0; i < 1000000; i++);

    pthread_mutex_lock(&cond_mutex);
    data_ready = 1;
    printf("Producer: data ready, signaling...\n");
    pthread_cond_signal(&cond_var);
    pthread_mutex_unlock(&cond_mutex);

    return NULL;
}

void *consumer_thread(void *arg)
{
    pthread_mutex_lock(&cond_mutex);

    printf("Consumer: waiting for data...\n");
    while (!data_ready) {
        pthread_cond_wait(&cond_var, &cond_mutex);
    }

    printf("Consumer: data received!\n");
    pthread_mutex_unlock(&cond_mutex);

    return NULL;
}

void example_condition_variable(void)
{
    pthread_t producer, consumer;

    printf("\n=== Condition Variable Example ===\n");

    /* Initialize mutex and condition variable */
    pthread_mutex_init(&cond_mutex, NULL);
    pthread_cond_init(&cond_var, NULL);
    data_ready = 0;

    /* Create threads */
    pthread_create("consumer", &consumer, NULL, consumer_thread, NULL);
    pthread_create("producer", &producer, NULL, producer_thread, NULL);

    /* Wait for completion */
    pthread_join(consumer, NULL);
    pthread_join(producer, NULL);

    /* Cleanup */
    pthread_cond_destroy(&cond_var);
    pthread_mutex_destroy(&cond_mutex);
}

/* Example 6: Multiple consumers with broadcast */
pthread_mutex_t broadcast_mutex = NULL;
pthread_cond_t broadcast_cond = NULL;
int broadcast_ready = 0;

void *broadcast_consumer(void *arg)
{
    int id = *(int *)arg;

    pthread_mutex_lock(&broadcast_mutex);
    printf("Consumer %d: waiting...\n", id);

    while (!broadcast_ready) {
        pthread_cond_wait(&broadcast_cond, &broadcast_mutex);
    }

    printf("Consumer %d: received broadcast!\n", id);
    pthread_mutex_unlock(&broadcast_mutex);

    return NULL;
}

void example_broadcast(void)
{
    pthread_t consumers[5];
    int ids[5] = {1, 2, 3, 4, 5};
    char name[32];

    printf("\n=== Broadcast Example ===\n");

    /* Initialize */
    pthread_mutex_init(&broadcast_mutex, NULL);
    pthread_cond_init(&broadcast_cond, NULL);
    broadcast_ready = 0;

    /* Create consumer threads */
    for (int i = 0; i < 5; i++) {
        snprintf(name, sizeof(name), "broadcast_%d", i+1);
        pthread_create(name, &consumers[i], NULL, broadcast_consumer, &ids[i]);
    }

    /* Give threads time to start waiting */
    for (volatile int i = 0; i < 10000000; i++);

    /* Broadcast to all waiting threads */
    pthread_mutex_lock(&broadcast_mutex);
    broadcast_ready = 1;
    printf("Broadcasting to all consumers...\n");
    pthread_cond_broadcast(&broadcast_cond);
    pthread_mutex_unlock(&broadcast_mutex);

    /* Wait for all consumers */
    for (int i = 0; i < 5; i++) {
        pthread_join(consumers[i], NULL);
    }

    /* Cleanup */
    pthread_cond_destroy(&broadcast_cond);
    pthread_mutex_destroy(&broadcast_mutex);
}

/* Example 7: Semaphore usage */
sem_t sem_example = NULL;
int semaphore_value = 0;

void *semaphore_producer(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < 3; i++) {
        /* Simulate producing work */
        for (volatile int j = 0; j < 1000000; j++);

        printf("Producer %d: posting semaphore (iteration %d)\n", id, i);
        sem_post(&sem_example);
    }

    return NULL;
}

void *semaphore_consumer(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < 2; i++) {
        printf("Consumer %d: waiting on semaphore...\n", id);
        sem_wait(&sem_example);
        printf("Consumer %d: acquired semaphore (iteration %d)\n", id, i);

        /* Simulate consuming work */
        for (volatile int j = 0; j < 500000; j++);
    }

    return NULL;
}

void example_semaphore(void)
{
    pthread_t producers[2], consumers[3];
    int producer_ids[2] = {1, 2};
    int consumer_ids[3] = {1, 2, 3};
    char name[32];

    printf("\n=== Semaphore Example ===\n");

    /* Initialize semaphore with initial value 0 */
    if (sem_init(&sem_example, 0, 0) != 0) {
        printf("Failed to initialize semaphore\n");
        return;
    }

    /* Create producer threads */
    for (int i = 0; i < 2; i++) {
        snprintf(name, sizeof(name), "semaphore_producer_%d", i+1);
        pthread_create(name, &producers[i], NULL, semaphore_producer, &producer_ids[i]);
    }

    /* Create consumer threads */
    for (int i = 0; i < 3; i++) {
        snprintf(name, sizeof(name), "semaphore_consumer_%d", i+1);
        pthread_create(name, &consumers[i], NULL, semaphore_consumer, &consumer_ids[i]);
    }

    /* Wait for all threads */
    for (int i = 0; i < 2; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < 3; i++) {
        pthread_join(consumers[i], NULL);
    }

    /* Get final semaphore value */
    int sval;
    sem_getvalue(&sem_example, &sval);
    printf("Final semaphore value: %d\n", sval);

    /* Cleanup */
    sem_destroy(&sem_example);
}

/* Example 8: Semaphore with trywait */
sem_t trywait_sem = NULL;

void example_semaphore_trywait(void)
{
    printf("\n=== Semaphore Trywait Example ===\n");

    /* Initialize semaphore with value 1 */
    sem_init(&trywait_sem, 0, 1);

    /* First trywait should succeed */
    if (sem_trywait(&trywait_sem) == 0) {
        printf("First sem_trywait: SUCCESS\n");
    } else {
        printf("First sem_trywait: FAILED\n");
    }

    /* Second trywait should fail (semaphore is 0) */
    if (sem_trywait(&trywait_sem) == 0) {
        printf("Second sem_trywait: SUCCESS (unexpected)\n");
    } else {
        printf("Second sem_trywait: FAILED (as expected)\n");
    }

    /* Post to make it available again */
    sem_post(&trywait_sem);

    /* Third trywait should succeed */
    if (sem_trywait(&trywait_sem) == 0) {
        printf("Third sem_trywait: SUCCESS\n");
    } else {
        printf("Third sem_trywait: FAILED\n");
    }

    /* Cleanup */
    sem_destroy(&trywait_sem);
}

/* Example 9: Read-Write Lock */
pthread_rwlock_t rwlock = NULL;
int shared_data = 0;

void *reader_thread(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < 3; i++) {
        pthread_rwlock_rdlock(&rwlock);
        printf("Reader %d: reading data = %d\n", id, shared_data);

        /* Simulate reading */
        for (volatile int j = 0; j < 500000; j++);

        pthread_rwlock_unlock(&rwlock);

        /* Sleep between reads */
        for (volatile int j = 0; j < 200000; j++);
    }

    return NULL;
}

void *writer_thread(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < 2; i++) {
        //printf("@@[%d]-%d: task %p\n", id, i, xTaskGetCurrentTaskHandle());
        pthread_rwlock_wrlock(&rwlock);
        shared_data++;
        printf("Writer %d: wrote data = %d\n", id, shared_data);

        /* Simulate writing */
        for (volatile int j = 0; j < 1000000; j++);

        pthread_rwlock_unlock(&rwlock);

        /* Sleep between writes */
        for (volatile int j = 0; j < 500000; j++);
    }

    return NULL;
}

void example_rwlock(void)
{
    pthread_t readers[4], writers[2];
    int reader_ids[4] = {1, 2, 3, 4};
    int writer_ids[2] = {1, 2};
    char name[32];

    printf("\n=== Read-Write Lock Example ===\n");

    /* Initialize rwlock */
    printf("Initializing rwlock (current value: %p)...\n", rwlock);
    if (pthread_rwlock_init(&rwlock, NULL) != 0) {
        printf("Failed to initialize rwlock\n");
        return;
    }
    printf("rwlock initialized successfully (new value: %p)\n", rwlock);

    shared_data = 0;

    /* Create reader threads */
    for (int i = 0; i < 4; i++) {
        sprintf(name, "reader_%d", i+1);
        pthread_create(name, &readers[i], NULL, reader_thread, &reader_ids[i]);
    }

    /* Create writer threads */
    for (int i = 0; i < 2; i++) {
        sprintf(name, "writer_%d", i+1);
        pthread_create(name, &writers[i], NULL, writer_thread, &writer_ids[i]);
    }

    /* Wait for all threads */
    for (int i = 0; i < 4; i++) {
        pthread_join(readers[i], NULL);
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(writers[i], NULL);
    }

    printf("Final shared_data value: %d\n", shared_data);

    /* Cleanup */
    pthread_rwlock_destroy(&rwlock);
}

/* Example 10: RWLock trylock */
pthread_rwlock_t trylock_rwlock = NULL;

void example_rwlock_trylock(void)
{
    printf("\n=== Read-Write Lock Trylock Example ===\n");

    /* Initialize rwlock */
    pthread_rwlock_init(&trylock_rwlock, NULL);

    /* Try read lock - should succeed */
    if (pthread_rwlock_tryrdlock(&trylock_rwlock) == 0) {
        printf("First tryrdlock: SUCCESS\n");

        /* Try another read lock - should succeed (multiple readers allowed) */
        if (pthread_rwlock_tryrdlock(&trylock_rwlock) == 0) {
            printf("Second tryrdlock: SUCCESS (multiple readers)\n");
            pthread_rwlock_unlock(&trylock_rwlock);
        }

        /* Try write lock while holding read lock - should fail */
        if (pthread_rwlock_trywrlock(&trylock_rwlock) == 0) {
            printf("trywrlock with read lock: SUCCESS (unexpected)\n");
            pthread_rwlock_unlock(&trylock_rwlock);
        } else {
            printf("trywrlock with read lock: FAILED (as expected)\n");
        }

        pthread_rwlock_unlock(&trylock_rwlock);
    }

    /* Now try write lock - should succeed */
    if (pthread_rwlock_trywrlock(&trylock_rwlock) == 0) {
        printf("trywrlock: SUCCESS\n");

        /* Try read lock while holding write lock - should fail */
        if (pthread_rwlock_tryrdlock(&trylock_rwlock) == 0) {
            printf("tryrdlock with write lock: SUCCESS (unexpected)\n");
            pthread_rwlock_unlock(&trylock_rwlock);
        } else {
            printf("tryrdlock with write lock: FAILED (as expected)\n");
        }

        pthread_rwlock_unlock(&trylock_rwlock);
    }

    /* Cleanup */
    pthread_rwlock_destroy(&trylock_rwlock);
}

/* Example 11: Producer-Consumer with Semaphore */
#define BUFFER_SIZE 5
sem_t empty_slots = NULL;
sem_t full_slots = NULL;
pthread_mutex_t buffer_mutex = NULL;
int buffer[BUFFER_SIZE];
int in = 0;
int out = 0;

void *bounded_producer(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < 10; i++) {
        int item = id * 100 + i;

        /* Wait for empty slot */
        sem_wait(&empty_slots);

        /* Lock buffer */
        pthread_mutex_lock(&buffer_mutex);
        buffer[in] = item;
        printf("Producer %d: produced item %d at position %d\n", id, item, in);
        in = (in + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&buffer_mutex);

        /* Signal item available */
        sem_post(&full_slots);

        /* Simulate production time */
        for (volatile int j = 0; j < 500000; j++);
    }

    return NULL;
}

void *bounded_consumer(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < 10; i++) {
        /* Wait for item */
        sem_wait(&full_slots);

        /* Lock buffer */
        pthread_mutex_lock(&buffer_mutex);
        int item = buffer[out];
        printf("Consumer %d: consumed item %d from position %d\n", id, item, out);
        out = (out + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&buffer_mutex);

        /* Signal slot available */
        sem_post(&empty_slots);

        /* Simulate consumption time */
        for (volatile int j = 0; j < 800000; j++);
    }

    return NULL;
}

void example_producer_consumer(void)
{
    pthread_t producers[2], consumers[2];
    int producer_ids[2] = {1, 2};
    int consumer_ids[2] = {1, 2};
    char name[32];

    printf("\n=== Producer-Consumer (Bounded Buffer) Example ===\n");

    /* Initialize semaphores */
    sem_init(&empty_slots, 0, BUFFER_SIZE);  /* Initially all slots empty */
    sem_init(&full_slots, 0, 0);             /* Initially no items */
    pthread_mutex_init(&buffer_mutex, NULL);

    in = 0;
    out = 0;

    /* Create threads */
    for (int i = 0; i < 2; i++) {
        sprintf(name, "producer_%d", i+1);
        pthread_create(name, &producers[i], NULL, bounded_producer, &producer_ids[i]);
        sprintf(name, "consumer_%d", i+1);
        pthread_create(name, &consumers[i], NULL, bounded_consumer, &consumer_ids[i]);
    }

    /* Wait for completion */
    for (int i = 0; i < 2; i++) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    printf("Producer-Consumer example completed\n");

    /* Cleanup */
    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    pthread_mutex_destroy(&buffer_mutex);
}

void free_mem(void)
{
    int total, used, free, max_used;
    sys_heap_info(&total, &free, &max_used);
    printf("HEAP: total=%d free=%d max_used=%d\n", total, free, max_used);

    sys_task_list(NULL);
}

/* Main function to run all examples */
void pthread_examples_main(void)
{
    printf("==============================================\n");
    printf("    pthread Wrapper for FreeRTOS Examples    \n");
    printf("==============================================\n\n");

    //without memory leaking
#if 0
    example_simple_thread();
    example_thread_attributes();
#endif



    example_mutex();
#if 1
    example_recursive_mutex();
#endif
#if 1
    example_condition_variable();

    example_broadcast();
#endif
#if 1
    example_semaphore();

    example_semaphore_trywait();

    //free_mem();
    example_rwlock();

    //free_mem();
    example_rwlock_trylock();

    free_mem();


    example_producer_consumer();
#endif
    printf("\n==============================================\n");
    printf("           All examples completed!           \n");
    printf("==============================================\n");
}
