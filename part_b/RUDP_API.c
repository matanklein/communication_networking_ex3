#include "RUDP_API.h"
#include <stdio.h> // Standard input/output library
#include <arpa/inet.h> // For the in_addr structure and the inet_pton function
#include <sys/socket.h> // For the socket function
#include <unistd.h> // For the close function
#include <string.h> // For the memset function
#include <netinet/tcp.h> // For the congestion control
#include <stdlib.h>
#include <time.h>
#include <sys/time.h> // For tv struct
#include <errno.h>
#include <math.h>

/*
flag = 0 : default
flag = 1 : SYN
flag = 2 : FIN
flag = 3 : ACK
*/

RUDP_Socket* rudp_socket(bool isServer, unsigned short int listen_port){
    struct timeval tv = { 5, 0 };

    int sock = -1;
    RUDP_Socket *rudp_struct = (RUDP_Socket*)malloc(sizeof(RUDP_Socket));

    if(rudp_struct == NULL){
        perror("allocate problem");
        exit(1);
    }

    // Try to create a UDP socket (IPv4, datagram-based, default protocol).
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    // If the socket creation failed, print an error message and exit.
    if (sock == -1)
    {
        perror("socket(2)");
        exit(1);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) == -1)
    {
        perror("setsockopt(2)");
        close(sock);
        exit(1);
    }

    //printf("Socket created successfully.\n");

    rudp_struct->isServer = isServer;
    rudp_struct->isConnected = false;
    rudp_struct->socket_fd = sock;

    if(isServer){
        // The variable to store the server's address.
        struct sockaddr_in server;

        // Set the server's address to "0.0.0.0" (all IP addresses on the local machine).
        server.sin_addr.s_addr = INADDR_ANY;

        // Set the server's address family to AF_INET (IPv4).
        server.sin_family = AF_INET;

        // Set the server's port to the specified port. Note that the port must be in network byte order.
        server.sin_port = htons(listen_port);

        // Try to bind the socket to the server's address and port.
        if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            perror("bind(2)");
            close(sock);
            exit(1);
        }
        //printf("Socket binded successfully.\n");
    }
    return rudp_struct;
}

