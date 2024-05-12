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
To optimize performance, an idea was conceived to use fall-through switch statements. Apparently, this will make it better. However, as there were no improvement to performance, and the interface is still disgustingly long, it was decided to move on to a better idea.

After this, Gabe conceived the idea of users putting all the arguments into an array whose size is define by a #define. Tests demonstrated that there were little to no difference in performance. However, this allowed a shortening of the arguments to 3, providing for a much nicer interface. A for loop was thus employed to iterate through these arguments. Static inline compiler magic means that the for loop would not be branching.

However, this does means that I'm adding a checker to ensure that the nargs argument is not higher than the NARGS constant that defines the array size so that weird stuff aren't happening.

### Adding timestamps, core id, thread id
After adding these 3 values, the performance overhead increased. At the time, I was confused and used cpuid to get the core id. This clearly resulted in an extremely high overhead. However, this made me suspicious of the limitations of wsl, and so I attemted to run my code on a native linux computer. Thus, my findings are shown below (after adjusting to use RDSTSCP to acquire the core id instead): 

| Test                        | Average Cost (WSL) | Average Cost (Linux Laptop) |
| --------------------------- | ------------------ | ---------------------------------- |
| Single Writer - 1 Argument  |   119-125 cycles   |            80-85 cycles            |
| Single Writer - 4 Arguments |   129-136 cycles   |            86-98 cycles            |
| Single Writer - 8 Arguments |   146-152 cycles   |           100-103 cycles           |
| 4 Writers - 4 Arguments     |   679-724 cycles   |           913-923 cycles           |
| 8 Writers - 4 Arguments     |  2960-3100 cycles  |           1837-1860 cycles         |

As can be observed, the average cost dropped significantly for all cases except for 4 writers with 4 arguments. The average cost for a single writer dropped down to up to 110 cycles, which is around the desired performance. I decided to stop here because the composite functions to acquire these data are different and the performance would be different. Note the limitation of this comparison, as these are still 2 different laptops.f

### Output to csv file
The output to the csv file involves writing the various arguments and data into the csv file. Unfortunately, ck ring does not provide a peeking ability, so one cannot read the buffer without dequeuing. 

In the current implementation, the formatted string for users to display data is not outputted to the csv file. This is because I currently delimit the csv file by comma and so is the format string. Adjustments will be needed to be made to the code to allow users to make sense of what each of the arguments added to the trace entails. I decided to leave it here because I wasn't sure about the design choice here and needed to talk more to Gabe about it. One could certainly request that the user format this string differently, but I don't like to trust the user. 


I was curious on how a concurrent writer may affect the performance of the reader. Applying the test for reading right after writing, we have:

| Test                        | Average Cost (WSL) | Average Cost (WSL while Writing)   |
| --------------------------- | ------------------ | ---------------------------------- |
| Single Writer - 1 Argument  |   119-126 cycles   |          150-165 cycles            |
| Single Writer - 4 Arguments |   129-136 cycles   |          164-173 cycles            |
| Single Writer - 8 Arguments |   146-152 cycles   |          183-209 cycles            |
| 4 Writers - 4 Arguments     |   679-724 cycles   |          716-740 cycles            |
| 8 Writers - 4 Arguments     |  2960-3100 cycles  |         2808-3206 cycles           |

The data showcases that there is significant overhead to the tracers when another thread is reading from the buffer. This is expected, but makes me sad.

### Current State
#### Data

Here's a single table for the final stuff:

| Test                        | Average Cost (WSL) | Average Cost (Linux Laptop) | Average Cost (WSL while Writing)   |
| --------------------------- | ------------------ | ---------------------------------- | ---------------------------------- |
| Single Writer - 1 Argument  |   118-125 cycles   |            80-85 cycles            |          150-165 cycles            |
| Single Writer - 4 Arguments |   129-136 cycles   |            86-98 cycles            |          164-173 cycles            |
| Single Writer - 8 Arguments |   146-152 cycles   |           100-103 cycles           |          183-209 cycles            |
| 4 Writers - 4 Arguments     |   679-724 cycles   |           913-923 cycles           |          716-740 cycles            |
| 8 Writers - 4 Arguments     |  2960-3100 cycles  |          1837-1860 cycles          |         2808-3206 cycles           |


With 0-4 microseconds random wait in between each trace:

| Test                        |    Average Cost    |
| --------------------------- | ------------------ | 
| 4 Writers - 4 Arguments     |   361-390 cycles   | 
| 8 Writers - 4 Arguments     |   380-410 cycles   | 


#### Conclusion
The data shows that we've largely achieved the objective for single writer, but the average cost for a worst case scenario is extremely high. Since this is very unlikely, the average use case of this should be ok. The cost of the tracing seems to reach the targetted point when running on linux when it's inputted with 8 arguments, so I'd blame WSL for the overhead of translating linux system calls onto my windows machine. Indeed, much of the additional cost comes from reading the timestamp (which cost ~50 cycles alone on wsl, and ~25 cycles on linux). As Gabe claimed that obtaining timestamp should be very fast on composite, I decided to not worry too much about that part. 

Although the multi-threading performance is worryingly high, on average it still seems to perform ok. The main additional change of this API from current implementation (the use of the array to allow for dynamic number of arguments) seems to not incur much in additional cost, so therefore this is cool and Gabe should totally use it.
