#include "my_trace.h"

struct t_trace_buffer {
    struct trace traces[MAX_EVENTS];
    struct ck_ring my_ring;
} trace_buffer;


CK_RING_PROTOTYPE(trace_buffer, trace);

void trace_init() {
    ck_ring_init(&trace_buffer.my_ring, MAX_EVENTS);
}

// This is horrible. This is hard coded. I hate it
int trace_event(const char * format, int event_type, int a, int b, int c, \
                        int d, int e, int f, int g, int h, int i, int j) 
{
    // Get the write ptr
    struct trace new_trace;
    bool res = true;


    new_trace.format = format;
    new_trace.event_type = event_type;
    
    // Add arguments to the trace structure based on event type
    switch(event_type)  {
        case EVENT_A:
            new_trace.event_a.a = a;
            break;

        case EVENT_B:
            new_trace.event_b.a = a;
            new_trace.event_b.b = b;
            break;
    }


    // Enqueue into the ring buffer
    res = CK_RING_ENQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &new_trace);
    if(!res) {
        return -1;
    }
    return 0;
}


// Start from current write ptr, we read the entire buffer
int output_trace() 
{
    struct trace cur_trace;
    bool res = true;
    int num_events = ck_ring_size(&trace_buffer.my_ring);

    for(int i=0; i < num_events; i++) {
        res = CK_RING_DEQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &cur_trace);
       
        if(!res) {
            return -1;
        } 
            
        if(cur_trace.format != NULL) {
            switch(cur_trace.event_type) {
                case EVENT_A:
                    printf((char*)cur_trace.format, cur_trace.event_a.a);
                    break;

                case EVENT_B:
                    printf((char*)cur_trace.format, cur_trace.event_b.a, cur_trace.event_b.b);
                    break;
            }
            
        }
        return 0;
    }
    return -1;
}
