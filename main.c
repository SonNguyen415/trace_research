#include <stdio.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "trace1.h"

#define TEST_ENTRY false
#define TEST_PERFORMANCE true

// These are for the performance test

#define NWRITERS 1
#define NENQUEUE 1
#define NTRIALS 1
#define OUTLIER_THRESHOLD 1024


// Get the cost of rdtsc, this is done across 1,000,000 trials
// @return the average time rdtsc takes
double get_rdtscp() {
    uint32_t start_time, end_time;
    int time_elapsed, trials = 1000000;
    double avg_time, total_time = 0;


    for(int i=0; i<trials; i++) {
        start_time = RDTSCP();
        end_time = RDTSCP();
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

    char * new_format = "Event A: %d\n";
    for(int i=0; i<10; i++) {
        int res = trace_event(new_format, 1, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
        assert(res);
    }

    printf("Test 1 -- Passed\n");
}

// Enqueue different event types
void test2() {
    printf("Test 2: Entering events of different types\n");
    trace_init();

    char * new_format = "Event A: %d\n";
    for(int i=0; i<4; i++) {
        int res = trace_event(new_format, 1, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
        assert(res);
    }

    new_format = "Event B: a: %d | b: %d\n";
    for(int i=0; i<2; i++) {
        int res = trace_event(new_format, 2, 5, 1, 0 , 0, 0, 0, 0, 0, 0, 0);
        assert(res);
    }

    printf("Test 2 -- Passed\n");
}

// Enqueue over-max events. Overwrite??
void test3() {
    printf("Test 3: Testing for overwrite\n");

    // Insert max elements
    trace_init();

    char * new_format = "Event A: %d\n";
    int res = -1;
    for(int i=0; i<MAX_EVENTS-1; i++) {
        res = trace_event(new_format, 1, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
        assert(res >= 0);
    }

    res = trace_event(new_format, 1, 4, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
    printf("Return: %d\n", res);

    printf("Test 3 -- Passed\n");
}


// Performance calculation
void test4(double rdtsc_cost) {
    printf("Test 4: Performance Test for a single writer\n");

    char * new_format = "Event A: %d\n";
    double avg_time, total_time = 0;
    int time_start,time_end, time_elapsed;
    int count = 0;

    // Create a CSV file to write to
    // FILE *fpt;
    // fpt = fopen("SingleWriter.csv", "w+");

    // // Headers for csv
    // fprintf(fpt,"Trial, Enqueue, Time Elapsed\n");

    for(int i=0; i < NTRIALS; i++) {
        trace_init();
        for(int j=0; j<NENQUEUE; j++) {
            time_start = RDTSCP();
            bool res = TRACE_EVENT(new_format, 1, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
            time_end = RDTSCP();
            assert(res);

            time_elapsed = time_end-time_start;

            
            // fprintf(fpt,"%d, %d, %d\n", i, j, time_elapsed);

            if(time_elapsed > OUTLIER_THRESHOLD) {
                count++;
            } else {
                total_time += time_elapsed;
            }
        }
    }

    // fclose(fpt);
    
    
    avg_time = total_time / (NTRIALS*NENQUEUE);
    printf("Average time taken: %.3f\n", avg_time);
    printf("Accounting for RDTSCP: %.3f\n", avg_time-rdtsc_cost);
    printf("Count Outliers: %d out of %d\n", count, (NTRIALS*NENQUEUE));
    printf("Test 4 Completed\n");
}


void * thread_trace(void * arg) {
    int thd_id = *((int *)arg);
    double time_start, time_end;

    // Set up average time for returning later from thread
    double * avg_time = (double *)malloc(sizeof(double));
    if(avg_time == NULL) {
        fprintf(stderr, "Malloc Error in Thread: %d.\n", thd_id);
        pthread_exit(NULL);
    }
    *avg_time = 0;

    char * new_format = "Event A: %d\n";

    // Enqueue to the ring buffer NENQUEUE times
    for(int i=0; i<NENQUEUE; i++) {
        time_start = RDTSCP();
        bool res = TRACE_EVENT(new_format, 1, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
        time_end = RDTSCP();

        assert(res);

        *avg_time += time_end - time_start;    
        // printf("    Time: %.3f\n", time_end - time_start);
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
    double avg_time = 0;

   // Make the threads
    for(i=0; i < NTRIALS; i++) {
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
            avg_time += th_results[j];
        }

    }

    avg_time = avg_time / (NTRIALS*NWRITERS);
    
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
        double rdtsc_cost = get_rdtscp();
        printf("RDTSCP Cost: %0.3f\n", rdtsc_cost); 
        printf("----------------------------------------------\n"); 

        // 4. Performance testing - single writer
        test4(rdtsc_cost);
        printf("----------------------------------------------\n"); 

        // 5. Performance testing - multiple writers
        test5(rdtsc_cost);
        printf("----------------------------------------------\n");

    }
   
    printf("Completed Tests\n");
}