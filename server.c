#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "constants.h"
#include <errno.h>
#include <pthread.h>
#include <sys/fsuid.h>

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
void getInputFromSocket(int socket, char *buffer) {
    long readSize = recv(socket, buffer, sizeof(buffer), 0);
    checkSocketInput(readSize);

    printf("\nReceived %s from client\n", buffer);

    //Sends acknowledgement to client
    write(socket, OK_MESSAGE, strlen(OK_MESSAGE));
}

char* downloadFile (int socket, char *fileName) {
    char* fr_path = "serverfiles/";
    char revbuf[LENGTH];
    char *fr_name = (char *) malloc(1 + strlen(fr_path)+ strlen(fileName));
    sprintf(fr_name, "%s%s", fr_path, fileName);

    FILE *fr = fopen(fr_name, "w");

    if(fr == NULL) {
        printf("File %s Cannot be opened file on server.\n", fr_name);
        free(fr_name);

        return "File failed to transfer";
    }
    int fr_block_sz = 0;
    long fileSize = 0;

    //Gets file size from client
    getInputFromSocket(socket, revbuf);
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
    checkSocketInput(fr_block_sz);

    printf("Received the whole file size\n");
    fclose(fr);
    free(fr_name);

    return "File has successfully uploaded";
}

void* handleNewClient(void *socketNum) {
    int socket = *((int *) socketNum);
    char message[LENGTH] = "";
    char uid[20] = "";
    char gid[20] = "";

    getInputFromSocket(socket, uid);
    getInputFromSocket(socket, gid);

    printf("User UID: %d, GID: %d", atoi(uid), atoi(gid));
    setfsgid(atoi(gid));
    setfsuid(atoi(uid));

    getInputFromSocket(socket, message);

    printf("\nClient sent %s\n", message);

    char* result = downloadFile(socket, message);
    write(socket, result, strlen(result));

    free(socketNum);
    printf("\nTransfer complete, client disconnecting\n");
}

int main() {
    struct sockaddr_in server, client;
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Unable to bind to port\n");
        exit(EXIT_FAILURE);
    } else {
        printf("Bound to port %d\n", PORT);
    }

    if (listen(serverSocket, 3) != 0) {
        printf("Error listening to connections\n");
    }

    while (1) {
        pthread_t tid;
        printf("\nWaiting for a connection\n");
        int connSize = sizeof(struct sockaddr_in);
        int *newSocket = malloc(sizeof(*newSocket));

        *newSocket = accept(serverSocket, (struct sockaddr*)&client, (socklen_t*)&connSize);

        if (*newSocket < 0 ) {
            perror("\nCouldn't establish connection\n");
            continue;
        } else {
            printf("Accepted connection from client\n");
        }
        pthread_create(&tid, NULL, handleNewClient, (void *) newSocket);
    }
    return 0;
}
