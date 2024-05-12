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
#define SINGLE_OUTLIER_THRESHOLD 1024
#define MULTI_OUTLIER_THRESHOLD 8192

// Setting this as false will make it so that multi writer test will randomize wait time in between
#define TEST_WORST_CASE true
// Set to true for test 7 and 9 (write and read simultaneously)
#define TEST_READ_WRITE false

// How much data to write for the output test
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
    assert(!res);

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




// Performance calculation for single writer
void test6_7(int test, double rdtsc_cost, char * format, int num_args, unsigned long args[]) {
    printf("Test %d: Performance Test for a single writer with %d arguments\n", test, num_args);
    if(test == 7) {
        printf("    Reading while writing\n");
    }

    double avg_time, total_time = 0;
    uint64_t time_start,time_end, time_elapsed;
    int count = 0; 

    remove("test7.csv");

    for(int i=0; i < NTRIALS; i++) {
        TRACE_INIT();
        for(int j=0; j<NENQUEUE; j++) {
            time_start = rdtscp();
            bool res = ENQUEUE_TRACE(format, num_args, args);
            time_end = rdtscp();
            assert(res);

            time_elapsed = time_end-time_start;

            if(test == 7) {
                APPEND_TRACE("test7.csv");
            }

            if(time_elapsed > SINGLE_OUTLIER_THRESHOLD) {
                count++;
            } else {
                total_time += time_elapsed;
            }

            
        }
    }
    
    avg_time = total_time / (NTRIALS*NENQUEUE-count);
    printf("Average time taken: %.3f\n", avg_time-rdtsc_cost);
    printf("Count Outliers: %d out of %d\n", count, (NTRIALS*NENQUEUE));
}



struct thd_data {
    double avg_time;
    int outliers;
};


void * thread_trace(void * arg) {
    struct thd_data * data = (struct thd_data*) arg;
    uint64_t time_start, time_end, time_elapsed;
    unsigned long args[4] = {1, 2, 3, 4};
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
        
        if(time_elapsed > 0 && time_elapsed < MULTI_OUTLIER_THRESHOLD) {
            data->avg_time += time_elapsed;    
        } else {
            data->outliers++;
        }
    }

    data->avg_time = data->avg_time / (NENQUEUE-data->outliers);

    pthread_exit(data);
    return data;
}


// Performance calculation for nwriters
void test8_9(int test, double rdtsc_cost, int nwriters) {
    printf("Test %d: Performance Test for %d writers\n", test, nwriters);   
    if(test == 9) {
        printf("    Reading while writing\n");
    }
    
    pthread_t writers[nwriters];
    struct thd_data * data[nwriters];
    double th_results[nwriters];
    double thd_outliers[nwriters];
    int i,j, outliers = 0;
    double trial_time, avg_time = 0;

    
    for(i=0; i < NTRIALS; i++) {
        remove("test9.csv");
        
        // Seed random number generator so we can induce randomness in multiple writers
        if(!TEST_WORST_CASE) {
            srand(time(NULL));
        }

        trial_time = 0;

        TRACE_INIT();

        // Create the threads
        for(j=0; j < nwriters; j++) {
            // Create the structs to hold the data
            data[j] = (struct thd_data *)malloc(sizeof(struct thd_data));
            if(data[j] == NULL) {
               return;
            }           
            data[j]->avg_time = 0;
            data[j]->outliers = 0;

            // Make the threads, passing the arguments onto it
            if(pthread_create(&writers[j], NULL, thread_trace, data[j]) != 0) {
                fprintf(stderr, "Error creating thread %d.\n", j);
                return;
            }
        }

        if(test == 9) {
            APPEND_TRACE("test9.csv");
        }

        // Wait for threads to finish
        for(j=0; j < nwriters; j++) {
            struct thd_data * thd_result;
            if(pthread_join(writers[j], (void**)&thd_result) != 0) {
                fprintf(stderr, "Error joining thread %d.\n", j);
            }
            th_results[j] = thd_result->avg_time;
            thd_outliers[j] = thd_result->outliers;
            free(thd_result);
        }
        
        // Aggregate average value
        for(j=0; j < nwriters; j++) {
            trial_time += th_results[j];
            outliers += thd_outliers[j];
        }

        trial_time = trial_time / nwriters;
        avg_time += trial_time;
        
    }

   
    avg_time = avg_time / NTRIALS;
    
    printf("Trials: %d | Writers: %d | Enqueue per trial: %d\n", NTRIALS, nwriters, NENQUEUE);
    printf("Average time taken: %.3f\n", avg_time);
    printf("Accounting for RDTSCP: %.3f\n", avg_time-rdtsc_cost);
    printf("Count Outliers: %d out of %d\n", outliers, (NTRIALS*NENQUEUE*nwriters));
}


