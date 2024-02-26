#ifndef TRACE_VARIADIC_H
#define TRACE_VARIADIC_H

// Abandoned idea for now because I then have no control over the size of the arguments
// Could use the arguments as a union

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define MAX_EVENTS 100
#define MAX_ARGS 10



// Define event types
#define EVENT_A 0
#define EVENT_B 1
#define EVENT_C 2


// We define some event types that the user can 
struct t_event_a {
    int a;
};

struct t_event_b {
    int a, b;
};

struct t_event_c {
    char * c;
};

// Tracer structure
struct trace {
    long cpu;
    // Some other variables for timestamp and shared variables

    // We store the format and the event type
    const char * format;
    int event_type;

    union {
        struct t_event_a event_a;
        struct t_event_b event_b;
        struct t_event_c event_c;
    };
};

// Function to add an event to the trace infrastructure
void trace_event(const char * format, int event_type, int num_args,...);

// Function to output trace buffer
void output_trace_variadic();

#endif