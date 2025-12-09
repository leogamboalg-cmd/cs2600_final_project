#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define PORT_NUMBER 3264

char username[1024];
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;

int createClient() {

    //create socket
    int fd = socket(AF_INET,SOCK_STREAM,0);

    //check if socket was not able to create
    if(fd<0) {
        perror("socket");
        exit(1);
    }
    return fd; //return socket int
}

void configureServer(struct sockaddr_in *addr, int port) {
    //set up the struct
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("127.0.0.1");
    addr->sin_port = htons(port);
}

void connectServer(int sockfd, struct sockaddr_in *addr) {

    //simply just connect to server, if error occured end program
    if(connect(sockfd, (struct sockaddr*)addr, sizeof(*addr)) <0) {
        perror("connection");
        exit(1);
    }

}

void* receiveThread(void *arg) {

    int sockfd = *(int*)arg;
    char buffer[1024]; //where to store message from server

    while(1) {

        memset(buffer,0,sizeof(buffer)); //clear out buffer

        //check to see if server disconnected
        if(recv(sockfd,buffer,sizeof(buffer),0)<=0) {

            printf("Server disconnected\n");
            break;
        }

        //use locks so that only one message is received
        //at a time
        pthread_mutex_lock(&print_lock);
        printf("\n%s\n", buffer);
        printf("Message: ");
        fflush(stdout);
        pthread_mutex_unlock(&print_lock);
    }

    return NULL;
}

void communicateServer(int sockfd) {


    char buffer[1024]; // used to store message to send

    while(1) {

        pthread_mutex_lock(&print_lock);
        printf("Message: ");
        fflush(stdout);
        pthread_mutex_unlock(&print_lock);

        //clear before new data
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer), stdin);

        //removing newline
        buffer[strcspn(buffer, "\n")] = 0;

        //if user enters exit, exit from server
        if(strcmp(buffer,"exit" == 0) {
            send(sockfd,buffer,strlen(buffer),0);
            break;
        }

        send(sockfd,buffer,strlen(buffer),0);
    }
}


int main(int argc, char *argv[]) {

    //if port isn't provided, end program
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    //receive socket int
    int sockfd = createClient();
    struct sockaddr_in server_addr;

    //configure and connect the server
    configureServer(&server_addr,port);
    connectServer(sockfd,&server_addr);

    //get username from user
    printf("Enter username: ");
    fgets(username,sizeof(username),stdin);
    username[strcspn(username, "\n")] = 0;
    //send username to server
    send(sockfd,username,strlen(username),0);

    //create thread that will handle receiving messages from server
    pthread_t recv;
    pthread_create(&recv, NULL, receiveThread, &sockfd);

    //method to send messages
    communicateServer(sockfd);
    close(sockfd);

    return 0;
}
