#include <stdio.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define TRACE_NARGS 8
#include "tracer.h"

#define TEST_ENTRY false
#define TEST_PERFORMANCE true
#define TEST_OUTPUT false

// These are for the entry test
#define NENTRY 1024

// These are for the performance test
#define NENQUEUE 1024
#define NTRIALS 4096
#define OUTLIER_THRESHOLD 65536
#define TEST_WORST_CASE true

// How many data to write for the tracer test
#define NDATA 64


// Read current time stamp
static inline uint64_t rdtscp(void) 
{
    unsigned int low,high;
    asm volatile("rdtscp": "=a"(low), "=d" (high));
    return ((uint64_t) high << 32 | (uint64_t) low);
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
    printf("Test 1: Dequeue from empty buffer should fail\n");

    TRACE_INIT();
    trace_event * my_event = DEQUEUE_TRACE();
    assert(!my_event);
    printf("Test 2 -- Passed\n");
}


// Test basic enqueue
void test2() {
    printf("Test 2: Entering a single event\n");
    TRACE_INIT();

    int i;
    unsigned long args[1] = {5};
    char * format = "Event A: %lu\n";
    
    // Enqueue NENTRY items
    for(i=0; i<NENTRY; i++) {
        bool res = ENQUEUE_TRACE(format, 1, args);
        assert(res);
    }

    // Dequeue NENTRY items
    for(i=0; i<NENTRY; i++) {
        trace_event * my_event = DEQUEUE_TRACE();
        assert(my_event);
    }

    printf("Test 2 -- Passed\n");
}

// Enqueue different event types
void test3() {
    printf("Test 3: Entering events of different types\n");
    TRACE_INIT();
    int i;
    char * format;

    unsigned long args_a[1] = {5};
    format = "Event A: %lu\n";
    for(i=0; i<NENTRY; i++) {
        bool res = ENQUEUE_TRACE(format, 1, args_a);
        assert(res);
    }

    
    unsigned long args_b[2] = {4, 3};
    format = "Event B: a: %lu | b: %lu\n";
    for(i=0; i<NENTRY; i++) {
        bool res = ENQUEUE_TRACE(format, 2, args_b);
        assert(res);
    }

    for(i=0; i<NENTRY*2; i++) {
        trace_event * my_event = DEQUEUE_TRACE();
        assert(my_event);
    }

    printf("Test 3 -- Passed\n");
}

// Enqueue over-max events. Overwrite??
void test4() {
    printf("Test 4: Testing for overwrite\n");

    // Insert max elements
    TRACE_INIT();

    unsigned long args[1] = {5};
    char * format = "Event A: %lu\n";
    bool res = false;
    for(int i=0; i<MAX_EVENTS-1; i++) {
        bool res = ENQUEUE_TRACE(format, 1, args);
        assert(res);
    }

    args[0] = 4;

    // Should return false when you overwrite
    res = ENQUEUE_TRACE(format, 1, args);
    assert(res == false);

    printf("Test 4 -- Passed\n");
}


// Test if dequeue actually store the data in the proper spot
void test5() {
    printf("Test 5: Testing if dequeue does store value\n");
    TRACE_INIT();

    unsigned long args[1] = {5};
    char * format = "Event A: %lu\n";
    bool res = ENQUEUE_TRACE(format, 1, args);
    assert(res);
    trace_event * my_event = DEQUEUE_TRACE();
    assert(my_event);
    assert(my_event->args[0] == args[0]);

    printf("Test 5 -- Passed\n");
}




