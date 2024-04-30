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
    
    if (inet_pton(AF_INET, dest_ip, &sockfd->dest_addr.sin_addr) <= 0)
    {
        perror("inet_pton(3)");
        close(sockfd->socket_fd);
        return 0;
    }

   
    Packet *send_pack = create_packet();
    memset(send_pack, 0, sizeof(Packet));
    send_pack->flag_syn = 1;
    send_pack->checksum = calculate_checksum(send_pack, sizeof(Packet));

    socklen_t adressLen = sizeof(sockfd->dest_addr);
    int bytes_sent = sendto(sockfd->socket_fd, send_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);

    // If the message sending failed, print an error message and return 1.
    // If no data was sent, print an error message and return 1. In UDP, this should not happen unless the message is empty.
    if (bytes_sent <= 0)
    {
        perror("sendto(2)");
        free(send_pack);
        close(sockfd->socket_fd);
        return 0;
    }
    free(send_pack);

    Packet recv_pack;

    int  flag = 1;
    while(flag){

        flag = 0;

        int bytes_received = recvfrom(sockfd->socket_fd, &recv_pack, sizeof(Packet), 0, NULL, 0);
        // If the message receiving failed, print an error message and return 0.
        // If no data was received, print an error message and return 0.
        if (bytes_received <= 0)
        {
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    flag = 1;
                    printf("timeout\n");
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

    unsigned short client_checksum = recv_pack.checksum;
    recv_pack.checksum = 0;
    unsigned short server_checksum  = calculate_checksum(&recv_pack, sizeof(Packet));

    if(client_checksum != server_checksum){
        perror("checksum error");
        close(sockfd->socket_fd);
        return 0;
    }

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

    Packet recv_pack;


    socklen_t adressLen = sizeof(sockfd->dest_addr);

    int flag_timeout = 1;
    while(flag_timeout){
        flag_timeout = 0;

        // Try to receive a ack from the server using the socket.
        int bytes_received = recvfrom(sockfd->socket_fd, &recv_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, &adressLen);

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
                    return 0;
                }
            }
            //printf("2\n");
            perror("recvfrom(2)");
            close(sockfd->socket_fd);
            return 0;  
        }
    }

    unsigned short sender_checksum = recv_pack.checksum;
    recv_pack.checksum = 0;
    unsigned short receiver_checksum  = calculate_checksum(&recv_pack, sizeof(Packet));
    if(sender_checksum != receiver_checksum){
        perror("checksum error");
        close(sockfd->socket_fd);
        return 0;
    }

    // Now the server will send the ack + syn.

    Packet *send_pack = create_packet();
    memset(send_pack, 0, sizeof(Packet));
    send_pack->flag_syn = 1;
    send_pack->flag_ack = 1;
    send_pack->checksum = calculate_checksum(send_pack, sizeof(Packet));

    int bytes_sent = sendto(sockfd->socket_fd, send_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);

    // If the message sending failed, print an error message and return 1.
    // If no data was sent, print an error message and return 1. In UDP, this should not happen unless the message is empty.
    if (bytes_sent <= 0)
    {
        perror("sendto(2)");
        free(send_pack);
        close(sockfd->socket_fd);
        return 0;
    }
    free(send_pack);

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

    int bytes_received = 0;
    Packet *recv_pack = create_packet();
    socklen_t adressLen = sizeof(sockfd->dest_addr);
    do{
        memset(recv_pack, 0, sizeof(Packet));
        bytes_received = recvfrom(sockfd->socket_fd, recv_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, &adressLen);
        if(bytes_received <= 0){
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    continue;
                }else{
                    perror("recvfrom(2)");
                    free(recv_pack);
                    return -1;
                }
            }
            perror("recvfrom(2)");
            free(recv_pack);
            return -1;
        }

        unsigned short sender_checksum = recv_pack->checksum;
        recv_pack->checksum = 0;
        unsigned short receiver_checksum  = calculate_checksum(recv_pack, sizeof(Packet));
        if(sender_checksum != receiver_checksum){
            perror("checksum error");
            free(recv_pack);
            return -1;
        }

        if(recv_pack->flag_fin == true){
            Packet *send_pack = create_packet();
            memset(send_pack, 0, sizeof(Packet));
            send_pack->flag_ack = 1;
            send_pack->flag_fin = 1;
            send_pack->ack_num = recv_pack->seq_num;
            send_pack->length = recv_pack->length;
            send_pack->checksum = calculate_checksum(send_pack, sizeof(Packet));
            int bytes_sent = sendto(sockfd->socket_fd, send_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);
            if(bytes_sent <= 0){
                perror("sendto(2)");
                free(recv_pack);
                free(send_pack);
                return -1;
            }   
            free(send_pack);
            close(sockfd->socket_fd);
            return 0;
        }

        if(recv_pack->flag_data == false){
            free(recv_pack);
            return -1;
        }

        if(recv_pack->flag_syn == true){
            free(recv_pack);
            return -1;
        }

        Packet *send_pack = create_packet();
        memset(send_pack, 0, sizeof(Packet));
        send_pack->flag_ack = 1;
        send_pack->ack_num = recv_pack->seq_num;
        send_pack->length = recv_pack->length;
        send_pack->checksum = calculate_checksum(send_pack, sizeof(Packet));
        int bytes_sent = sendto(sockfd->socket_fd, send_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);
        if(bytes_sent <= 0){
            perror("sendto(2)");
            free(recv_pack);
            free(send_pack);
            return -1;
        }
        free(send_pack);
    }
    while((unsigned int)bytes_received < buffer_size);

    memcpy(buffer, recv_pack->data, buffer_size);
    free(recv_pack);

    return bytes_received;
}

