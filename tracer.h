#ifndef MY_TRACE_H
#define MY_TRACE_H

#include <string.h>
#include <stdio.h>
#include <ck_ring.h>

// 2^20 buffer size
#define MAX_EVENTS 1048576
#define NARGS 1

// Trace structure
struct t_event {
    int args[NARGS];
    
    // Some other variables for timestamp and stuff that all events should store
    const char * format;
    int num_args;
};


struct t_trace_buffer {
    struct t_event traces[MAX_EVENTS];
    struct ck_ring my_ring;
} trace_buffer;


CK_RING_PROTOTYPE(trace_buffer, t_event);


// Initializer for the ck ring
void trace_init() 
{
    ck_ring_init(&trace_buffer.my_ring, MAX_EVENTS);
}


/* Test function to add an event to the trace buffer
 * @param format the string that the data will be written into when dequeued
 * @param num_args the number of arguments to the event
 * @return true on success, false otherwise
 */ 
static inline bool trace_event(const char * format, const int num_args, int args[]) 
{
    // Get the write ptr
    struct t_event new_trace;
    bool ret = true;

    new_trace.format = format;
    new_trace.num_args = num_args;
    
    // Add arguments to the trace structure based on event type

    for(int i=0; i<num_args; i++) {
        new_trace.args[i] = args[i];
        printf("i=%d | num_args: %d\n", i, num_args);
    }
   

    // Enqueue into the ring buffer
    ret = CK_RING_ENQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &new_trace);
    
    return ret;
}


// Start from current write ptr, we read the entire buffer
// @return true on success, false otherwise
bool output_trace() 
{
    struct t_event cur_trace;
    bool ret = true;
    int num_events = ck_ring_size(&trace_buffer.my_ring);

    for(int i=0; i < num_events; i++) {
        ret = CK_RING_DEQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &cur_trace);
       
        if(!ret) {
            return ret;
        } 
            
        if(cur_trace.format != NULL) {
            
        }
        
    }
    return ret;
}


// Read the buffer, but do not output to file, only dequeue 
// @return true on success, false otherwise
bool get_trace() {
    struct t_event cur_trace;
    bool ret = true;
    int num_events = ck_ring_size(&trace_buffer.my_ring);

    for(int i=0; i < num_events; i++) {
        ret = CK_RING_DEQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &cur_trace);
       
        if(!ret) {
            return ret;
        } 
        
    }
    return ret;
}  



static inline uint32_t rdtscp(void) 
{
    uint32_t a = 0;
    asm volatile("rdtscp": "=a"(a):: "edx");
    return a;
}


#define TRACE_EVENT(format, num_args, args) \
    trace_event(format, num_args, args)


#define RDTSCP(void) rdtscp(void)

#endif