int rudp_connect(RUDP_Socket *sockfd, const char *dest_ip, unsigned short int dest_port){

    if(sockfd == NULL){
        return 0;
    }

    if(sockfd->isConnected == true){
        return 0;
    }

    if(sockfd->isServer == true){
        return 0;
    }

    // Reset the server structure to zeros.
    memset(&sockfd->dest_addr, 0, sizeof(sockfd->dest_addr));

    // Set the server's address family to AF_INET (IPv4).
    sockfd->dest_addr.sin_family = AF_INET;

    // Set the server's port to the defined port. Note that the port must be in network byte order,
    // so we first convert it to network byte order using the htons function.
    sockfd->dest_addr.sin_port = htons(dest_port);

    // Convert the server's address from text to binary form and store it in the server structure.
    // This should not fail if the address is valid (e.g. "127.0.0.1").
    if (inet_pton(AF_INET, dest_ip, &sockfd->dest_addr.sin_addr) <= 0)
    {
        perror("inet_pton(3)");
        close(sockfd->socket_fd);
        return 0;
    }

    sockfd->ack_num = 0;
    sockfd->seq_num = 0;
    sockfd->flags = 1; // A SYN flag.

    //sockfd->checksum = calculate_checksum((void*)sockfd, sizeof(*sockfd));

    //printf("checksum:%d\n", sockfd->checksum);

    socklen_t adressLen = sizeof(sockfd->dest_addr);
    int bytes_sent = sendto(sockfd->socket_fd, sockfd, sizeof(*sockfd), 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);

    // If the message sending failed, print an error message and return 1.
    // If no data was sent, print an error message and return 1. In UDP, this should not happen unless the message is empty.
    if (bytes_sent <= 0)
    {
        perror("sendto(2)");
        close(sockfd->socket_fd);
        return 0;
    }

    //printf("Socket send successfully.\n");

    sockfd->seq_num += sizeof(sockfd);

    RUDP_Socket *receivem = malloc(sizeof(RUDP_Socket));
    memset(receivem, 0, sizeof(RUDP_Socket));

    int  flag = 1;
    //printf("send message\n");
    while(flag){
        //printf("entering the loop\n");
        flag = 0;

        // Try to receive a ack from the server using the socket.
        socklen_t adressLen = sizeof(sockfd->dest_addr);
        int bytes_received = recvfrom(sockfd->socket_fd, receivem, sizeof(*receivem), 0, (struct sockaddr *)&sockfd->dest_addr, &adressLen);
        // If the message receiving failed, print an error message and return 0.
        // If no data was received, print an error message and return 0.
        if (bytes_received <= 0)
        {
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    flag = 1;
                    continue;
                }else{
                    perror("recvfrom(2)");
                    close(sockfd->socket_fd);
                    free(receivem);
                    return 0;
                }
            }
            perror("recvfrom(2)");
            close(sockfd->socket_fd);
            free(receivem);
            return 0;
        }
    }
    //printf("receive message\n");

    // if(calculate_checksum(receivem, sizeof(*receivem)) != receivem->checksum){
    //     perror("checksum error");
    //     close(sockfd->socket_fd);
    //     free(receivem);
    //     return 0;
    // }

    // sockfd->checksum = calculate_checksum(receivem, sizeof(*receivem));
    sockfd->ack_num = receivem->seq_num + sizeof(receivem);
    sockfd->seq_num = receivem->ack_num;
    sockfd->flags = 0; // A SYN flag set to false.

    free(receivem);

    /*bytes_sent = sendto(sockfd->socket_fd, sockfd, sizeof(*sockfd), 0, (struct sockaddr *)&sockfd->dest_addr, sizeof(sockfd->dest_addr));

    // If the message sending failed, print an error message and return 1.
    // If no data was sent, print an error message and return 1. In UDP, this should not happen unless the message is empty.
    if (bytes_sent <= 0)
    {
        perror("sendto(2)");
        close(sockfd->socket_fd);
        return 0;
    }
    printf("send message\n");*/
    sockfd->isConnected = true;
    return 1;

}

