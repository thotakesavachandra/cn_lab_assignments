// A Simple UDP Server that sends a HELLO message
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include<dirent.h>
  
#define MAXLINE 1024 


int getFiles(char *files[]){
    int i = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            files[i] = (char*)malloc(sizeof(char)*strlen(dir->d_name));
            strcpy(files[i], dir->d_name);
            i++;
        }
        closedir(d);
    }
    return i;
}


  
int main() { 
    int sockfd; 
    struct sockaddr_in servaddr, cliaddr; 
    int n; 
    socklen_t len;
    char buffer[MAXLINE]; 
      
    // Create socket file descriptor 
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    servaddr.sin_family    = AF_INET; 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(8181); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
  
    char *files[100];
    int nFiles = getFiles(files);
    printf("Files in the current directory : \n");
    for(int i = 0; i < nFiles; i++){
        printf("%d) %s\n", i+1, files[i]);
    }

    printf("\nServer Running....\n\n");

    len = sizeof(cliaddr);
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0, 
			( struct sockaddr *) &cliaddr, &len); 
    buffer[n] = '\0'; 


    char fileName[100];
    strcpy(fileName, buffer);
    
    int found = 0;
    for(int i=0;i<nFiles;i++){
        if(strcmp(fileName, files[i]) == 0){
            found = 1;
            break;
        }
    }

    printf("%s is requested\n",fileName);

    if(found==0){
        printf("FILE NOT FOUND\n");
        char returnmsg[100];
        strcpy(returnmsg,"FILE ");
        strcat(returnmsg,fileName);
        strcat(returnmsg," NOT FOUND");
        sendto(sockfd, returnmsg, sizeof(returnmsg), 0,
            (struct sockaddr *) &cliaddr, sizeof(cliaddr));
        exit(0);
    }

    
    printf("FILE FOUND\n");
    FILE* fp = fopen(fileName,"r");
    
    // get the number of lines in file


    // sending HELLO

    char word[100];
    fscanf(fp,"%s",word);
    sendto(sockfd, word, sizeof(word), 0,
                (struct sockaddr *) &cliaddr, sizeof(cliaddr));


    // sending remaining WORDS

    int i=1;
    while(fgets(word,100,fp)!=NULL){
        char* p = strchr(word, '\n');
        if (p) {
            *p = 0;
        }
        // get request for word
        int n = recvfrom(sockfd, (char *)buffer, MAXLINE, 0,
            (struct sockaddr *) &cliaddr, &len);
        buffer[n] = '\0';
        
        // 
        char checkWord[100];
        strcpy(checkWord,"WORD");
        char num[10];
        sprintf(num,"%d",i);
        strcat(checkWord,num);
        if(strcmp(buffer,checkWord) == 0){
            // fscanf(fp,"%s",word);
            sendto(sockfd, word, sizeof(word), 0,
                (struct sockaddr *) &cliaddr, sizeof(cliaddr));
        }
        else{
            char returnmsg[100];
            strcpy(returnmsg,"WORD");
            strcat(returnmsg,num);
            strcat(returnmsg," NOT FOUND");
            sendto(sockfd, returnmsg, sizeof(returnmsg), 0,
                (struct sockaddr *) &cliaddr, sizeof(cliaddr));
        }
    }
    
    printf("Transmission Complete :)\n");
    close(sockfd);
    return 0; 
} 
