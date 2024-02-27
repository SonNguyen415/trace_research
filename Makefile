CC = gcc
CFLAGS = -Wall -Wextra -g -Wno-unused-parameter
SRCS = main.c my_trace.c
EXEC = tracer
INC_DIR = .

# Rule to build the executable
$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) -I$(INC_DIR) $(SRCS) -o $(EXEC)

# Cleanup rule
clean:
	rm -f $(EXEC)