int rudp_accept(RUDP_Socket *sockfd){

    if(sockfd == NULL){
        return 0;
    }

    if(sockfd->isConnected == true){
        return 0;
    }

    if(sockfd->isServer == false){
        return 0;
    }

    sockfd->ack_num = 0;
    sockfd->seq_num = 0;
    sockfd->flags = 1; // A SYN flag.

    RUDP_Socket *receivem = malloc(sizeof(RUDP_Socket));
    memset(receivem, 0, sizeof(RUDP_Socket));

    socklen_t adressLen = sizeof(sockfd->dest_addr);

    int flag_timeout = 1;
    while(flag_timeout){
        flag_timeout = 0;

        // Try to receive a ack from the server using the socket.
        int bytes_received = recvfrom(sockfd->socket_fd, receivem, sizeof(RUDP_Socket), 0, (struct sockaddr *)&sockfd->dest_addr, &adressLen);
        //printf("flag:%d\n", receivem->flags);

        // If the message receiving failed, print an error message and return 0.
        // If no data was received, print an error message and return 0.
        if (bytes_received <= 0)
        {
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    flag_timeout = 1;
                    continue;
                }else{
                    //printf("1\n");
                    perror("recvfrom(2)");
                    close(sockfd->socket_fd);
                    free(receivem);
                    return 0;
                }
            }
            //printf("2\n");
            perror("recvfrom(2)");
            close(sockfd->socket_fd);
            free(receivem);
            return 0;  
        }
    }
    //printf("receive message\n");

    // unsigned short result  = calculate_checksum((void*)receivem, sizeof(*receivem));
    // if(result != receivem->checksum){
    //     printf("client is:%d\n", receivem->checksum);
    //     printf("server is:%d\n", result);
    //     perror("checksum error");
    //     close(sockfd->socket_fd);
    //     free(receivem);
    //     return 0;
    // }
    //printf("3\n");

    sockfd->ack_num = receivem->seq_num + sizeof(receivem);
    sockfd->seq_num = receivem->ack_num;

    free(receivem);

    // Now the server will send the ack + syn.
    /*
    char *dest_ip = inet_ntoa(receivem->dest_addr.sin_addr);
    unsigned short int dest_port = sockfd->dest_addr.sin_port;
    
    // Reset the server structure to zeros.
    memset(&sockfd->dest_addr, 0, sizeof(sockfd->dest_addr));

    // Set the server's address family to AF_INET (IPv4).
    sockfd->dest_addr.sin_family = AF_INET;

    // Set the server's port to the defined port. Note that the port must be in network byte order,
    // so we first convert it to network byte order using the htons function.
    sockfd->dest_addr.sin_port = htons(dest_port);

    // Convert the server's address from text to binary form and store it in the server structure.
    // This should not fail if the address is valid (e.g. "127.0.0.1").
    if (inet_pton(AF_INET, dest_ip, &sockfd->dest_addr.sin_addr) <= 0)
    {
        perror("inet_pton(3)");
        close(sockfd->socket_fd);
        return 0;
    }*/

    //sockfd->checksum = calculate_checksum((void*)receivem, sizeof(*receivem));

    int bytes_sent = sendto(sockfd->socket_fd, sockfd, sizeof(*sockfd), 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);

    // If the message sending failed, print an error message and return 1.
    // If no data was sent, print an error message and return 1. In UDP, this should not happen unless the message is empty.
    if (bytes_sent <= 0)
    {
        perror("sendto(2)");
        close(sockfd->socket_fd);
        return 0;
    }
    //printf("send message\n");
    sockfd->seq_num += sizeof(sockfd);
    sockfd->flags = 0; // A SYN flag set to false.

    //now the server will get the last hand shake.
    /*
    int flag_timeout2 = 1;
    while(flag_timeout2){
        flag_timeout2 = 0;

        // Try to receive a ack from the server using the socket.
        int bytes_received = recvfrom(sockfd->socket_fd, receivem, sizeof(*receivem), 0, (struct sockaddr *)&sockfd->dest_addr, (socklen_t *)sizeof(sockfd->dest_addr));

        // If the message receiving failed, print an error message and return 0.
        // If no data was received, print an error message and return 0.
        if (bytes_received <= 0)
        {
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    flag_timeout2 = 1;
                    continue;
                }else{
                    perror("recvfrom(2)");
                    close(sockfd->socket_fd);
                    return 0;
                }
            }
            perror("recvfrom(2)");
            close(sockfd->socket_fd);
            return 0;
        }
    }

    printf("receive message\n");

    // if(calculate_checksum((void*)receivem, sizeof(*receivem)) != receivem->checksum){
    //     perror("checksum error");
    //     close(sockfd->socket_fd);
    //     return 0;
    // }
    */

    sockfd->ack_num = receivem->seq_num + sizeof(receivem);
    sockfd->seq_num = receivem->ack_num;

    sockfd->isConnected = true;
    return 1;
}

