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

// Size of the message.
unsigned int size_of_message = 2 * 1024 * 1024;

/*
 * @brief RUDP Client main function.
 * @param None
 * @return 0 if the client runs successfully, 1 otherwise.
*/
int main(int argc, char *argv[]) {
    RUDP_Socket* client;

    // The variable to store the server's address.
    struct sockaddr_in server;

    FILE *message1;

    // Open a file for writing.
    message1 = fopen("messageRUDP.txt", "w+");

    if(message1 == NULL){
        return 1;
    }

    // Create a message of 2MB to send to the server.
    char *message = util_generate_random_data(size_of_message);

    fwrite(message, 1, size_of_message, message1);

    // Reset the server structure to zeros.
    memset(&server, 0, sizeof(server));

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

    // Convert the server's address from text to binary form and store it in the server structure.
    // This should not fail if the address is valid (e.g. "127.0.0.1").
    if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) <= 0)
    {
        perror("inet_pton(3)");
        rudp_close(client);
        return 1;
    }

    // Set the server's address family to AF_INET (IPv4).
    server.sin_family = AF_INET;

    // Set the server's port to the defined port. Note that the port must be in network byte order,
    // so we first convert it to network byte order using the htons function.
    server.sin_port = htons(SERVER_PORT);

    printf("Connecting to %s:%d...\n", SERVER_IP, SERVER_PORT);

    // Try to connect to the server using the socket and the server structure.
    if (rudp_connect(client, SERVER_IP, SERVER_PORT) == 0)
    {
        perror("rudp_connect()");
        rudp_disconnect(client);
        rudp_close(client);
        return 1;
    }

    int sender_choise = 0;
    printf("if you want to send the file again press '1'.\npress '0' if you dont want to send the file again\n");
    scanf("%d", &sender_choise);
    while(sender_choise == 1){
    
        // Try to send the message to the server using the socket.
        int bytes_sent = rudp_send(client, message1, sizeof(message1));

        // If the message sending failed, print an error message and return 1.
        // If no data was sent, print an error message and return 1. Only occurs if the connection was closed.
        if (bytes_sent < 0)
        {
            perror("rudp_send()");
            rudp_disconnect(client);
            rudp_close(client);
            return 1;
        }

        printf("if you want to send the file again press '1'.\npress '0' if you dont want to send the file again\n");
        scanf("%d", &sender_choise);
    }

    // fprintf(stdout, "Sent %d bytes to the server!\n"
    //                 "Waiting for the server to respond...\n", bytes_sent);

    // Close the socket.
    rudp_disconnect(client);

    // release the memory.
    rudp_close(client);
    
    // Close the file.
    fclose(message1);

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