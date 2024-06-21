// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
#define MAXLINE 1024

int main() { 
    int sockfd, err; 
    struct sockaddr_in servaddr; 
    int n;
    socklen_t len = sizeof(servaddr); 
    char *hello = "CLIENT:HELLO"; 
  
    // Creating socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
      
    // Server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    err = inet_aton("127.0.0.1",&servaddr.sin_addr);
    // err = inet_aton("127.0.0.1", &servaddr.sin_addr);
    if (err == 0) {
	   printf("Error in ip-conversion\n");
	   exit(EXIT_FAILURE);
     }


    char fileName[100];
    printf("Enter the file name: ");
    scanf("%s", fileName);



    if(sendto(sockfd, fileName, strlen(fileName), 0, 
			(const struct sockaddr *) &servaddr, len) <0){
        perror("sendto failed");
        exit(EXIT_FAILURE);
    } 
    
    printf("Request sent\n");

    char buffer[100];
    n = recvfrom(sockfd,buffer,MAXLINE,0
            ,(struct sockaddr *) &servaddr, &len);
    buffer[n] = '\0';

    if(strcmp(buffer,"HELLO")!=0){
        printf("Error in connection\n");
        exit(EXIT_FAILURE);
    }

    printf("Connection established\n");

    // create a file writfile.txt
    FILE* fp = fopen("writefile.txt", "w");
    
    int i=1;
    while(1){
        char req[100],num[10];
        strcpy(req,"WORD");
        sprintf(num,"%d",i);
        strcat(req,num);
        sendto(sockfd,req,strlen(req),0
            ,(struct sockaddr *) &servaddr, len);
        n = recvfrom(sockfd,buffer,MAXLINE,0
            ,(struct sockaddr *) &servaddr, &len);
        buffer[n] = '\0';
        if(strcmp(buffer,"END")==0){
            break;
        }
        fprintf(fp,"%s\n",buffer);

        // print buffer to the file

    }
    fclose(fp);    
    close(sockfd); 

    printf("Process Completed :)\n");
    return 0; 
} 
