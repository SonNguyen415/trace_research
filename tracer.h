#ifndef MY_TRACE_H
#define MY_TRACE_H

#include <string.h>
#include <stdio.h>
#include <ck_ring.h>


// 2^20 buffer size
#define MAX_EVENTS 1048576

#ifndef NARGS
#define NARGS 1 // Default value
#endif

// Trace structure
struct t_event {
    unsigned long args[NARGS];
    
    // Some other variables for timestamp and stuff that all events should store    
    const char * format;
    uint64_t time_stamp;
    uint64_t cpuid;
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
 * @param num_args the number of arguments to the event, specifying the length of args
 * @param args the array containing the argumefnts to be inserted into format
 * @return true on success, false otherwise
 */ 
static inline bool trace_event(const char * format, const int num_args, unsigned long args[]) 
{
    unsigned int low,high;
    struct t_event new_trace;
    bool ret = true;

    new_trace.format = format;
    asm volatile("rdtsc": "=a"(low), "=d" (high));
    new_trace.time_stamp = ((uint64_t) high << 32 | low);
    asm volatile("cpuid": "=a"(new_trace.cpuid)::);
    new_trace.num_args = num_args;
    
    // Add arguments to the trace structure based on event type
    for(int i=0; i<num_args; i++) {
        new_trace.args[i] = args[i];
    }

    // Enqueue into the ring buffer
    ret = CK_RING_ENQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &new_trace);
    
    return ret;
}


// Start from current write ptr, we read the entire buffer
// @return true on success, false otherwise
static inline bool output_trace() 
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
static inline bool get_trace() 
{
    struct t_event cur_trace;
    bool ret ;
    ret = CK_RING_DEQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &cur_trace);
    return ret;
}  


#define TRACE_EVENT(format, num_args, args) \
    trace_event(format, num_args, args)

#define GET_TRACE() get_trace()

#define OUTPUT_TRACE() output_trace()


#endif
