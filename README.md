# Trace Research

Array input looking good

## Performance test for array input: 
- Single Writer with 1 argument: 50-60 cycles
- Single Writer with 8 arguments: 66-72 cycles
- 8 Writers with 4 arguments: 2600-3100 cycles
- 8 Writers with 4 arguments and 0-5 microseconds wait in between: ~550 cycles

### Using RDTSCP to get timestamp for the trace
- Single Writer with 1 argument: 105-120 cycles
- Single Writer with 8 arguments: 140-145 cycles
- 8 Writers with 4 arguments: 2700-3200 cycles
- 8 Writers with 4 arguments and 0-5 microseconds wait in between: 450-550 cycles