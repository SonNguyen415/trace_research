#ifndef MY_TRACE_H
#define MY_TRACE_H

#include <string.h>
#include <stdio.h>

#define MAX_EVENTS 100
#define MAX_ARGS 10


// Define event types
#define EVENT_A 0
#define EVENT_B 1


// We define some event types that the user can trace
// Question: what other characteristics differentiate events beside mere arguments?
struct t_event_a {
    int a;
};

struct t_event_b {
    int a, b;
};


// Tracer structure, storing arguments as variables
struct trace {
    union {
        struct t_event_a event_a;
        struct t_event_b event_b;
    };
    
    // Some other variables for timestamp and stuff that all events should store
    const char * format;
    int event_type;
};



// Function to add an event to the trace infrastructure
// This is horrible. This is hard coded. I hate it
void trace_event(const char * format, int event_type, int a, int b, int c, \
                        int d, int e, int f, int g, int h, int i, int j);


// Function to output trace buffer
void output_trace();

#endif