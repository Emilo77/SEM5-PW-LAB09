/* The classic producer-consumer example.
 *
 * Illustrates mutexes and conditions.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "err.h"

#define BUFFER_SIZE 16

/* A producers-consumers queue of integers. */
struct Queue {
    int buffer[BUFFER_SIZE]; // A circular buffer of integers.
    int read_pos, write_pos; // Positions in buffer for reading and for writing (begin and end of queue).
    pthread_mutex_t mutex; // Mutex for exclusive access to buffer.
    pthread_cond_t not_empty; // Signaled when buffer is not empty.
    pthread_cond_t not_full; // Signaled when buffer is not full.
};

/* Initialize a buffer */
void queue_init(struct Queue* b)
{
    ASSERT_ZERO(pthread_mutex_init(&b->mutex, NULL));
    ASSERT_ZERO(pthread_cond_init(&b->not_empty, NULL));
    ASSERT_ZERO(pthread_cond_init(&b->not_full, NULL));
    b->read_pos = 0;
    b->write_pos = 0;
}

/* Destroy the buffer */
void queue_destroy(struct Queue* b)
{
    ASSERT_ZERO(pthread_cond_destroy(&b->not_empty));
    ASSERT_ZERO(pthread_cond_destroy(&b->not_full));
    ASSERT_ZERO(pthread_mutex_destroy(&b->mutex));
}

/* Store an integer in the buffer */
void queue_put(struct Queue* b, int data)
{
    ASSERT_ZERO(pthread_mutex_lock(&b->mutex));

    // Wait until buffer is not full.
    while ((b->write_pos + 1) % BUFFER_SIZE == b->read_pos)
        ASSERT_ZERO(pthread_cond_wait(&b->not_full, &b->mutex));
    // Note pthread_cond_wait reacquired b->lock before returning.

    bool was_empty = (b->read_pos == b->write_pos);

    // Write the data and advance write pointer.
    b->buffer[b->write_pos] = data;
    b->write_pos++;
    if (b->write_pos == BUFFER_SIZE)
        b->write_pos = 0;

    // Signal that the buffer is not empty anymore.
    if (was_empty)
        ASSERT_ZERO(pthread_cond_signal(&b->not_empty));

    ASSERT_ZERO(pthread_mutex_unlock(&b->mutex));
}

/* Read and remove an integer from the buffer */
int queue_get(struct Queue* b)
{
    ASSERT_ZERO(pthread_mutex_lock(&b->mutex));

    // Wait until buffer is not empty.
    while (b->write_pos == b->read_pos)
        ASSERT_ZERO(pthread_cond_wait(&b->not_empty, &b->mutex));

    bool was_full = ((b->write_pos + 1) % BUFFER_SIZE == b->read_pos);

    // Read the data and advance read pointer.
    int data = b->buffer[b->read_pos];
    b->read_pos++;
    if (b->read_pos >= BUFFER_SIZE)
        b->read_pos = 0;

    // Signal that the buffer is not full anymore.
    if (was_full)
        ASSERT_ZERO(pthread_cond_signal(&b->not_full));

    ASSERT_ZERO(pthread_mutex_unlock(&b->mutex));

    return data;
}

/* ---------------------------- A test program ----------------------------
 * One thread inserts integers from 1 to 10000,
 * the other reads them and prints them.
 *
 * All integers between 0 and 9999 should be printed exactly twice,
 * once to the right of the arrow and once to the left.
 */

#define END_OF_QUEUE (-1)

void* producer_main(void* data)
{
    struct Queue* queue = data;
    for (int n = 0; n < 10000; n++) {
        printf("%d --->\n", n);
        queue_put(queue, n);
    }
    queue_put(queue, END_OF_QUEUE);
    return NULL;
}

void* consumer_main(void* data)
{
    struct Queue* queue = data;
    while (1) {
        int d = queue_get(queue);
        if (d == END_OF_QUEUE)
            break;
        printf("---> %d\n", d);
    }
    return NULL;
}

int main()
{
    struct Queue queue;
    queue_init(&queue);

    // Create (and start) the threads.
    pthread_t producer_thread, consumer_thread;
    ASSERT_ZERO(pthread_create(&producer_thread, NULL, producer_main, (void*)&queue));
    ASSERT_ZERO(pthread_create(&consumer_thread, NULL, consumer_main, (void*)&queue));

    // Wait until producer and consumer finish.
    ASSERT_ZERO(pthread_join(producer_thread, NULL));
    ASSERT_ZERO(pthread_join(consumer_thread, NULL));

    queue_destroy(&queue);
    return 0;
}
