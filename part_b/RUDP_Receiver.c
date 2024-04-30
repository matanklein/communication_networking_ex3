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
    int BUFFER_SIZE = 1024 * 1024 * 2;

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
        struct _node* _head;
    }StrList;

    StrList* list = NULL;
    DataPoint* current;

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
    char buffer[BUFFER_SIZE];

    // Try to create a TCP socket (IPv4, stream-based, default protocol).
    RUDP_Socket *Server = rudp_socket(true, SERVER_PORT);
    fprintf(stdout, "Listening for incoming connections on port %d...\n", SERVER_PORT);

    if(rudp_accept(Server) == 0){
        perror("rudp_accept()");
        rudp_close(Server);
        return 1;
    }

    printf("Server has a connection!\n");

    int flag_disconnect = 0;
    // The server's main loop.
    while (!flag_disconnect)
    {
        printf("Waiting for a message...\n");

        // Record start time
        gettimeofday(&start, NULL);

        int bytes_total = 0;
        do{
            int bytes_received = 0;
            //printf("1\t");
            // Receive a message from the client and store it in the buffer.
            if(bytes_total + WINDOW_SIZE > BUFFER_SIZE){
                bytes_received = rudp_recv(Server, buffer + bytes_total, BUFFER_SIZE - bytes_total);
            }else{
                bytes_received = rudp_recv(Server, buffer + bytes_total, WINDOW_SIZE);
            }
            // If the message receiving failed, print an error message and return 1.
            if (bytes_received < 0)
            {
                perror("rudp_recv");
                rudp_close(Server);
                return 1;
            }
            
            else if (bytes_received == 0)
            {
                fprintf(stdout, "Client %s:%d disconnected\n", inet_ntoa(Server->dest_addr.sin_addr), ntohs(Server->dest_addr.sin_port));
                flag_disconnect = 1;
                break;
            }

            bytes_total += bytes_received;
        }
        while(bytes_total < BUFFER_SIZE);

        printf("Received %d bytes\n", bytes_total);

        if(flag_disconnect == 1){
            break;
        }

        //add the message to the counter
        counter_message++;

        // Record end time.
        gettimeofday(&end, NULL);

        // Calculate the time until the messege arrived.
        elapsedTimeMs = (end.tv_sec - start.tv_sec) * 1000 + (double)(end.tv_usec - start.tv_usec) / 1000;
        sum_of_time += elapsedTimeMs;

        // calculate the bandwidth.
        band_width = calculateSpeed(BUFFER_SIZE,elapsedTimeMs);
        sum_of_bandwidth += band_width;

        // Store the data in the list.
        DataPoint* p = (DataPoint*)malloc(sizeof(DataPoint));
        if(p == NULL){
            perror("malloc()");
            return 1;
        }

	    p->speed_mb_per_sec = band_width;
	    p->time_ms = elapsedTimeMs;
        p->next = NULL;

        if(list == NULL){
            list = (StrList*)malloc(sizeof(StrList));
            if(list == NULL){
                perror("malloc()");
                return 1;
            }
            list->_head = NULL;
            current = list->_head;
        }
        if(list->_head == NULL){
            list->_head = p;
            current = p;
        }else{
            current->next = p;
            current = p;
        }
    }

    fprintf(stdout, "\nServer finished!\n");

    printf("\n********************************************\n");
    DataPoint* tmp = list->_head;
    int index = 1;
    while(tmp){

        printf("Run #%d: Time = %ldms; Speed = %.3fMB/s\n\n", index, tmp->time_ms, tmp->speed_mb_per_sec);
        index++;                
        tmp = tmp->next;
    }
    double avg_time = sum_of_time / counter_message;
    double avg_bandwidth = sum_of_bandwidth / counter_message;

    printf("\nAverage time: %.3fms\n", avg_time);
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
