#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "sock.h"

int clientArr[MaxConnects];
int clientNum = 0;
pthread_t threads[MaxConnects];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void report(const char* msg, int terminate) {
        perror(msg);
        if (terminate) exit(-1); // failure
} // end report

// send/receive cycle for connected clients
void *clientFunc(void *ptr) {
        char buffer[BuffSize + 1];
        int client_fd = *((int*) ptr);
        FILE *fp;

        // constantly send to and receive from client
        while(1) {
                pthread_mutex_lock(&mutex);
                memset(buffer, '\0', sizeof(buffer)); // reset buffer
                int count = recv(client_fd, &buffer, sizeof(buffer), 0);
                if (count > 0) { // received a message
                        puts(buffer);
                        int curr_client;
                        for (int i = 0; i < MaxConnects; ++i) {
                                curr_client = clientArr[i];
                                if (curr_client > 0) // on valid address
                                        send(curr_client, &buffer, sizeof(buffer), 0);
                        } // end for

                        if (strcmp(buffer, "exit") == 0) {
                                pthread_mutex_unlock(&mutex);
                                break;
                        } // end if
                        else {
                                // save to chat history
                                fp = fopen("chat_history", "a");
                                fprintf(fp, "%s\n", buffer);
                                fclose(fp);
                        } // end else
                } // end if
                pthread_mutex_unlock(&mutex);
        } // end while
} // end clientFunc

// listen for clients
void *listening(void *ptr) {
        while (1) { // listen indefinitely
                int fd = *((int*) ptr);
                struct sockaddr_in caddr; // client address
                int len = sizeof(caddr); // address length could change

                int client_fd = accept(fd, (struct sockaddr*) &caddr, &len);
                if (client_fd < 0) {
                        report("accept", 0);
                        continue;
                }
                else { // valid client address
                        pthread_t clientThread;
                        threads[clientNum] = clientThread;

                        clientArr[clientNum] = client_fd;
                        clientNum++;

                        // spawn new thread for each connected client
                        pthread_create(&clientThread, NULL, clientFunc, (void *) &client_fd);
                } // end else
        } // end while
} // end listening

int main() {
        // clear chat history file
        FILE *fp = fopen("chat_history", "w");
        fclose(fp);

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) report("socket", 1); // terminate

        // bind server's local address in memory
        struct sockaddr_in saddr;
        memset(&saddr, 0, sizeof(saddr));       // clear the bytes
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        saddr.sin_port = htons(PortNumber);

        if (bind(fd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
                report("bind", 1); // terminate

        if (listen(fd, MaxConnects) < 0) // listen for clients up to MaxConnects
                report("listen", 1); // terminate

        fprintf(stderr, "Listening on port %i for clients...\n", PortNumber);

        // spawn new thread for listening
        pthread_t listenThread;
        int retVal = pthread_create(&listenThread, NULL, listening, (void *) &fd);

        // close everything once listening is done
        pthread_join(listenThread, NULL);
        for (int i = 0; i < MaxConnects; ++i) {
                if (clientArr[i] > 0) { // only valid addresses
                        close(clientArr[i]);
                        pthread_join(threads[i], NULL);
                } // end if
        } // end for
        close(fd);

        return 0;
} // end main
