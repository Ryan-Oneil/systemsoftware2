#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "constants.h"
#include <errno.h>

char* downloadFile (int socket, char *fileName) {
    char* fr_path = "serverfiles/";
    char revbuf[LENGTH];
    char *fr_name = (char *) malloc(1 + strlen(fr_path)+ strlen(fileName));
    sprintf(fr_name, "%s%s", fr_path, fileName);
    FILE *fr = fopen(fr_name, "w");

    if(fr == NULL) {
        printf("File %s Cannot be opened file on server.\n", fr_name);

        return "File failed to transfer";
    }
    memset(revbuf, 0, LENGTH);
    int fr_block_sz = 0;
    long fileSize = 0;
    recv(socket, revbuf, sizeof(revbuf), 0);
    fileSize = atoi(revbuf);

    printf("\nThe size of the file is %ld\n", fileSize);

    while((fileSize > 0) && ((fr_block_sz = recv(socket, revbuf, LENGTH, 0)) > 0)) {
        int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);

        if (write_sz < fr_block_sz) {
            perror("File write failed on server. \n");
        }
        memset(revbuf, 0, LENGTH);
        fileSize -= fr_block_sz;
    }
    if(fr_block_sz < 0) {
        if (errno == EAGAIN)
            printf("recv() timed out.\n");
        else {
            fprintf(stderr, "recv() failed due to errno = %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }
    printf("Received the whole file size\n");
    fclose(fr);
    free(fr_name);

    return "";
}

int main() {
    char* message;
    long readSize;
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;

    socklen_t addr_size;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Unable to bind to port");
        exit(EXIT_FAILURE);
    } else {
        printf("Bound to port %d\n", PORT);
    }

    if (listen(serverSocket, 3) != 0) {
        printf("Error\n");
    }

    while (1) {
        printf("Waiting for a connection\n");
        addr_size = sizeof(serverStorage);

        newSocket = accept(serverSocket, (struct sockaddr*)&serverStorage, &addr_size);

        if (newSocket < 0 ) {
            perror("Couldn't establish connection");
            exit(EXIT_FAILURE);
        } else {
            printf("Accepted connection from client\n");
        }

        memset(message, 0, 500);
        readSize = recv(newSocket, message, sizeof(message), 0);
        printf("\nClient sent %s\n", message);
        downloadFile(newSocket, message);

        printf("\nLetting the client know the file was uploaded\n");
        write(newSocket, "File has been uploaded", strlen("File has been uploaded"));

        if (readSize == 0) {
            printf("Client disconnected\n");
            fflush(stdout);
        } else if (readSize == -1) {
            perror("recv failed");
        }
    }
    return 0;
}
