#include <stdio.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NARGS 8
#include "tracer.h"

#define TEST_ENTRY false
#define TEST_PERFORMANCE true

// These are for the entry test
#define NENTRY 1024

// These are for the performance test
#define NWRITERS 8
#define NENQUEUE 1024
#define NTRIALS 4096
#define OUTLIER_THRESHOLD 1024*1024
#define TEST_WORST_CASE true


// Read current time stamp
static inline uint64_t rdtscp(void) 
{
    unsigned int low,high;
    asm volatile("rdtscp": "=a"(low), "=d" (high));
    return ((uint64_t) high << 32 | low);
}

// Get the cost of rdtsc, this is done across 1,000,000 trials
// @return the average time rdtsc takes
double get_rdtscp_cost() {
    uint64_t start_time, end_time, time_elapsed;
    int trials = 1000000;
    double avg_time, total_time = 0;

    for(int i=0; i<trials; i++) {
        start_time = rdtscp();
        end_time = rdtscp();
        time_elapsed = end_time - start_time;
        total_time += time_elapsed;
    }
    avg_time = total_time / trials;
    
    return avg_time;
}

// Test basic enqueue
void test1() {
    printf("Test 1: Entering a single event\n");
    trace_init();

    int i;
    unsigned long args[1] = {5};
    char * format = "Event A: %d\n";
    
    // Enqueue NENTRY items
    for(i=0; i<NENTRY; i++) {
        bool res = trace_event(format, 1, args);
        assert(res);
    }

    // Dequeue NENTRY items
    for(i=0; i<NENTRY; i++) {
        bool res = GET_TRACE();
        assert(res);
    }

    printf("Test 1 -- Passed\n");
}

// Enqueue different event types
void test2() {
    printf("Test 2: Entering events of different types\n");
    trace_init();
    int i;
    char * format;

    unsigned long args_a[1] = {5};
    format = "Event A: %d\n";
    for(i=0; i<NENTRY; i++) {
        bool res = trace_event(format, 1, args_a);
        assert(res);
    }

    
    unsigned long args_b[2] = {4, 3};
    format = "Event B: a: %d | b: %d\n";
    for(i=0; i<NENTRY; i++) {
        bool res = trace_event(format, 2, args_b);
        assert(res);
    }

    for(i=0; i<NENTRY*2; i++) {
        bool res = GET_TRACE();
        assert(res);
    }

    printf("Test 2 -- Passed\n");
}

// Enqueue over-max events. Overwrite??
void test3() {
    printf("Test 3: Testing for overwrite\n");

    // Insert max elements
    trace_init();

    unsigned long args[1] = {5};
    char * format = "Event A: %d\n";
    bool res = false;
    for(int i=0; i<MAX_EVENTS-1; i++) {
        bool res = trace_event(format, 1, args);
        assert(res);
    }

    args[0] = 4;
    // Should return false when you overwrite
    res = trace_event(format, 1, args);
    assert(res == false);

    printf("Test 3 -- Passed\n");
}


// Performance calculation
void test4(double rdtsc_cost, char * format, int num_args, unsigned long args[]) {
    printf("Test 4: Performance Test for a single writer with %d arguments\n", num_args);

   
    double avg_time, total_time = 0;
    uint64_t time_start,time_end, time_elapsed;
    int count = 0;

    for(int i=0; i < NTRIALS; i++) {
        trace_init();
        for(int j=0; j<NENQUEUE; j++) {
            time_start = rdtscp();
            // printf("Time start: %ld\n", time_start);
            bool res = TRACE_EVENT(format, num_args, args);
            time_end = rdtscp();
            // printf("Time end: %ld\n", time_end);
            assert(res);

            time_elapsed = time_end-time_start;

            if(time_elapsed > OUTLIER_THRESHOLD) {
                count++;
            } else {
                total_time += time_elapsed;
            }

            
        }
    }
    
    avg_time = total_time / (NTRIALS*NENQUEUE);
    printf("Average time taken: %.3f\n", avg_time);
    printf("Accounting for RDTSCP: %.3f\n", avg_time-rdtsc_cost);
    printf("Count Outliers: %d out of %d\n", count, (NTRIALS*NENQUEUE));
    printf("Test 4 Completed\n");
}