// Performance calculation
void test6(double rdtsc_cost, char * format, int num_args, unsigned long args[]) {
    printf("Test 6: Performance Test for a single writer with %d arguments\n", num_args);

    double avg_time, total_time = 0;
    uint64_t time_start,time_end, time_elapsed;
    int count = 0;

    for(int i=0; i < NTRIALS; i++) {
        TRACE_INIT();
        for(int j=0; j<NENQUEUE; j++) {
            time_start = rdtscp();
            bool res = ENQUEUE_TRACE(format, num_args, args);
            time_end = rdtscp();
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
    printf("Test 6 Completed\n");
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

    char * format = "Event A: %lu, %lu, %lu, %lu\n";

    // Enqueue to the ring buffer NENQUEUE times
    for(int i=0; i<NENQUEUE; i++) {
        time_start = rdtscp();
        bool res = ENQUEUE_TRACE(format, 1, args);
        time_end = rdtscp();

        assert(res);

        time_elapsed = time_end - time_start;

        if(!TEST_WORST_CASE) {
            int random_number = rand() % 4;
            usleep(random_number);
        }
        
        if(time_elapsed > 0 && time_elapsed < OUTLIER_THRESHOLD) {
            *avg_time += time_elapsed;    
        }
    }

    *avg_time = *avg_time / NENQUEUE;


    pthread_exit(avg_time);
    return avg_time;
}


// Performance calculation
void test7(double rdtsc_cost, int nwriters) {
    printf("Test 7: Performance Test for %d writers\n", nwriters);   
    printf("CPUS Available: %d\n", get_nprocs());
    
    pthread_t writers[nwriters];
    double th_results[nwriters];
    int i,j;
    double trial_time, avg_time = 0;

   
    for(i=0; i < NTRIALS; i++) {
        // Seed random number generator so we can induce randomness in multiple writers
        if(!TEST_WORST_CASE) {
            srand(time(NULL));
        }

        trial_time = 0;

        TRACE_INIT();

        // Create the threads
        for(j=0; j < nwriters; j++) {
            if(pthread_create(&writers[j], NULL, thread_trace, &j) != 0) {
                fprintf(stderr, "Error creating thread %d.\n", j);
                return;
            }
        }

        // Wait for threads to finish
        for(j=0; j < nwriters; j++) {
            double * thd_result;
            if(pthread_join(writers[j], (void**)&thd_result) != 0) {
                fprintf(stderr, "Error joining thread %d.\n", j);
            }
            th_results[j] = *thd_result;
            free(thd_result);
        }
        
        // Aggregate average value
        for(j=0; j < nwriters; j++) {
            trial_time += th_results[j];
        }

        trial_time = trial_time / nwriters;
        avg_time += trial_time;
    }

   
    avg_time = avg_time / NTRIALS;
    
    printf("Trials: %d | Writers: %d | Enqueue per trial: %d\n", NTRIALS, nwriters, NENQUEUE);
    printf("Average time taken: %.3f\n", avg_time);
    printf("Accounting for RDTSCP: %.3f\n", avg_time-rdtsc_cost);
    printf("Test 7 Completed\n");

}



void * test8_thread_a(void * arg) {

    unsigned long args[3] = {1, 2, 3};
    char * format = "Event A: %lu, %lu, %lu, %lu\n";
    bool res = false;

    // Enqueue to the ring buffer twice
    for(int i=0; i<2; i++) {
        res = ENQUEUE_TRACE(format, 3, args);
        assert(res);
    }
    
    return NULL;
}


void * test8_thread_b(void * arg) {

    unsigned long args[6] = {1, 2, 3, 4, 5, 6};
    char * format = "Event A: %lu, %lu, %lu, %lu\n";
    bool res = false;

    // Enqueue to the ring buffer twice
    for(int i=0; i<3; i++) {
        res = ENQUEUE_TRACE(format, 6, args);
        assert(res);
    }
    
    return NULL;
}



// Test if file print is accurate
void test8() {
    printf("Test 8: Test if outputting trace event to csv is accurate\n");
    int i, nwriters;
    bool ret;
    nwriters = 4;
    pthread_t writers[nwriters];

    TRACE_INIT();

   // Make the threads
    for(i=0; i < nwriters; i++) {
        if(i % 2 == 0) {
            if(pthread_create(&writers[i], NULL, test8_thread_a, NULL) != 0) {
                fprintf(stderr, "Error creating thread %d.\n", i);
                return;
            }
        } else {
            if(pthread_create(&writers[i], NULL, test8_thread_b, NULL) != 0) {
                fprintf(stderr, "Error creating thread %d.\n", i);
                return;
            }
        }
      
    }
    
    // Wait for threads to finish
    for(i=0; i < nwriters; i++) {
        if(pthread_join(writers[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread %d.\n",i);
        }
    }

    ret = OUTPUT_TRACE("test.csv", "w");
    assert(ret);
    
   
    printf("Test 8 Completed\n");
}




int main() {  
    // Tests:
    if(TEST_ENTRY) {
        // 1. Test dequeue from empty buffer
        test1();
        printf("----------------------------------------------\n");

        // 2. Basic test for entering event and output them
        test2();
        printf("----------------------------------------------\n");

        // 3. Entering different event types
        test3();
        printf("----------------------------------------------\n");

        // 4. Testing overwrite
        test4();
        printf("----------------------------------------------\n");

        // 5. Test if dequeue result in data
        test5();
        printf("----------------------------------------------\n");
    }
  

    if(TEST_PERFORMANCE) {
       
        double rdtsc_cost = get_rdtscp_cost();
        printf("RDTSCP Cost: %0.3f\n", rdtsc_cost); 
        printf("----------------------------------------------\n"); 

        char * format = "Event A: %lu\n";
        unsigned long args_a[1] = {5};

        // 6a. Performance testing - single writer
        test6(rdtsc_cost, format, 1, args_a);
        printf("----------------------------------------------\n"); 
        
        // 6c. Performance testing - 4 args per events
        unsigned long args_b[4] = {1, 2, 3, 4};
        format = "Event B: %lu, %lu, %lu, %lu\n";
        test6(rdtsc_cost, format, 4, args_b);
        printf("----------------------------------------------\n"); 

        // 6c. Performance testing - 8 args per events
        unsigned long args_c[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        format = "Event C: %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n";
        test6(rdtsc_cost, format, 8, args_c);
        printf("----------------------------------------------\n"); 

        // 7a. Performance testing - 4 writers 
        test7(rdtsc_cost, 4);
        printf("----------------------------------------------\n");

        // 7b. Performance testing - 8 writers 
        test7(rdtsc_cost, 8);
        printf("----------------------------------------------\n");

    }

    if(TEST_OUTPUT) {
        // 8. Test if writing to file is accurate
        test8();
        printf("----------------------------------------------\n");
    }
   
    printf("Completed Tests\n");
}
