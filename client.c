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
#include "socket.c"

void sendCredentialsToServer(int networkSocket);
long getFileSize(const char *fileName);
void uploadFile(int socket, char *file);

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

void sendCredentialsToServer(int networkSocket) {
    char uid[ID_SIZE];
    char gid[ID_SIZE];

    sprintf(uid, "%d", geteuid());
    sprintf(gid, "%d", getegid());

    sendInputToServer(networkSocket, uid, ID_SIZE);
    sendInputToServer(networkSocket, gid, ID_SIZE);
}

long getFileSize(const char *fileName) {
    struct stat st;
    stat(fileName, &st);

    return st.st_size;
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