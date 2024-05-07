## Spring 2024 Research Report
The goal of this project was to create a new tracing infrastructure that was more dynamic than the one currently used by the Composite Operating System while maintaining low overhead. Thus, the intention is to create a macro that will allow users to call right before any event to log them into a ring buffer. Events can then be outputted into a file to be analyzed later. The stated objetive is to ensure that the aforementioned logger would run at less than 100 cycles.


###  Union of Structs
I initially looked at the ftrace documentations as a reference, but it was much more complex than what we wanted. The strace likewise was more complex than needed, with the desired objective to be a tracer that we can explicitly call in the program, rather than a command to be inputted into the shell.

The first thing I did was, through discussion with Professor Gabe Parmer, create a ring buffer that was an array of events, each event being a union of different event types. This is done so that users can input in different number of arguments, with each event type being simply a struct with different number of arguments. The user can then enqueue into the buffer while specifying the event type as an argument to the tracing function, which will be a predefined flag to the function. 

As a result of this way of doing things, the interface was extremely complex with up to 12 arguments, 2 for the string format to be outputted and the event type flag, and 10 for the arguments, with each argument that's not needed still requires the user to input a value. As a result, I decided to experiment on utilizing it as a variadic function. There was concern about the possible drop in performance and the fact that when the user have lots of arguments the function call would still have a crap ton of arguments. This idea was never seriously pursued, as a nicer idea was conceived later.

During this time, I was concerned also with the concurrency kit. Since the kit can only enqueue and dequeue into the ring buffer, and we might want to add in data, it means that new data will have to be dropped when the ring buffer is full before the user dequeue from the ring buffer. I entertainted the idea of adding an additional check that will dequeue the buffer when the ring is full, but doing so would cost so much in performance when they want to enqueue that I deemed it useless and the user might as well just explicitly dequeue the buffer. Another idea I had was to just halt the process and drop a warning to the user to give the option of outputting all current data, but I decided that it'll be a problem for posterity.

### Testing
Much of the tests for correctness was trivial so I won't go into details. I spent much of the time on the performance test. I've decided to discard the acquisition of timestamp, thread id, and core id in my initial analysis as the functions to acquire them is different on Composite, which is the final desired operating system we want to incorporate this in. RDTSCP was used to acquire the timestamps. The tests were ran for 4096 trials, each trial have a writer enqueue 1024 times. The average was taken to compute the final result for each test. The result for such tests is shown below:


| Test                        |    Average Cost    |
| --------------------------- | ------------------ | 
| Single Writer - 1 Argument  |    54-60 cycles    | 
| Single Writer - 4 Arguments |    59-62 cycles    | 
| Single Writer - 8 Arguments |    67-74 cycles    | 
| 4 Writers - 4 Arguments     |   590-620 cycles   | 
| 8 Writers - 4 Arguments     |  2700-2850 cycles  | 


At this time, I was concerned with the extremely high performance cost of the multiple writers. Discussions with Gabe revealed that it was expected to be high due to high contention and this was the absolute worst case. I decided to implement a randomized wait in between 0 and 4 microseconds between each enqueue for a simple test. The result is shown below:


| Test                        |    Average Cost    |
| --------------------------- | ------------------ | 
| 4 Writers - 4 Arguments     |   361-390 cycles   | 
| 8 Writers - 4 Arguments     |   380-410 cycles   | 

Interestingly, the difference between 4 and 8 writers dropped significantly here.

### Using array as input
To optimize performance, an idea was conceived to use fall-through switch statements. However, testing showed that there was little to no increase in performance. As such, this idea was dropped.

After this, Gabe conceived the idea of users putting all the arguments into an array whose size is define by a #define. Testing this out found that there were little to no difference in performance. This allowed a shortening of the arguments to 3, providing for a much nicer interface. A for loop was thus employed to iterate through these arguments.

### Adding timestamps, core id, thread id
After adding these 3 values, the performance overhead increased. At the time, I was confused and used cpuid to get the core id. This clearly resulted in an extremely high overhead. However, this made me suspicious of the limitations of wsl, and so I attemted to run my code on a native linux computer. Thus, my findings are shown below (after adjusting to use RDSTSCP to acquire the core id instead): 

| Test                        | Average Cost (WSL) | Average Cost (Ubuntu Linux Laptop) |
| --------------------------- | ------------------ | ---------------------------------- |
| Single Writer - 1 Argument  |   123-137 cycles   |             80-85 cycles           |
| Single Writer - 4 Arguments |   150-165 cycles   |             86-88 cycles           |
| Single Writer - 8 Arguments |   208-224 cycles   |            100-103 cycles          |
| 4 Writers - 4 Arguments     |   669-697 cycles   |            913-923 cycles          |
| 8 Writers - 4 Arguments     |  2915-3080 cycles  |            1837-1860 cycles        |

As can be observed, the average cost dropped significantly for all cases except for 4 writers with 4 arguments. The average cost for a single writer dropped down to up to 110 cycles, which is around the desired performance. I decided to stop here because the composite functions to acquire these data are different and the performance would be different. Note the limitation of this comparison, as these are still 2 different laptops.

### Output to csv file
The output to the csv file involves writing the various arguments and data into the csv file. Unfortunately, ck ring does not provide a peeking ability, so one cannot read the buffer without dequeuing. In any case, the data was added in. I was curious on how a concurrent writer may affect the performance of the reader. Applying the test for reading right after writing, we have:

| Test                        | Average Cost (WSL) | Average Cost (WSL while Writing)   |
| --------------------------- | ------------------ | ---------------------------------- |
| Single Writer - 1 Argument  |   123-137 cycles   |             151 cycles             |
| Single Writer - 4 Arguments |   150-165 cycles   |             86-88 cycles           |
| Single Writer - 8 Arguments |   208-224 cycles   |            100-103 cycles          |
| 4 Writers - 4 Arguments     |   669-697 cycles   |            913-923 cycles          |
| 8 Writers - 4 Arguments     |  2915-3080 cycles  |            1837-1860 cycles        |


### Current State
#### Current State

#### Data

#### Limitations

### Conclusion