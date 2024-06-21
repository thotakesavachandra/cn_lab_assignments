#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>

#ifndef BASE
    #define BASE 50000
#endif


int recvMsg(int sockfd,char buff[],int size,const char* pattern){
    int totRecv = 0;
    int len = strlen(pattern);
    while(1){
        int curr = recv(sockfd,buff+totRecv,size-totRecv,0); 
        if(curr<0) return -1;
        // printf("%d",curr); fflush(stdout);
        totRecv += curr;
        if(totRecv>=len && strncmp(buff+totRecv-len,pattern,len)==0){
            // printf("breaking %d\n",totRecv);
            break;
        }
    }
    buff[totRecv]='\0';
    return totRecv;
}



typedef struct UserInfo{
    int isConnected;
    struct sockaddr_in serv_addr, cli_addr;
    int sockfd;
}UserInfo;

UserInfo initUser(int isConnected, int serverPort){
    UserInfo user;
    user.isConnected = isConnected;
    // user.serverPort = serverPort;
    user.sockfd = 0;

    user.serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &user.serv_addr.sin_addr);
    user.serv_addr.sin_port = htons(serverPort);
    return user;
}


int main(int argc, char *argv[]){

    if(argc != 2){
        printf("Usage: %s <yourId>\n", argv[0]);
        exit(1);
    }
    int id = atoi(argv[1]);

    struct UserInfo user[4];
    // user[0] = {0, 50000, 0};
    user[1] = initUser(0, BASE+0);
    user[2] = initUser(0, BASE+1);
    user[3] = initUser(0, BASE+2);

    user[id].sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int sockfd = user[id].sockfd;
    if (bind(sockfd, (struct sockaddr *) &user[id].serv_addr,
					sizeof(user[id].serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}
    if (listen(sockfd, 5) < 0) {
        printf("Unable to listen\n");
        exit(0);
    }

    
    fd_set rfds;
    FD_SET(1024, &rfds);
    int maxi = user[id].sockfd;
    if(maxi<1024) maxi = 1024;
    FD_SET(user[id].sockfd, &rfds);
    struct timeval tv;
    int retval;

    int recFd[2];
    int isConnected[2] = {0,0};

    /* Watch stdin (fd 0) to see when it has input. */
    // getchar();
    int t = 5;
    while(1){
        FD_ZERO(&rfds);
        FD_SET(0, &rfds);
        FD_SET(user[id].sockfd, &rfds);
        if(isConnected[0]){
            FD_SET(recFd[0], &rfds);
        }
        if(isConnected[1]){
            FD_SET(recFd[1], &rfds);
        }
        // FD_SET(recFd, &rfds);
        
        /* Wait up to five seconds. */

        tv.tv_sec = 50;
        tv.tv_usec = 0;

        retval = select(maxi+1, &rfds, NULL, NULL, &tv);
        /* Don't rely on the value of tv now! */
        for(int i=0;i<=maxi;i++){
            if(FD_ISSET(i,&rfds)){
                printf("FD_ISSET: %d %d\n",i,user[id].sockfd);
            }
        }
        if (retval == -1)
            perror("select()");
        else if (retval){
            printf("Data is available now. %d\n",retval);
            // if(FD_ISSET(0,&rfds)){
            //     printf("stdin is available\n");
            //     char buff[10];
            //     scanf("%s",buff);
            // }
            
            if(FD_ISSET(user[id].sockfd,&rfds)){
                if(isConnected[0]==0){
                    recFd[0] = accept(user[id].sockfd, (struct sockaddr *)&user[id].cli_addr, (socklen_t*)&user[id].cli_addr);
                    if(recFd[0]>maxi) maxi = recFd[0];
                    isConnected[0] = 1;
                }
                else if(isConnected[1]==0){
                    recFd[1] = accept(user[id].sockfd, (struct sockaddr *)&user[id].cli_addr, (socklen_t*)&user[id].cli_addr);
                    if(recFd[1]>maxi) maxi = recFd[1];
                    isConnected[1] = 1;
                }
            }
            else if(isConnected[0] && FD_ISSET(recFd[0],&rfds)){
                char buff[1024];
                int len = read(recFd[0],buff,1024);
                printf("Received: %s\n",buff);
            }
            else if(isConnected[1] && FD_ISSET(recFd[1],&rfds)){
                char buff[1024];
                int len = read(recFd[1],buff,1024);
                printf("Received: %s\n",buff);
            }
            else if(FD_ISSET(0,&rfds)){
                printf("got input\n");  
                char buff[1024];
                // scanf("%s",buff);
                fgets(buff,1024,stdin);
                int target=2;
                sscanf(buff,"user_%d/",&target);
                char msg[1024];
                sprintf(msg,"user_%d: ",id);
                strcat(msg,buff);
                printf("sending message: %s",msg);
                if(send(user[target].sockfd,msg,strlen(msg),0) < 0){
                    user[target].sockfd = socket(AF_INET, SOCK_STREAM, 0);
                    if(connect(user[target].sockfd, (struct sockaddr *)&user[target].serv_addr, sizeof(user[target].serv_addr)) < 0){
                        perror("Connection failed");
                        exit(1);
                    }
                    if(send(user[target].sockfd,msg,strlen(msg),0) <0){
                        printf("failed");
                        exit(0);
                    }
                    printf("send succesful\n");
                    
                }
            }
            printf("received end\n");
        }
        else{
            printf("No data within five seconds.\n");
            for(int i=1;i<=3;i++){
                if(user[i].isConnected){
                    close(user[i].sockfd);
                    user[i].isConnected = 0;
                }
            }
        }
    }

    exit(EXIT_SUCCESS);
}