int rudp_send(RUDP_Socket *sockfd, void *buffer, unsigned int buffer_size){ 

    if(sockfd == NULL){
        return -1;
    }

    if(sockfd->isConnected == false){
        return -1;
    }

    if(buffer_size <= 0){
        return -1;
    }

    Packet *send_pack = create_packet();
    memset(send_pack, 0, sizeof(Packet));
    send_pack->flag_data = 1;
    memcpy(send_pack->data, buffer, buffer_size);
    send_pack->length = buffer_size;
    send_pack->checksum = calculate_checksum(send_pack, sizeof(Packet));
    int bytes_sent = 0;
    socklen_t adressLen = sizeof(sockfd->dest_addr);
    do{
        bytes_sent = sendto(sockfd->socket_fd, send_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);
        if(bytes_sent <= 0){
            perror("sendto(2)");
            free(send_pack);
            return -1;
        }
        
        Packet *recv_pack = create_packet();
        memset(recv_pack, 0, sizeof(Packet));
        int bytes_received = recvfrom(sockfd->socket_fd, recv_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, &adressLen);
        if(bytes_received <= 0){
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    continue;
                }else{
                    perror("recvfrom(2)");
                    free(recv_pack);
                    return -1;
                }
            }
            perror("recvfrom(2)");
            free(recv_pack);
            return -1;
        }

        unsigned short sender_checksum = recv_pack->checksum;
        recv_pack->checksum = 0;
        unsigned short receiver_checksum  = calculate_checksum(recv_pack, sizeof(Packet));
        if(sender_checksum != receiver_checksum){
            perror("checksum error");
            free(recv_pack);
            return -1;
        }

        if(recv_pack->flag_fin == true){
            free(recv_pack);
            return 0;
        }

        if(recv_pack->flag_ack == false){
            free(recv_pack);
            return -1;
        }

        if(recv_pack->flag_syn == true){
            free(recv_pack);
            return -1;
        }

        if(recv_pack->length == buffer_size){
            free(recv_pack);
            free(send_pack);
            break;
        }
    }
    while(1);

    return bytes_sent;
}

int rudp_disconnect(RUDP_Socket *sockfd){

    if(sockfd == NULL){
        return 0;
    }

    if(sockfd->isConnected == false){
        return 0;
    }

    Packet *send_pack = create_packet();
    memset(send_pack, 0, sizeof(Packet));
    send_pack->flag_fin = 1;
    send_pack->checksum = calculate_checksum(send_pack, sizeof(Packet));
    socklen_t adressLen = sizeof(sockfd->dest_addr);
    while(1){

        int bytes_sent = sendto(sockfd->socket_fd, send_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, adressLen);
        if(bytes_sent <= 0){
            perror("sendto(2)");
            free(send_pack);
            return 0;
        }

        Packet *recv_pack = create_packet();
        memset(recv_pack, 0, sizeof(Packet));
        int bytes_received = recvfrom(sockfd->socket_fd, recv_pack, sizeof(Packet), 0, (struct sockaddr *)&sockfd->dest_addr, &adressLen);
        if(bytes_received <= 0){
            if(bytes_received < 0){
                if(errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS){
                    continue;
                }else{
                    perror("recvfrom(2)");
                    free(recv_pack);
                    free(send_pack);
                    return 0;
                }
            }
            perror("recvfrom(2)");
            free(recv_pack);
            free(send_pack);
            return 0;
        }

        unsigned short sender_checksum = recv_pack->checksum;
        recv_pack->checksum = 0;
        unsigned short receiver_checksum  = calculate_checksum(recv_pack, sizeof(Packet));
        if(sender_checksum != receiver_checksum){
            perror("checksum error");
            free(recv_pack);
            return 0;
        }

        if(recv_pack->flag_ack == false){
            free(recv_pack);
            free(send_pack);
            return 0;
        }

        if(recv_pack->flag_fin == true){
            free(recv_pack);
            free(send_pack);
            break;
        }
    }

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
//printf("\ntotal_sum for the checksum:%d\n", total_sum);
return (((unsigned short int)total_sum));
}

Packet* create_packet(){
    Packet *packet = (Packet*)malloc(sizeof(Packet));
    if(packet == NULL){
        perror("allocate problem");
        exit(1);
    }
    return packet;
}
