# Use the gcc compiler.
CC = gcc

# Flags for the compiler.
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic

# Command to remove files.
RM = rm -f

# Phony targets - targets that are not files but commands to be executed by make.
.PHONY: all default clean runuc runus

# Default target - compile everything and create the executables and libraries.
all: RUDP_Receiver RUDP_Sender RUDP_API

# Alias for the default target.
default: all


############
# Programs #
############


# Compile the rudp server.
RUDP_Receiver: RUDP_Receiver.o RUDP_API.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Compile the rudp client.
RUDP_Sender: RUDP_Sender.o RUDP_API.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Compile the rudp API.
RUDP_API.o: RUDP_API.c RUDP_API.h
	$(CC) $(CFLAGS) -c $< -o $@ 

# Create a shared library for the rudp API.
RUDP_API: RUDP_API.o
	$(CC) -shared -o libRUDP_API.so $^ 


################
# Run programs #
################


# Run udp server.
runus: RUDP_Receiver
	./RUDP_Receiver

# Run udp client.
runuc: RUDP_Sender
	./RUDP_Sender


################
# System Trace #
################

# Run the udp server with system trace.
runus_trace: RUDP_Receiver
	strace ./RUDP_Receiver

# Run the udp client with system trace.
runuc_trace: RUDP_Sender
	strace ./RUDP_Sender

################
# Object files #
################

# Compile all the C files into object files.
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


#################
# Cleanup files #
#################

# Remove all the object files, shared libraries and executables.
clean:
	$(RM) *.o *.so RUDP_Receiver RUDP_Sender