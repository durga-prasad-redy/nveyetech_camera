//
// Created by outdu-gram on 27/6/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define IPAddress "192.168.1.180"
#define PORT 8000
#define BUFFER_SIZE 1024

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, IPAddress, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Receive data from the server
    // valread = read(sock, buffer, BUFFER_SIZE);
    // printf("Received Data: %s\n", buffer);
    while (1) {
        valread = read(sock, buffer, BUFFER_SIZE);
        if (valread == 0) {
            printf("Server stopped sending data. Exiting...\n");
            break;
        }

        printf("Received Data: %s\n", buffer);
    }

    close(sock);
    return 0;
}
