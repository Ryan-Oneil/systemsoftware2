#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "constants.h"
#include <sys/stat.h>
#include <errno.h>

long getFileSize(const char *fileName) {
    struct stat st;
    stat(fileName, &st);

    return st.st_size;
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
    send(socket, fileSize, sizeof(fileSize), 0);

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
    char message[500];
    char serverMessage[500];

    printf("\nEnter fileName: ");
    scanf("%s", message);

    send(networkSocket, message, strlen(message), 0);
    uploadFile(networkSocket, message);

    if (recv(networkSocket, serverMessage, strlen(serverMessage), 0) < 0) {
        printf("\nIO error: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    printf("\nMessage received %s\n", serverMessage);

    close(networkSocket);

    return 0;
}
