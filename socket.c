#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "constants.h"

struct Socket {
    int socket;
    struct sockaddr_in address;
};

struct Socket createSocket() {
    int networkSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (networkSocket == -1) {
        printf("\nError creating socket\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in address;
    address.sin_addr.s_addr = inet_addr(IP);
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);

    struct Socket createdSocket = {networkSocket, address};

    return createdSocket;
}

int createClientSocket() {
    struct Socket serverSocket = createSocket();

    int connection_status = connect(serverSocket.socket, (struct sockaddr*)&serverSocket.address, sizeof(serverSocket.address));

    if (connection_status < 0) {
        printf("\nError connecting to server\n");
        exit(EXIT_FAILURE);
    }
    printf("\nConnection established\n");

    return serverSocket.socket;
}

int createServerSocket() {
    struct Socket serverSocket = createSocket();

    if (bind(serverSocket.socket, (struct sockaddr*)&serverSocket.address, sizeof(serverSocket.address)) < 0) {
        printf("Unable to bind to port\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Bound to port %d\n", PORT);
    }

    if (listen(serverSocket.socket, 3) != 0) {
        printf("Error listening to connections\n");
    }
    return serverSocket.socket;
}

void checkSocketInput(int status) {
    if(status < 0) {
        if (errno == EAGAIN) {
            fflush(stdout);
            printf("\nClient timed out.\n");
        } else {
            fprintf(stderr, "Client failed due to errno = %d\n", errno);
            pthread_exit(NULL);
        }
    }
}

//This function prevents input from client getting sent with other input
//Without this the client could end up sending all the data at once causing confusion on server end
void getInputFromSocket(int socket, char *buffer, int bufferSize) {
    long readSize = recv(socket, buffer, bufferSize, 0);
    checkSocketInput(readSize);

    printf("\nReceived %s from client\n", buffer);

    //Sends acknowledgement to client
    write(socket, OK_MESSAGE, LENGTH);
}

//This function waits for the server to acknowledge the data we sent before sending more
void sendInputToServer(int socket, void *message, int messageSize) {
    printf("\nSending %s to server\nWaiting on response\n", (char *) message);
    char serverMessage[LENGTH];

    send(socket, message, messageSize, 0);

    if (recv(socket, serverMessage, LENGTH, 0) < 0) {
        printf("\nIO error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("\nServer: %s\n", serverMessage);

    if (strcmp(serverMessage, OK_MESSAGE) != 0) {
        exit(EXIT_FAILURE);
    }
}

void readInputFromSocket(int networkSocket) {
    char serverMessage[LENGTH];

    if (recv(networkSocket, serverMessage, LENGTH, 0) < 0) {
        printf("\nIO error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("\nStatus: %s\n", serverMessage);
}