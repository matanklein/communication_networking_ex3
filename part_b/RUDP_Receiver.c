#include <stdio.h> // Standard input/output library
#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h> // For the close function
#include <string.h> // For the memset function
#include <netinet/tcp.h> // For the congestion control
#include <stdlib.h>
#include <sys/time.h>
#include "RUDP_API.h"

/*
 * @brief The maximum number of clients that the server can handle.
 * @note The default maximum number of clients is 1.
*/
#define MAX_CLIENTS 1

#define EXIT_MESSAGE "exit message"

double calculateSpeed(int , long);

/*
 * @brief TCP Server main function.
 * @param None
 * @return 0 if the server runs successfully, 1 otherwise.
*/
int main(int argc, char *argv[]) {
    // Make the buffer size 2MB
    size_t BUFFER_SIZE = 1024 * 1024 * 2;

    // The variable to store the socket file descriptor.
    //int sock = -1;

    // The variable to store the server's address.
    //struct sockaddr_in server;

    // Count the number of messeges.
    int counter_message = 0;

    double sum_of_time = 0;
    double sum_of_bandwidth = 0;

    double band_width;

    // Make list to save the messages.
    typedef struct _node{
        long time_ms;
        double speed_mb_per_sec;
        struct _node* next;
    } DataPoint;


    typedef struct _StrList {
        DataPoint* _head;
    }StrList;

    StrList* list = (StrList*)malloc(sizeof(StrList));
    if(list == NULL){
        return 1;
    }
    list->_head = NULL;
    DataPoint* curr;

    // Save time.
    struct timeval start, end;
    double elapsedTimeMs;

    // check if the command is correct
    if(argc != 3){
        perror("command got wrong");
        return 1;
    }

    // Gets the server's port and convert in to int from String.
    int SERVER_PORT = atoi(argv[2]);

    // Create a buffer to store the received message.
    char *buffer = (char*)malloc(BUFFER_SIZE);

    if(buffer == NULL){
        return 1;
    }

    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    RUDP_Socket *Server = rudp_socket(true, SERVER_PORT);
    fprintf(stdout, "Listening for incoming connections on port %d...\n", SERVER_PORT);

    //printf("Server is running...\n");

    // Set the server's address to "0.0.0.0" (all IP addresses on the local machine).
    //server.sin_addr.s_addr = INADDR_ANY;

    // Set the server's address family to AF_INET (IPv4).
    //server.sin_family = AF_INET;

    // Set the server's port to the specified port. Note that the port must be in network byte order.
    //server.sin_port = htons(SERVER_PORT);

    if(rudp_accept(Server) == 0){
        perror("rudp_accept()");
        rudp_close(Server);
        return 1;
    }

    printf("Server has a connection!\n");

    // Print a message to the standard output to indicate that a new client has connected.
    //fprintf(stdout, "Client %s:%d connected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    // The server's main loop.
    while (1)
    {

        // Record start time
        gettimeofday(&start, NULL);
       
            // Receive a message from the client and store it in the buffer.
            int bytes_received = rudp_recv(Server, buffer, BUFFER_SIZE);
            printf("bytes received: %d\n", bytes_received);
            // If the message receiving failed, print an error message and return 1.
            if (bytes_received < 0)
            {
                perror("rudp_recv");
                rudp_close(Server);
                return 1;
            }

            // If the amount of received bytes is 0, the client has disconnected.
            // Close the client's socket and continue to the next iteration.
            else if (bytes_received == 0)
            {
                fprintf(stdout, "Client %s:%d disconnected\n", inet_ntoa(Server->dest_addr.sin_addr), ntohs(Server->dest_addr.sin_port));
                //rudp_disconnect(Server);
                break;
            }

        //add the message to the counter
        counter_message++;

        // Record end time.
        gettimeofday(&end, NULL);

        // Ensure that the buffer is null-terminated, no matter what message was received.
        // This is important to avoid SEGFAULTs when printing the buffer.
        buffer[BUFFER_SIZE- 1] = '\0';

        // Calculate the time until the messege arrived.
        elapsedTimeMs = (end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_usec - start.tv_usec) / 1000;
        sum_of_time += elapsedTimeMs;

        // calculate the bandwidth.
        band_width = calculateSpeed(BUFFER_SIZE,elapsedTimeMs);
        sum_of_bandwidth += band_width;

        //printf("bandwidth: %f\n", band_width);
        //printf("byte recived is %d counter: %d\n", bytes_received,counter_message);
        
        // Store the data in the list.
        DataPoint* p = (DataPoint*)malloc(sizeof(DataPoint));
        if(p == NULL){
            return 1;
        }

	    p->speed_mb_per_sec = band_width;
	    p->time_ms = elapsedTimeMs;

        if(list->_head == NULL){
            list->_head = p;
            curr = p;
        }else{
            curr->next = p;
            curr = p;
        }
        
    }

    fprintf(stdout, "\nServer finished!\n");

    printf("********************************************\n");
                DataPoint* tmp = list->_head;
                int index = 1;
                while(tmp){
                    //if(tmp->time_ms >= 50){
                    printf("Run #%d: Time = %ldms; Speed = %.3fMB/s\n\n", index, tmp->time_ms, tmp->speed_mb_per_sec);
                    index++;
                    
                    
                    tmp = tmp->next;
                }
                double avg_time = sum_of_time / counter_message;
                double avg_bandwidth = sum_of_bandwidth / counter_message;

                printf("\n");
                printf("Average time: %.3fms\n\n", avg_time);
                printf("Average bandwidth: %.3fMB/s\n", avg_bandwidth);
                printf("********************************************\n");

            rudp_close(Server);

            if(list == NULL){
                return 0;
            }
            DataPoint* cur = list->_head;
            while (cur != NULL)
            {
                DataPoint* next = cur->next;
                free(cur);
                cur = next;
            }
            free(list);
            
            return 0;

}

double calculateSpeed(int fileSizeBytes, long elapsedTimeMs) {

    //printf("the size is: %d\n", fileSizeBytes);
    if(elapsedTimeMs == 0){return 0;}
    return (fileSizeBytes / (1024.0 )/( 1024.0)) / (elapsedTimeMs / 1000.0);
}