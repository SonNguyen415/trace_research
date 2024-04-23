CC = gcc
CFLAGS = -Wall -g -Wno-unused-parameter -pthread
SRCS = main.c tracer.h
EXEC = tracer_cpuid
INC_DIR = .
CK_DIR = ./ck

# Rule to build the executable
$(EXEC): $(SRCS) 
	$(CC) $(CFLAGS) -I$(INC_DIR) -L$(CK_DIR) $(SRCS) -o $(EXEC) -lck

# Cleanup rule
clean:
	rm -f $(EXEC)

main.c: tracer.h