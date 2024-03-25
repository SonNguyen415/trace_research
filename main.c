#include <stdio.h>
#include "my_trace.h"


// Test basic enqueue
void test1() {
   trace_init();

    char * new_format = "Event A: %d\n";
    for(int i=0; i<10; i++) {
        int res = trace_event(new_format, EVENT_A, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
        if(res < 0) {
            printf("Enqueue Error\n");
        } 
    }

}

// Enqueue different event types
void test2() {
    
}

// Enqueue over-max events. Should overwrite




// Performance calculation
void test7() {
    char * new_format = "Event A: %d\n";
    double avg_time, total_time = 0;
    int num_events = 16;
    int count = 0;
    int trials = 10000000;
    int outlier_threshold = 1000;
    double rdtsc_cost = get_rdtsc();

    printf("RDTSC Cost: %0.3f\n", rdtsc_cost);
    for(int j=0; j < trials; j++) {
        trace_init();
        for(int i=0; i<num_events; i++) {
            int res = TRACE_EVENT(new_format, EVENT_A, 5, 0, 0 , 0, 0, 0, 0, 0, 0, 0);
            if(res < 0) {
                printf("Enqueue Error\n");
            } else {
                if(res > outlier_threshold) {
                    count++;
                }
                total_time += res;
            }
        }
    }
    
    avg_time = total_time / (num_events*trials);
    printf("Average time taken: %.3f\n", avg_time);
    printf("Accounting for RDTSC: %.3f\n", avg_time-rdtsc_cost);
    printf("Count Outliers: %d out of %d\n", count, trials*num_events);
}




int main() {    


    
    // Tests:
    // 1. Basic test for entering event and output them
    test1();

    // 2. Entering different event types

    // 3. Testing overwrite
    test2();


    // 4. Multiple writers, 1 reader
    // 5. Single writer and read when buffer is full
    // 6. Multiple writers, buffer is full


    // 7. Performance testing - single writer
    test7();

    // 8. Performance testing - multiple writers


    printf("Completed process\n");
}