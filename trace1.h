#ifndef MY_TRACE_H
#define MY_TRACE_H

#include <string.h>
#include <stdio.h>
#include <ck_ring.h>

// 2^20 buffer size
#define MAX_EVENTS 1048576
#define MAX_ARGS 10

// Define event types
#define EVENT_A 1
#define EVENT_B 2


// Trace structure
struct t_event {
    int args[MAX_ARGS];
    
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
 * @param others the 
 * @return -1 on error, the time elapsed otherwise
 */ 
static inline bool trace_event(const char * format, const int num_args, int a, int b, int c, \
                        int d, int e, int f, int g, int h, int i, int j) 
{
    // Get the write ptr
    struct t_event new_trace;
    bool ret = true;

    new_trace.format = format;
    new_trace.num_args = num_args;
    // Add arguments to the trace structure based on event type
    switch(num_args)  {
        case 3:
            new_trace.args[2] = c;

        case 2:
            new_trace.args[1] = b;
            
        case 1:
            new_trace.args[0] = a;

        default:
            break;
    }

    // Enqueue into the ring buffer
    ret = CK_RING_ENQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &new_trace);
    
    return ret;
}


// Start from current write ptr, we read the entire buffer
// @return 0 on success, -1 on any error
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
            switch(cur_trace.num_args) {
                case 1:
                    printf((char*)cur_trace.format, cur_trace.args[0]);
                    break;

                case 2:
                    printf((char*)cur_trace.format, cur_trace.args[0], cur_trace.args[1]);
                    break;

            }
            
        }
        
    }
    return ret;
}


#define TRACE_EVENT(format, num_args, a, b, c, d, e, f, g, h, i, j) \
    trace_event(format, num_args, a, b, c, d, e, f, g, h, i, j)


#endif