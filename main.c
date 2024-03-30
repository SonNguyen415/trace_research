#include <stdio.h>
#include <assert.h>
#include "my_trace.h"

#define TEST_ENTRY true
#define TEST_PERFORMANCE false
#define NWRITERS 16
#define NTRIALS 1000000

// Test basic enqueue
void test1() {
    printf("Test 1: Entering a single event\n");
    trace_init();

    char * new_format = "Event A: %d\n";
    for(int i=0; i<10; i++) {
        int res = trace_event(new_format, EVENT_A, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
        assert(res >= 0);
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
        assert(res >= 0);
    }

    new_format = "Event B: a: %d | b: %d\n";
    for(int i=0; i<2; i++) {
        int res = trace_event(new_format, EVENT_B, 5, 1, 0 , 0, 0, 0, 0, 0, 0, 0);
        assert(res >= 0);
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

    for(int j=0; j<MAX_EVENTS; j++) {
       
    }


    printf("Test 3 -- Passed\n");
}



// Performance calculation
void test4(double rdtsc_cost) {
    printf("Test 4: Performance Test for a single writer\n");

    char * new_format = "Event A: %d\n";
    double avg_time, total_time = 0;
    int num_events = 16;
    int count = 0;
    int outlier_threshold = 1000;
    
    for(int i=0; i < NTRIALS; i++) {
        trace_init();
        for(int j=0; j<num_events; j++) {
            int res = TRACE_EVENT(new_format, EVENT_A, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
            assert(res >= 0);
            if(res > outlier_threshold) {
                count++;
            }
            total_time += res;
        }
    }
    
    avg_time = total_time / (num_events*NTRIALS);
    printf("Average time taken: %.3f\n", avg_time);
    printf("Accounting for RDTSC: %.3f\n", avg_time-rdtsc_cost);
    printf("Count Outliers: %d out of %d\n", count, NTRIALS*num_events);
}

// Performance calculation
void test5(double rdtsc_cost) {
    printf("Test 5: Performance Test for a multiple writers\n");   
    printf("Idk how to do this one\n");
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
        printf("RDTSC Cost: %0.3f\n", rdtsc_cost); 
        printf("----------------------------------------------\n"); 

        // 4. Performance testing - single writer
        test4(rdtsc_cost);

        // 5. Performance testing - multiple writers
        test5(rdtsc_cost);

    }
   
    printf("Completed Tests\n");
}