int rudp_recv(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){
    if(sockfd == NULL){
        return -1;
    }

    if(sockfd->isConnected == false){
        return -1;
    }

    if(buffer_size <= 0){
        return -1;
    }

    void *temp = buffer;

    unsigned int bytes_total = 0;
    int bytes_received;
    char packet[WINDOW_SIZE] = {0};
    socklen_t adressLen = sizeof(sockfd->dest_addr);
    RUDP_Socket *receivem = malloc(sizeof(RUDP_Socket));
    do{

        bytes_received = recvfrom(sockfd->socket_fd, packet, WINDOW_SIZE, 0, (struct sockaddr *)&sockfd->dest_addr, &adressLen);
        //printf("bytes_received:%d\t", bytes_received);
        if(bytes_received <= 0){
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    continue;
                }else{
                    perror("recvfrom(2)");
                    //close(sockfd->socket_fd);
                    free(receivem);
                    return -1;
                }
            }
            perror("recvfrom(2)");
            //close(sockfd->socket_fd);
            free(receivem);
            return -1;
        }

        memcpy(receivem, packet, sizeof(RUDP_Socket));
        memcpy(temp, packet + sizeof(RUDP_Socket), bytes_received - sizeof(RUDP_Socket));
        bytes_total += bytes_received;
        sockfd->ack_num = receivem->seq_num + bytes_received;
        sockfd->seq_num = receivem->ack_num;
        printf("hallllooo\n");
        unsigned short result  = calculate_checksum(buffer, bytes_received - sizeof(RUDP_Socket));
        if(result != receivem->checksum){
            printf("client is:%d\n", receivem->checksum);
            printf("server is:%d\n", result);
            perror("checksum error");
            //close(sockfd->socket_fd);
            free(receivem);
            return -1;
        }
        printf("confused\n");
        temp = (char*)temp + bytes_received - sizeof(RUDP_Socket);

        if(receivem->flags == 2){
            close(sockfd->socket_fd);
            free(receivem);
            return 0;
        }
    

        sockfd->flags = 1; // A ACK flag.

        //sockfd->checksum = calculate_checksum(sockfd, sizeof(*sockfd));
        int bytes_sent = sendto(sockfd->socket_fd, sockfd, sizeof(*sockfd), 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);
        if (bytes_sent <= 0)
        {
            perror("sendto(2)");
            //close(sockfd->socket_fd);
            free(receivem);
            return -1;
        }

        sockfd->seq_num = receivem->ack_num;
        sockfd->ack_num = receivem->seq_num + sizeof(receivem);
        sockfd->flags = 0; // A ACK flag set to false.
    }

    while(bytes_total < buffer_size);
    
    free(receivem);

    return bytes_total;
}

int rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){ 

    char packet[WINDOW_SIZE] = {0};
    int num_of_packets = ceil((buffer_size + sizeof(RUDP_Socket)) / (double)WINDOW_SIZE);
    int count = 0;
    void *temp = buffer;

    if(sockfd == NULL){
        return -1;
    }

    if(sockfd->isConnected == false){
        return -1;
    }

    if(buffer_size <= 0){
        return -1;
    }

    int ans = 0;
    int bytes_sent;
    RUDP_Socket *receivem = malloc(sizeof(RUDP_Socket));
    socklen_t adressLen = sizeof(sockfd->dest_addr);
    do{
        //printf("count:%d\n", count);

        //transfer the data to the packet.
        memcpy(packet + sizeof(RUDP_Socket), (char*)temp + count*(WINDOW_SIZE - sizeof(RUDP_Socket)), WINDOW_SIZE - sizeof(RUDP_Socket));

        //fill the header of the packet.
        sockfd->checksum = calculate_checksum((char*)temp + count*(WINDOW_SIZE - sizeof(RUDP_Socket)), WINDOW_SIZE - sizeof(RUDP_Socket));

        //transfer the header to the packet.
        memcpy(packet, sockfd, sizeof(RUDP_Socket));

        bytes_sent = sendto(sockfd->socket_fd, packet, WINDOW_SIZE, 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);
        if (bytes_sent <= 0)
        {
            perror("sendto(2)");
            free(receivem);
            //close(sockfd->socket_fd);
            return -1;
        }

        int bytes_received = recvfrom(sockfd->socket_fd, receivem, sizeof(*receivem), 0, (struct sockaddr *)&sockfd->dest_addr, &adressLen);
        if(bytes_received <= 0){
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    continue;
                }else{
                    perror("recvfrom(2)");
                    free(receivem);
                    //close(sockfd->socket_fd);
                    return -1;
                }
            }
            perror("recvfrom(2)");
            free(receivem);
            //close(sockfd->socket_fd);
            return -1;
        }

        // if(calculate_checksum(receivem, sizeof(*receivem)) != receivem->checksum){
        //     perror("checksum error");
        //     //close(sockfd->socket_fd);
        //     return -1;
        // }

        if(receivem->flags != 3){
            free(receivem);
            return -1;
        }

        if(receivem->ack_num != sockfd->seq_num){
            continue;
        }

        sockfd->seq_num = receivem->ack_num;
        sockfd->ack_num = receivem->seq_num + sizeof(receivem);
        ans += bytes_sent - sizeof(RUDP_Socket);
        count++;
        printf("count:%d\t", count);
    }
    while (count < num_of_packets);

    // If its a FIN packet
    if(sockfd->flags == 2){
        close(sockfd->socket_fd);
        return 0;
    }

    // If its a ACK packet
    if(sockfd->flags == 3){
        return 1;
    }

    return ans; // return only the length of data without the header.
}

