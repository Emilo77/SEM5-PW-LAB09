#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "err.h"

#define N_THREADS 10
#define NAP_TIME_SECONDS 1
#define LIMIT 1000 // We will count primes between 2 and LIMIT - 1.

void* worker(void* data)
{
    int id = *((int*)data);
    free(data);
    printf("Thread %d: pid=%d, tid=%lu.\n", id, getpid(), pthread_self());

    int* prime_count = malloc(sizeof(int));
    *prime_count = 0;

    for (int number = 2 + id; number < LIMIT; number += N_THREADS) {
        bool is_prime = true;
        for (int divisor = 2; divisor <= number / 2; divisor++) {
            if (number % divisor == 0) {
                is_prime = false;
                break;
            }
        }
        if (is_prime) {
            printf("Thread %d: %d is prime.\n", id, number);
            (*prime_count)++;
        }
    }

    return prime_count;
}

int main(int argc, char* argv[])
{
    int detach_state = argc > 1 ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE;

    printf("Process %d is creating threads.\n", getpid());

    // Create thread attributes.
    pthread_attr_t attr;
    ASSERT_ZERO(pthread_attr_init(&attr));
    ASSERT_ZERO(pthread_attr_setdetachstate(&attr, detach_state));

    // Create (and start) the threads.
    pthread_t threads[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        int* worker_arg = malloc(sizeof(int));
        *worker_arg = i;
        ASSERT_ZERO(pthread_create(&threads[i], &attr, worker, worker_arg));
    }

    if (detach_state == PTHREAD_CREATE_DETACHED) {
        sleep(NAP_TIME_SECONDS);
        printf("Main thread finished.\n");
    } else {
        printf("Main thread is waiting for workers...\n");
        for (int i = 0; i < N_THREADS; i++) {
            int* result;
            ASSERT_ZERO(pthread_join(threads[i], (void**)&result));
            printf("Thread %d returned prime count: %d\n", i, *result);
            free(result);
        }
    }

    ASSERT_ZERO(pthread_attr_destroy(&attr));

    return 0;
}
