#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/fsuid.h>
#include <dirent.h>
#include "socket.c"

void verifyDirectory(const char *dir, int socket);
void setupUserCredentials(int socket);
void* handleNewClient(void *socketNum);
char* downloadFile (int socket, char *fileName, const char *directory);

pthread_mutex_t lock;

int main() {
    int serverSocket = createServerSocket();
    struct sockaddr_in client;

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
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
    pthread_mutex_destroy(&lock);
    return 0;
}

void* handleNewClient(void *socketNum) {
    int socket = *((int *) socketNum);
    char fileName[LENGTH] = "";
    char directory[LENGTH] = "";

    setupUserCredentials(socket);
    getInputFromSocket(socket, directory, LENGTH);
    getInputFromSocket(socket, fileName, LENGTH);

    printf("\nClient sent %s\n", fileName);

    pthread_mutex_lock(&lock);

    char* result = downloadFile(socket, fileName, directory);

    pthread_mutex_unlock(&lock);

    write(socket, result, strlen(result));

    free(socketNum);
    printf("\nClient disconnecting\n");
}

char* downloadFile(int socket, char *fileName, const char *directory) {
    verifyDirectory(directory, socket);

    char revbuf[LENGTH];
    char *fr_name = (char *) malloc(1 + strlen(directory)+ strlen(fileName));
    sprintf(fr_name, "%s/%s", directory, fileName);

    FILE *fr = fopen(fr_name, "w");

    if(fr == NULL) {
        printf("File %s Cannot be opened file on server. %s\n", fr_name, strerror(errno));
        free(fr_name);

        return strerror(errno);
    }
    int fr_block_sz = 0;
    long fileSize = 0;

    //Gets file size from client
    getInputFromSocket(socket, revbuf, LENGTH);
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

    printf("File transfer complete\n");
    fclose(fr);
    free(fr_name);

    sleep(10);

    return "File has successfully uploaded";
}

void verifyDirectory(const char *directory, int socket) {
    printf("\nChecking if %s exists\n", directory);
    DIR* dir = opendir(directory);

    if (dir != NULL) {
        closedir(dir);
    } else {
        fprintf(stderr, "Client failed due to errno = %s\n", strerror(errno));
        write(socket, strerror(errno), LENGTH);

        pthread_exit(NULL);
    }
}

void setupUserCredentials(int socket) {
    char uid[ID_SIZE] = "";
    char gid[ID_SIZE] = "";

    getInputFromSocket(socket, uid, ID_SIZE);
    getInputFromSocket(socket, gid, ID_SIZE);

    printf("User UID: %d, GID: %d", atoi(uid), atoi(gid));
    setfsgid(atoi(gid));
    setfsuid(atoi(uid));
}