int rudp_disconnect(RUDP_Socket *sockfd){
    if(sockfd == NULL){
        return 0;
    }
    if(sockfd->isConnected == false){
        return 0;
    }
    sockfd->flags = 2; // A FIN flag.
    int bytes_sent = sendto(sockfd->socket_fd, sockfd, sizeof(*sockfd), 0, (struct sockaddr *)&sockfd->dest_addr, sizeof(sockfd->dest_addr));

    // If the message sending failed, print an error message and return 1.
    // If no data was sent, print an error message and return 1. In UDP, this should not happen unless the message is empty.
    if (bytes_sent <= 0)
    {
        perror("sendto(2)");
        close(sockfd->socket_fd);
        return 0;
    }

    sockfd->checksum = calculate_checksum(sockfd, sizeof(*sockfd));
    sockfd->seq_num += sizeof(sockfd);

    RUDP_Socket *receivem = NULL;

    int flag_timeout = 1;
    while(flag_timeout){
        flag_timeout = 0;

        // Try to receive a ack from the server using the socket.
        int bytes_received = recvfrom(sockfd->socket_fd, receivem, sizeof(*receivem), 0, (struct sockaddr *)&sockfd->dest_addr, (socklen_t *)sizeof(sockfd->dest_addr));

        // If the message receiving failed, print an error message and return 0.
        // If no data was received, print an error message and return 0.
        if (bytes_received <= 0)
        {
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    flag_timeout = 1;
                    continue;
                }else{
                    perror("recvfrom(2)");
                    close(sockfd->socket_fd);
                    return 0;
                }
            }
            perror("recvfrom(2)");
            close(sockfd->socket_fd);
            return 0;
        }
    }
    unsigned short result  = calculate_checksum(receivem, sizeof(*receivem));
    if(result != receivem->checksum){
        printf("client is:%d", receivem->checksum);
        printf("server is:%d", result);
        perror("checksum error");
        close(sockfd->socket_fd);
        return 0;
    }

    sockfd->ack_num = receivem->seq_num + sizeof(receivem);
    sockfd->seq_num = receivem->ack_num;

    sockfd->isConnected = false;

    close(sockfd->socket_fd);
    
    return 1;
}

int rudp_close(RUDP_Socket *sockfd){
    if(sockfd == NULL){
        return 0;
    }
    if(sockfd->socket_fd == -1){
        free(sockfd);
        return 0;
    }
    close(sockfd->socket_fd);
    free(sockfd);
    return 0;
}

unsigned short int calculate_checksum(void *data, unsigned int bytes) {
unsigned short int *data_pointer = (unsigned short int *)data;
//printf("-----data_pointer:%d\n", *data_pointer);
unsigned int total_sum = 0;
// Main summing loop
while (bytes > 1) {
total_sum += *data_pointer++;
bytes -= 2;
//printf("total_sum:%d\t", total_sum);
}
// Add left-over byte, if any
if (bytes > 0)
total_sum += *((unsigned char *)data_pointer);
// Fold 32-bit sum to 16 bits
while (total_sum >> 16)
total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
printf("\ntotal_sum for the checksum:%d\n", total_sum);
return (((unsigned short int)total_sum));
}
