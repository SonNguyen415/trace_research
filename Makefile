CC = gcc
CFLAGS = -Wall -Wextra -g -Wno-unused-parameter 
SRCS = main.c my_trace.c
EXEC = tracer
INC_DIR = .
CK_DIR = ./ck

# Rule to build the executable
$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) -I$(INC_DIR) -L$(CK_DIR) $(SRCS) -o $(EXEC) -lck
	

# Cleanup rule
clean:
	rm -f $(EXEC)
