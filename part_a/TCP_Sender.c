#include <stdio.h> // Standard input/output library
#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h> // For the close function
#include <string.h> // For the memset function
#include <netinet/tcp.h> // For the congestion control
#include <stdlib.h>
#include <time.h>

#define EXIT_MESSAGE "exit message"

char *util_generate_random_data(unsigned int);

// Size of the message.
int size_of_message = 2 * 1024 * 1024;

/*
 * @brief TCP Client main function.
 * @param None
 * @return 0 if the client runs successfully, 1 otherwise.
*/
int main(int argc, char *argv[]) {

    // The variable to store the socket file descriptor.
    int sock = -1;

    // The variable to store the server's address.
    struct sockaddr_in server;

    // Create a message of 2MB to send to the server.
    char *message = util_generate_random_data((unsigned int)size_of_message);

    // Reset the server structure to zeros.
    memset(&server, 0, sizeof(server));

    // Check if the command is correct.
    if(argc != 7){
        perror("command got wrong");
        return 1;
    }
    
    // Gets the server's port and convert in to int from String.
    int SERVER_PORT = atoi(argv[4]);

    // Gets the server's ip.
    char *SERVER_IP = argv[2];

    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // Set congestion control algorithm (TCP Cubic or TCP Reno)
    const char *congestion_algo = argv[6];
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, congestion_algo, strlen(congestion_algo)) < 0) {
        perror("setsockopt(2)");
        close(sock);
        return 1;
    }

    // Set socket options to disable Keep-Alive
    int optval = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == -1) {
        perror("Setsockopt failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // If the socket creation failed, print an error message and return 1.
    if (sock == -1)
    {
        perror("socket(2)");
        return 1;
    }

    // Convert the server's address from text to binary form and store it in the server structure.
    // This should not fail if the address is valid (e.g. "127.0.0.1").
    if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) <= 0)
    {
        perror("inet_pton(3)");
        close(sock);
        return 1;
    }

    // Set the server's address family to AF_INET (IPv4).
    server.sin_family = AF_INET;

    // Set the server's port to the defined port. Note that the port must be in network byte order,
    // so we first convert it to network byte order using the htons function.
    server.sin_port = htons(SERVER_PORT);

    fprintf(stdout, "Connecting to %s:%d...\n", SERVER_IP, SERVER_PORT);

    // Try to connect to the server using the socket and the server structure.
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("connect(2)");
        close(sock);
        return 1;
    }

    int sender_choise = 0;
    do{

        int bytes_sent = 0;
        while(bytes_sent < size_of_message){

            int count = send(sock, message + bytes_sent, size_of_message - bytes_sent, 0);
            if (bytes_sent < 0)
            {
                break;
            }
            bytes_sent += count;
        }

        printf("if you want to send the file again press '1'.\npress '0' if you dont want to send the file again\n");
        scanf("%d", &sender_choise);
    }
    while(sender_choise == 1);

    //sleep(1);

    int bytes_sent = send(sock, EXIT_MESSAGE, strlen(EXIT_MESSAGE) + 1, 0);

    // If the exit message sending failed, print an error message and return 1.
    // If no data was sent, print an error message and return 1. Only occurs if the connection was closed.
    if (bytes_sent <= 0)
    {
        perror("send(2)");
        close(sock);
        return 1;
    }

    // Close the socket with the server.
    close(sock);

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