void * thread_trace(void * arg) {
    int thd_id;
    uint64_t time_start, time_end, time_elapsed;

    thd_id = *((int *)arg);
    unsigned long args[4] = {1, 2, 3, 4};

    // Set up average time for returning later from thread
    double * avg_time = (double *)malloc(sizeof(double));
    if(avg_time == NULL) {
        fprintf(stderr, "Malloc Error in Thread: %d.\n", thd_id);
        pthread_exit(NULL);
    }
    *avg_time = 0;

    char * format = "Event A: %d\n";

    // Enqueue to the ring buffer NENQUEUE times
    for(int i=0; i<NENQUEUE; i++) {
        time_start = rdtscp();
        bool res = TRACE_EVENT(format, 1, args);
        time_end = rdtscp();

        assert(res);

        time_elapsed = time_end - time_start;

        if(!TEST_WORST_CASE) {
            int random_number = rand() % 5;
            usleep(random_number);
        }
        
        if(time_elapsed > 0) {
            *avg_time += time_elapsed;    
        }
    }

    *avg_time = *avg_time / NENQUEUE;


    pthread_exit(avg_time);
    return avg_time;
}


// Performance calculation
void test5(double rdtsc_cost) {
    printf("Test 5: Performance Test for multiple writers\n");   
    printf("CPUS Available: %d\n", get_nprocs());
    
    pthread_t writers[NWRITERS];
    double th_results[NWRITERS];
    int i,j;
    double trial_time, avg_time = 0;

   

   // Make the threads
    for(i=0; i < NTRIALS; i++) {
        // Seed random number generator so we can induce randomness in multiple writers
        if(!TEST_WORST_CASE) {
            srand(time(NULL));
        }

        trial_time = 0;

        trace_init();

        // Create the threads
        for(j=0; j < NWRITERS; j++) {
            if(pthread_create(&writers[j], NULL, thread_trace, &j) != 0) {
                fprintf(stderr, "Error creating thread %d.\n", j);
            }
        }

        // Wait for threads to finish
        for(j=0; j < NWRITERS; j++) {
            double * thd_result;
            if(pthread_join(writers[j], (void**)&thd_result) != 0) {
                fprintf(stderr, "Error joining thread %d.\n", j);
            }
            th_results[j] = *thd_result;
            free(thd_result);
        }
        
        // Aggregate average value
        for(j=0; j < NWRITERS; j++) {
            trial_time += th_results[j];
        }

        trial_time = trial_time / NWRITERS;
        avg_time += trial_time;
    }

   
    avg_time = avg_time / NTRIALS;
    
    printf("Trials: %d | Writers: %d | Enqueue per trial: %d\n", NTRIALS, NWRITERS, NENQUEUE);
    printf("Average time taken: %.3f\n", avg_time);
    printf("Accounting for RDTSCP: %.3f\n", avg_time-rdtsc_cost);
    printf("Test 5 Completed\n");

}




int main() {  
    // Tests:
    if(TEST_ENTRY) {
        // 1. Basic test for entering event and output them
        test1();
        printf("----------------------------------------------\n");

        // 2. Entering different event types
        test2();
        printf("----------------------------------------------\n");

        // 3. Testing overwrite
        test3();
        printf("----------------------------------------------\n");
    }
  

    if(TEST_PERFORMANCE) {
       
        double rdtsc_cost = get_rdtscp_cost();
        printf("RDTSCP Cost: %0.3f\n", rdtsc_cost); 
        printf("----------------------------------------------\n"); 

        char * format = "Event A: %d\n";
        unsigned long args_a[1] = {5};

        // 4a. Performance testing - single writer
        test4(rdtsc_cost, format, 1, args_a);
        printf("----------------------------------------------\n"); 
        
          // 4c. Performance testing - 4 args per events
        unsigned long args_b[4] = {1, 2, 3, 4};
        test4(rdtsc_cost, format, 4, args_b);
        printf("----------------------------------------------\n"); 

        // 4c. Performance testing - 8 args per events
        unsigned long args_c[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        test4(rdtsc_cost, format, 8, args_c);
        printf("----------------------------------------------\n"); 

        // 5. Performance testing - multiple writers 
        test5(rdtsc_cost);
        printf("----------------------------------------------\n");

    }
   
    printf("Completed Tests\n");
}
