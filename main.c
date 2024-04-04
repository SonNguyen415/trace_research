#include <stdio.h>
#include <assert.h>
// #include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include "trace2.h"

#define TEST_ENTRY false
#define TEST_PERFORMANCE true

// These are for the performance test

#define NWRITERS 16
#define NENQUEUE (MAX_EVENTS / 4)
#define NTRIALS 1024
#define OUTLIER_THRESHOLD 1024


static inline uint32_t rdtsc(void) 
{
    uint32_t a = 0;
    asm volatile("rdtscp": "=a"(a):: "edx");
    return a;
}


// Get the cost of rdtsc, this is done across 1,000,000 trials
// @return the average time rdtsc takes
double get_rdtsc() {
    uint32_t start_time, end_time;
    int time_elapsed, trials = 1000000;
    double avg_time, total_time = 0;


    for(int i=0; i<trials; i++) {
        start_time = rdtsc();
        end_time = rdtsc();
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
        int res = trace_event(new_format, EVENT_A, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
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
        int res = trace_event(new_format, EVENT_A, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
        assert(res);
    }

    new_format = "Event B: a: %d | b: %d\n";
    for(int i=0; i<2; i++) {
        int res = trace_event(new_format, EVENT_B, 5, 1, 0 , 0, 0, 0, 0, 0, 0, 0);
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
        res = trace_event(new_format, EVENT_A, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
        assert(res >= 0);
    }

    res = trace_event(new_format, EVENT_A, 4, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
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

    for(int j=0; j < NTRIALS; j++) {
        trace_init();
        for(int i=0; i<NENQUEUE; i++) {
            time_start = rdtsc();
            int res = TRACE_EVENT(new_format, EVENT_A, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
            time_end = rdtsc();
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
    printf("Accounting for RDTSC: %.3f\n", avg_time-rdtsc_cost);
    printf("Count Outliers: %d out of %d\n", count, (NTRIALS*NENQUEUE));
}



// Performance calculation
void test5(double rdtsc_cost) {
    printf("Test 5: Performance Test for a multiple writers\n");   

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
        double rdtsc_cost = get_rdtsc();
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