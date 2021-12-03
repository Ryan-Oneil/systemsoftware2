#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "constants.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <libgen.h>

void readInputFromSocket(int networkSocket);
void sendCredentialsToServer(int networkSocket);
int createClientSocket();

long getFileSize(const char *fileName) {
    struct stat st;
    stat(fileName, &st);

    return st.st_size;
}

//This function waits for the server to acknowledge the data we sent before sending more
void sendInputToServer(int socket, void *message, int messageSize) {
    printf("\nSending %s to server\n", (char *) message);
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

void uploadFile(int socket, char *file) {
    char sdbuf[LENGTH];
    char *fileName = basename(file);

    sendInputToServer(socket, fileName, LENGTH);

    printf("[Client] Sending %s to the server... ", fileName);
    FILE *fs = fopen(fileName, "r");

    if (fs == NULL) {
        printf("\nERROR: File %s not found. \n", fileName);

        exit(EXIT_FAILURE);
    }
    bzero(sdbuf, LENGTH);
    char fileSize[LENGTH];
    sprintf(fileSize, "%ld", getFileSize(fileName));
    printf("\nSending filesize of %s\n", fileSize);

    //Sends server the fileSize to determine the end of file
    sendInputToServer(socket, fileSize, LENGTH);

    int fs_block_sz;

    while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0) {
        if (send(socket, sdbuf, fs_block_sz, 0) < 0) {
            fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fileName, errno);
            exit(EXIT_FAILURE);
        }
        bzero(sdbuf, LENGTH);
    }
    fclose(fs);
}

int main() {
    int networkSocket = createClientSocket();

    char file[LENGTH];
    char uploadDirectory[LENGTH];

    sendCredentialsToServer(networkSocket);

    printf("\nEnter the file name and path you'd like to upload: ");
    scanf("%512s", file);

    printf("\nEnter the directory you'd like to upload to [Manufacturing | Distribution]: ");
    scanf("%512s", uploadDirectory);

    sendInputToServer(networkSocket, uploadDirectory, LENGTH);
    uploadFile(networkSocket, file);

    readInputFromSocket(networkSocket);
    close(networkSocket);

    return 0;
}

int createClientSocket() {
    int networkSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (networkSocket == -1) {
        printf("\nError creating socket\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(IP);
    server_address.sin_port = htons(PORT);

    int connection_status = connect(networkSocket, (struct sockaddr*)&server_address, sizeof(server_address));

    if (connection_status < 0) {
        printf("\nError connecting to server\n");
        exit(EXIT_FAILURE);
    }
    printf("\nConnection established\n");

    return networkSocket;
}

void sendCredentialsToServer(int networkSocket) {
    char uid[ID_SIZE];
    char gid[ID_SIZE];

    sprintf(uid, "%d", geteuid());
    sprintf(gid, "%d", getegid());

    sendInputToServer(networkSocket, uid, ID_SIZE);
    sendInputToServer(networkSocket, gid, ID_SIZE);
}

void readInputFromSocket(int networkSocket) {
    char serverMessage[LENGTH];

    if (recv(networkSocket, serverMessage, LENGTH, 0) < 0) {
        printf("\nIO error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("\nStatus: %s\n", serverMessage);
}
