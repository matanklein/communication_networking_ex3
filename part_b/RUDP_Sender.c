#include <stdio.h> // Standard input/output library
#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h> // For the close function
#include <string.h> // For the memset function
#include <netinet/tcp.h> // For the congestion control
#include <stdlib.h>
#include <time.h>
#include "RUDP_API.h"


#define EXIT_MESSAGE "exit message"

char *util_generate_random_data(unsigned int);

// Size of the message 2MB.
int size_of_message = 2 * 1024 * 1024;

/*
 * @brief RUDP Client main function.
 * @param None
 * @return 0 if the client runs successfully, 1 otherwise.
*/
int main(int argc, char *argv[]) {
    RUDP_Socket* client;

    // Create a message of 2MB to send to the server.
    char *message = util_generate_random_data(size_of_message);

    // Check if the command is correct.
    if(argc != 5){
        perror("command got wrong");
        return 1;
    }
    
    // Gets the server's port and convert in to int from String.
    unsigned short SERVER_PORT = atoi(argv[4]);

    // Gets the server's ip.
    const char *SERVER_IP = argv[2];

    // Try to create an UDP socket and a RUDP struct.
    client = rudp_socket(false, SERVER_PORT);

    // If the RUDP struct creation failed.
    if(client == NULL){
        perror("faild to create a RUDP socket.");
        return 1;
    }

    // If the socket creation failed, print an error message and return 1.
    if (client->socket_fd == -1)
    {
        perror("socket(2)");
        return 1;
    }

    printf("Connecting to %s:%d...\n", SERVER_IP, SERVER_PORT);

    // Try to connect to the server using the socket and the server structure.
    if (rudp_connect(client, SERVER_IP, SERVER_PORT) == 0)
    {
        perror("rudp_connect()");
        rudp_close(client);
        return 1;
    }

    int bytes_total = 0;
    int sender_choise = 1;
    while(sender_choise == 1){
        
        bytes_total = 0;
        
        do{
            // Try to send the message to the server using the socket.
            int bytes_sent = rudp_send(client, message + bytes_total, WINDOW_SIZE);

            // If the message sending failed, print an error message and return 1.
            // If no data was sent, print an error message and return 1. Only occurs if the connection was closed.
            if (bytes_sent < 0)
            {
                perror("rudp_send()");
                rudp_disconnect(client);
                rudp_close(client);
                return 1;
            }
            else if (bytes_sent == 0)
            {
                rudp_disconnect(client);
                rudp_close(client);
                return 1;
            }

            bytes_total += bytes_sent;
        }
        while(bytes_total < size_of_message);

        printf("if you want to send the file again press '1'.\npress '0' if you dont want to send the file again\n");
        scanf("%d", &sender_choise);
    }

    // Close the socket.
    rudp_disconnect(client);

    // release the memory.
    rudp_close(client);

    fprintf(stdout, "Connection closed!\n");

    // Return 0 to indicate that the client ran successfully.
    return 0;
}

/*
* @brief A random data generator function based on srand() and rand().
* @param size The size of the data to generate (up to 2^32 bytes).
* @return A pointer to the buffer.
*/
char *util_generate_random_data(unsigned int size) {
char *buffer = NULL;
// Argument check.
if (size == 0)
return NULL;
buffer = (char *)calloc(size, sizeof(char));
// Error checking.
if (buffer == NULL)
return NULL;
// Randomize the seed of the random number generator.
srand(time(NULL));

for (unsigned int i = 0; i < size; i++)
*(buffer + i) = ((unsigned int)rand() % 256);

return buffer;
}
