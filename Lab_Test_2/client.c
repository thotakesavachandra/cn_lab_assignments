#include "headers.h"



void recv_null_term_str(int sockfd, char * buff){
    int n_received = 0;
    while(1){
        int r = recv(sockfd, buff+n_received, 1, 0);
        n_received += r;
        if(*(buff+n_received-1)=='\0'){
            break;
        }
    }
}



int main(){
    printf("--------------- Welcome ------------------\n");
    
    struct sockaddr_in servaddr;
    int servlen = sizeof(servaddr);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    inet_aton(SERVER_ADDR, &servaddr.sin_addr);

    int r = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(r<0){
        perror("Socket Creation Failed\n");
        exit(EXIT_FAILURE);
    }
    int sockfd = r;

    r = connect(sockfd, (struct sockaddr*)&servaddr, servlen);
    if(r<0){
        perror("Connect Failed\n");
        exit(EXIT_FAILURE);
    }

    int n;
    recv(sockfd, (char*)&n, 4, MSG_WAITALL);
    n = ntohl(n);

    char** names = (char**) malloc(sizeof(char*)*n);

    for(int i=0; i<n; i++){
        names[i] = (char*)malloc(MAX_NAME_SIZE);
        recv_null_term_str(sockfd, names[i]);
        // int n_received = 0;
        // while(1){
        //     r = recv(sockfd, names[i]+n_received, 1, 0);
        //     if(*(names[i]+n_received)=='\0'){
        //         break;
        //     }
        //     n_received += r;
        // }
    }

    printf("Candidate List : \n");

    for(int i=0; i<n; i++){
        printf("%d)%s\n", i+1, names[i]);
    }
    printf("\nEnter the choice name: ");
    char choice[MAX_NAME_SIZE];
    fgets(choice, MAX_NAME_SIZE, stdin);
    *strchr(choice, '\n') = '\0';

    if(strlen(choice)==0){
        printf("Refrained From Voting\n");
    }
    else{
        r = send(sockfd, choice, strlen(choice)+1, 0);
        if(r<0){
            perror("Send Failed\n");
            exit(EXIT_FAILURE);
        }

        char buff[100];
        recv_null_term_str(sockfd, (char*)buff);
        printf("Message from Server: %s\n", buff);
    }
    close(sockfd);
}