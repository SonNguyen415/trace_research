#ifndef TRACE_H
#define TRACE_H

#include <string.h>
#include <stdio.h>
#include <ck_ring.h>
#include <pthread.h>
#include <inttypes.h>

// 2^20 buffer size
#define MAX_EVENTS 1048576

#ifndef TRACE_NARGS
#define TRACE_NARGS 1 // Default value
#endif

// Trace structure
struct t_event {
    unsigned long args[TRACE_NARGS];
    
    // Some other variables for timestamp and stuff that all events should store    
    const char * format;
    uint64_t time_stamp;
    uint32_t cpuid;
    pthread_t thd_id;
    int nargs;
} typedef trace_event;


struct t_trace_buffer {
    struct t_event traces[MAX_EVENTS];
    struct ck_ring my_ring;
} trace_buffer;


CK_RING_PROTOTYPE(trace_buffer, t_event);


static inline void trace_init() 
{
    ck_ring_init(&trace_buffer.my_ring, MAX_EVENTS);
}

static inline bool enqueue_trace(const char * format, const int nargs, unsigned long args[]) 
{
    if(TRACE_NARGS < nargs) {
        return false;
    }

    uint32_t low,high,cpuid;
    trace_event new_trace;
    bool ret;
    
    new_trace.format = format;
    asm volatile("rdtscp\n\t" 
                "mov %%ecx, %0\n\t"
                : "=g"(cpuid),"=a"(low), "=d" (high)
                );
    new_trace.time_stamp = ((uint64_t) high << 32 | (uint64_t) low);
    new_trace.cpuid = cpuid;
    new_trace.thd_id = pthread_self();
    new_trace.nargs = nargs;
    
    // Add arguments to the trace structure
    for(int i=0; i<nargs; i++) {
        new_trace.args[i] = args[i];
    }

    // Enqueue into the ring buffer
    ret = CK_RING_ENQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &new_trace);
    
    return ret;
}


static inline trace_event * dequeue_trace() 
{
    trace_event new_trace;
    trace_event * trace_ptr = &new_trace;

    bool ret = CK_RING_DEQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &new_trace);
    if(ret) {
        return trace_ptr;
    }

    return NULL;
}  


static inline bool output_trace(char * file_name, char * mode) 
{
    trace_event cur_trace;
    int num_events, i, j;
    bool ret = false;

    num_events = ck_ring_size(&trace_buffer.my_ring);

    // Open csv file as specified 
    FILE * fp = fopen(file_name, mode);
    if(fp == NULL) {
        perror("Error opening file");
        return ret;
    }

    for(i=0; i < num_events; i++) {
        // Dequeue from buffer
        ret = CK_RING_DEQUEUE_MPSC(trace_buffer, &trace_buffer.my_ring, trace_buffer.traces, &cur_trace);
        if(!ret) {
            return ret;
        } 
        fprintf(fp, "%" PRIu64 ",%" PRIu32 ",%lu, %d", cur_trace.time_stamp, cur_trace.cpuid, cur_trace.thd_id, cur_trace.nargs);
        
        for(j=0; j < cur_trace.nargs; j++) {
            fprintf(fp, ",%lu", cur_trace.args[j]);
        }
        fprintf(fp, "\n");
    }
  
    fclose(fp);
    
    return ret;
}


// Initializer for the ck ring
#define TRACE_INIT() trace_init()


/* Add an event to the trace buffer
 * @param format the string that the data will be written into when dequeued
 * @param nargs the number of arguments to the event, specifying the length of args
 * @param args the array containing the arguments to be inserted into format
 * @return true on success, false otherwise
 */ 
#define ENQUEUE_TRACE(format, nargs, args) \
    enqueue_trace(format, nargs, args)


// Read the buffer, but do not output to file, only dequeue 
// @return a pointer trace_event * to the event that has just been dequeued
#define DEQUEUE_TRACE() dequeue_trace()


/* Read and dequeue the entire trace buffer into a csv, overwriting said CSV
 * CSV file is formatted as Timestamp, Core ID, Thread ID, Number of Arguments, and followed by the arguments.
 * @param file_name the csv file name that we want to write to
 * @return true on success, false otherwise
 */ 
#define OUTPUT_TRACE(file_name) output_trace(file_name, "w")

/* Read and dequeue the entire trace buffer and append it to a csv
 * CSV file is formatted as Timestamp, Core ID, Thread ID, Number of Arguments, and followed by the arguments.
 * @param file_name the csv file name that we want to append to
 * @return true on success, false otherwise
 */ 
#define APPEND_TRACE(file_name) output_trace(file_name, "a")

#endif
