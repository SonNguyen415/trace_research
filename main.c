#include <stdio.h>
#include "my_trace.h"

void test1() {
    char * new_format = "Event A: %d\n";
    for(int i=0; i<10; i++) {
        trace_event(new_format, EVENT_A, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    new_format = "Event B: %d, %d\n";
    for(int i=0; i<10; i++) {
        trace_event(new_format, EVENT_B, 4, 11, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    
    output_trace();
}


void test2() {
    
}

int main() {    


    // Tests:
    // 1. Basic test for entering event and output them
    test1();

    // 2. Testing overwrite
    test2();


    // 3. Multiple writers, 1 reader
    // 4. Single writer and read when buffer is full
    // 5. Multiple writers, buffer is full
    // When buffer is full, do we suspend writing when we read?   


    printf("Completed process\n");
}