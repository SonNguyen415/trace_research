CC = gcc
CFLAGS = -Wall -g -Wno-unused-parameter
SRCS = main.c 
EXEC = tracer
INC_DIR = .
CK_DIR = ./ck

# Rule to build the executable
$(EXEC): $(SRCS) 
	$(CC) $(CFLAGS) -I$(INC_DIR) -L$(CK_DIR) $(SRCS) -o $(EXEC) -lck
	@echo "Number of CPUs: `nproc`"	

# Cleanup rule
clean:
	rm -f $(EXEC)