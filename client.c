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

void readInputFromSocket(int networkSocket);

long getFileSize(const char *fileName) {
    struct stat st;
    stat(fileName, &st);

    return st.st_size;
}

//This function waits for the server to acknowledge the data we sent before sending more
void sendInputToServer(int socket, void *message) {
    char serverMessage[100];
    send(socket, message, sizeof(message), 0);

    if (recv(socket, serverMessage, 100, 0) < 0) {
        printf("\nIO error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("\nStatus: %s\n", serverMessage);
}

void uploadFile(int socket, const char *fileName) {
    char* fs_path = "clientFiles/";
    char* fs_name = (char *) malloc(1 + strlen(fs_path)+ strlen(fileName));
    sprintf(fs_name, "%s%s", fs_path, fileName);

    char sdbuf[LENGTH];
    printf("[Client] Sending %s to the server... ", fs_name);
    FILE *fs = fopen(fs_name, "r");

    if (fs == NULL) {
        printf("ERROR: File %s not found. \n", fs_name);
        free(fs_name);

        exit(EXIT_FAILURE);
    }
    bzero(sdbuf, LENGTH);
    char fileSize[255];
    sprintf(fileSize, "%ld", getFileSize(fs_name));
    printf("\nSending filesize of %s\n", fileSize);

    //Sends server the fileSize to determine the end of file
    sendInputToServer(socket, fileSize);

    int fs_block_sz;

    while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0) {
        if (send(socket, sdbuf, fs_block_sz, 0) < 0) {
            fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
            exit(EXIT_FAILURE);
        }
        bzero(sdbuf, LENGTH);
    }
    free(fs_name);
    fclose(fs);
}

int main() {
    int networkSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (networkSocket == -1) {
        printf("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(IP);
    server_address.sin_port = htons(PORT);

    int connection_status = connect(networkSocket, (struct sockaddr*)&server_address, sizeof(server_address));

    if (connection_status < 0) {
        printf("\nError connecting to server");
        exit(EXIT_FAILURE);
    }
    printf("Connection established\n");
    char message[LENGTH];
    char uid[30];
    char gid[30];

    sprintf(uid, "%d", geteuid());
    sprintf(gid, "%d", getegid());

    sendInputToServer(networkSocket, uid);
    sendInputToServer(networkSocket, gid);

    printf("\nEnter fileName: ");
    scanf("%512s", message);

    sendInputToServer(networkSocket, message);
    uploadFile(networkSocket, message);

    readInputFromSocket(networkSocket);

    close(networkSocket);

    return 0;
}

void readInputFromSocket(int networkSocket) {
    char serverMessage[LENGTH];

    if (recv(networkSocket, serverMessage, LENGTH, 0) < 0) {
        printf("\nIO error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("\nStatus: %s\n", serverMessage);
}
