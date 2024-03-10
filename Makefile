# Use the gcc compiler.
CC = gcc

# Flags for the compiler.
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic

# Command to remove files.
RM = rm -f

# Phony targets - targets that are not files but commands to be executed by make.
.PHONY: all default clean run_TCP_Receiver runtc runts runuc runus

# Default target - compile everything and create the executables and libraries.
all: TCP_Receiver TCP_Sender RUDP_Receiver RUDP_Sender RUDP_API

# Alias for the default target.
default: all


############
# Programs #
############

# Compile the tcp server.
TCP_Receiver: TCP_Receiver.o
	$(CC) $(CFLAGS) -o $@ $^

# Compile the tcp client.
TCP_Sender: TCP_Sender.o
	$(CC) $(CFLAGS) -o $@ $^

# Compile the udp server.
RUDP_Receiver: RUDP_Receiver.o RUDP_API.o
	$(CC) $(CFLAGS) -o $@ $^

# Compile the udp client.
RUDP_Sender: RUDP_Sender.o RUDP_API.o
	$(CC) $(CFLAGS) -o $@ $^


################
# Run programs #
################

# Run tcp server.
runts: TCP_Receiver
	./TCP_Receiver

# Run tcp client.
runtc: TCP_Sender
	./TCP_Sender

# Run udp server.
runus: RUDP_Receiver
	./RUDP_Receiver

# Run udp client.
runuc: RUDP_Sender
	./RUDP_Sender


################
# System Trace #
################

# Run the tcp server with system trace.
runts_trace: TCP_Receiver
	strace ./TCP_Receiver

# Run the tcp client with system trace.
runtc_trace: TCP_Sender
	strace ./TCP_Sender

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
	$(RM) *.o *.so TCP_Receiver TCP_Sender RUDP_Receiver RUDP_Sender