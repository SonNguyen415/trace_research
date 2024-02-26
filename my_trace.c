#include "my_trace.h"

struct t_trace_buffer {
    struct trace traces[MAX_EVENTS];
    int read_ptr;
    int write_ptr;
} trace_buffer;

// This is horrible. This is hard coded. I hate it
void trace_event(const char * format, int event_type, int a, int b, int c, \
                        int d, int e, int f, int g, int h, int i, int j) 
{
    // Get the write ptr
    int ptr = trace_buffer.write_ptr;

    // This should be in atomic instruction
    // Shouldn't all of it need to be in atomic instruction? CAS seems inadequate for this
    trace_buffer.traces[ptr].format = format;
    trace_buffer.traces[ptr].event_type = event_type;
    trace_buffer.write_ptr = (ptr + 1) % MAX_EVENTS;
    
    // Add arguments to the trace structure based on event type
    switch(event_type)  {
        case EVENT_A:
            trace_buffer.traces[ptr].event_a.a = a;
            break;

        case EVENT_B:
            trace_buffer.traces[ptr].event_b.a = a;
            trace_buffer.traces[ptr].event_b.b = b;
            break;
    }

}


// Start from current write ptr, we read the entire buffer
void output_trace() 
{
    trace_buffer.read_ptr = trace_buffer.write_ptr;
    int idx = trace_buffer.read_ptr;

    for(int i=0; i < MAX_EVENTS; i++) {
        struct trace cur_trace = trace_buffer.traces[idx];
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
        idx = (idx + 1) % MAX_EVENTS;
    }
}