void * test10_thread_a(void * arg) {

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


void * test10_thread_b(void * arg) {

    unsigned long args[6] = {1, 2, 3, 4, 5, 6};
    char * format = "Event A: %lu, %lu, %lu, %lu, %lu, %lu\n";
    bool res = false;

    // Enqueue to the ring buffer twice
    for(int i=0; i<3; i++) {
        res = ENQUEUE_TRACE(format, 6, args);
        assert(res);
    }
    
    return NULL;
}


// Test if file print is accurate
void test10() {
    printf("Test 10: Test if outputting trace event to csv is accurate\n");
    int i, nwriters;
    bool ret;
    nwriters = 4;
    pthread_t writers[nwriters];

    TRACE_INIT();

   // Make the threads
    for(i=0; i < nwriters; i++) {
        if(i % 2 == 0) {
            if(pthread_create(&writers[i], NULL, test10_thread_a, NULL) != 0) {
                fprintf(stderr, "Error creating thread %d.\n", i);
                return;
            }
        } else {
            if(pthread_create(&writers[i], NULL, test10_thread_b, NULL) != 0) {
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

    ret = OUTPUT_TRACE("test10.csv");
    assert(ret);
    
   
    printf("Test 10 Completed\n");
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
        printf("CPUS Available: %d\n", get_nprocs());
        printf("----------------------------------------------\n"); 

        char * format = "Event A: %lu\n";
        unsigned long args_a[1] = {5};

        // 6a. Performance testing - single writer
        test6_7(6, rdtsc_cost, format, 1, args_a);
        printf("----------------------------------------------\n"); 
        
        // 6c. Performance testing - 4 args per events
        unsigned long args_b[4] = {1, 2, 3, 4};
        format = "Event B: %lu, %lu, %lu, %lu\n";
        test6_7(6, rdtsc_cost, format, 4, args_b);
        printf("----------------------------------------------\n"); 
        
        // 6c. Performance testing - 8 args per events
        unsigned long args_c[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        format = "Event C: %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu\n";
        test6_7(6, rdtsc_cost, format, 8, args_c);
        printf("----------------------------------------------\n"); 
        printf("Test 6 Completed\n");
        printf("----------------------------------------------\n\n");


        if(TEST_READ_WRITE) {
            // 7a. Test reading while writing with 1 argument
            test6_7(7, rdtsc_cost, format, 1, args_a);
            printf("----------------------------------------------\n"); 

            // 7b. Test reading while writing with 4 arguments
            test6_7(7, rdtsc_cost, format, 4, args_b);
            printf("----------------------------------------------\n"); 

            // 7c. Test reading while writing with 8 arguments
            test6_7(7, rdtsc_cost, format, 8, args_c);
            printf("----------------------------------------------\n"); 

            printf("Test 7 Completed\n");
            printf("----------------------------------------------\n\n");
        }
       

        // 8a. Performance testing - 4 writers 
        test8_9(8, rdtsc_cost, 4);
        printf("----------------------------------------------\n");

        // 8b. Performance testing - 8 writers 
        test8_9(8, rdtsc_cost, 8);
        printf("----------------------------------------------\n");

        printf("Test 8 Completed\n");
        printf("----------------------------------------------\n\n");

        
        if(TEST_READ_WRITE) {
            // 9. Performance testing - 4 writers writing while reading
            test8_9(9, rdtsc_cost, 4);
            printf("----------------------------------------------\n");

            // 9. Performance testing - 8 writers writing while reading
            test8_9(9, rdtsc_cost, 8);
            printf("----------------------------------------------\n");

            printf("Test 9 Completed\n");
            printf("----------------------------------------------\n");
        }


    }

    if(TEST_OUTPUT) {
        // 10. Test if writing to file is accurate
        test10();
        printf("----------------------------------------------\n");
    }
   
    printf("Completed Tests\n");
}
