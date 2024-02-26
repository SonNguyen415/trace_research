#include "trace_variadic.h"

struct t_trace_buffer {
    struct trace traces[MAX_EVENTS];
    int read_ptr;
    int write_ptr;
} trace_buffer;

// Function to attempt tracing as a variadic function
void trace_event(const char * format, int event_type, int num_args,...) 
{

    // Get the write ptr
    int ptr = trace_buffer.write_ptr;

    // This should be in atomic instruction but this will do for now
    trace_buffer.traces[ptr].format = format;
    trace_buffer.traces[ptr].event_type[0] = event_type;
    trace_buffer.write_ptr = (ptr + 1) % MAX_EVENTS;
    
    switch(event_type)  {
        case EVENT_A:
         // Declare ptr for argument list
            va_list va_ptr;
            va_start(ptr, num_args);
            for(int i=0; i < num_args; i++) {
                va_arg(ptr, int);
            }
            
            trace_buffer.traces[ptr].event_a.a = a;
            break;

        case EVENT_B:
            trace_buffer.traces[ptr].event_b.a = a;
            trace_buffer.traces[ptr].event_b.b = b;
            break;

        case EVENT_c:
            trace_buffer.traces[ptr].event_c.c = c;
            break;
    }
}


// Start from current write ptr, we read the entire buffer
void output_trace() 
{
    trace_buffer.read_ptr = trace_buffer.write_ptr;
    int idx = trace_buffer.read_ptr;

    for(int i=0; i < MAX_EVENTS; i++) {
        if(trace_buffer.traces[idx].format != NULL) {
            printf("%s\n", (char*)trace_buffer.traces[idx].format);
        }
        idx = (idx + 1) % MAX_EVENTS;
    }
}