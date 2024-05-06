## Spring 2024 Research Report
The goal of this project was to create a new tracing infrastructure that was more dynamic than the one currently used by the Composite Operating System while maintaining low overhead. Thus, the intention is to create a macro that will allow users to call right before any event to log them into a ring buffer. Events can then be outputted into a file to be analyzed later. The stated objetive is to ensure that the aforementioned logger would run at less than 100 cycles.


####  Union of Structs
I initially looked at the ftrace documentations as a reference, but it was much more complex than what we wanted. The strace likewise was more complex than needed, with the desired objective to be a tracer that we can explicitly call in the program, rather than a command to be inputted into the shell.

The first thing I did was, through discussion with Professor Gabe Parmer, create a ring buffer that was an array of events, each event being a union of different event types. This is done so that users can input in different number of arguments, with each event type being simply a struct with different number of arguments. The user can then enqueue into the buffer while specifying the event type as an argument to the tracing function, which will be a predefined flag to the function. 

As a result of this way of doing things, the interface was extremely complex with up to 12 arguments, 2 for the string format to be outputted and the event type flag, and 10 for the arguments, with each argument that's not needed still requires the user to input a value. As a result, I decided to experiment on utilizing it as a variadic function. There was concern about the possible drop in performance and the fact that when the user have lots of arguments the function call would still have a crap ton of arguments. This idea was never seriously pursued, as a nicer idea was conceived later.

During this time, I was concerned also with the concurrency kit. Since the kit can only enqueue and dequeue into the ring buffer, and we might want to add in data, it means that new data will have to be dropped when the ring buffer is full before the user dequeue from the ring buffer. I considered adding an additional check that will dequeue the buffer when the ring is full, but doing so would cost so much in performance that I deemed it useless and the user might as well just explicitly call output. Another idea I had was to just halt the process and drop a warning to the user to give the option of outputting all current data, but this was never pursued.

#### Testing
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

#### Using array as input
To optimize performance, an idea was conceived to use fall-through switch statements. However, testing showed that there was little to no increase in performance. As such, this idea was dropped.

After this, Gabe conceived the idea of users putting all the arguments into an array whose size is define by a #define. Testing this out found that there were little to no difference in performance. This allowed a shortening of the arguments to 3, providing for a much nicer interface. A for loop was thus employed to iterate through these arguments.

#### Adding timestamps, core id, thread id
After adding these 3 values, the performance overhead increased:


| Test                        |    Average Cost    |
| --------------------------- | ------------------ | 
| Single Writer - 1 Argument  |   123-137 cycles   | 
| Single Writer - 4 Arguments |   150-165 cycles   | 
| Single Writer - 8 Arguments |   208-224 cycles   | 
| 4 Writers - 4 Arguments     |   669-697 cycles   | 
| 8 Writers - 4 Arguments     |  2915-3080 cycles  | 

This is clearly unacceptable and too high, but these are values 

#### Output to csv file




#### Current State
###### Current State

###### Data

###### Limitations

#### Conclusion