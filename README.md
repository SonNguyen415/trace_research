# Trace Research

Array input looking good

## Performance test for array input: 
- Single Writer with 1 argument: 50-60 cycles
- Single Writer with 8 arguments: 66-72 cycles
- 8 Writers with 4 arguments: 2600-3100 cycles
- 8 Writers with 4 arguments and 0-5 microseconds wait in between: ~550 cycles

### Using RDTSC to get timestamp for the trace
- Single Writer with 1 argument: 85-90 cycles
- Single Writer with 8 arguments: 100-110 cycles
- 8 Writers with 4 arguments: 2700-3200 cycles
- 8 Writers with 4 arguments and 0-5 microseconds wait in between: 450-550 cycles


1. I did smth potentially dumb 
2. Unsigned long yes?
3. What data needs to be in
4. What should output look like


1. Timestamp
2. CPUID
3. Thread ID

Look at object file
