#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>


const char domain[] = "iitkgp.edu";
const char endSeq[] = "\r\n";

#define PORT 20000

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


int extractLines(int sockfd,char msg[][100]){
    int nlines=0;
    char buff[10000];
    int n = recvMsg(sockfd,buff,sizeof(buff),"\r\n.\r\n");
    if(n<0) return -1;
    int last = 0;
    int i=0;
    while(i<n){
        if(i>=2 && strncmp(buff+i-1,endSeq,2)==0){
            sprintf(msg[nlines++],"%.*s",i-last+1,buff+last);
            last = i+1;
        }
        i++;
    }
    return nlines;
}

/*
int _recvMsg(int sockfd,char buff[]){
    int totRecv = 0;
    while(1){
        int curr = recv(sockfd,buff+totRecv,1,0); 
        if(curr<0) return -1;
        // printf("%d",curr); fflush(stdout);
        totRecv += curr;
        if(totRecv>=2 && strncmp(buff+totRecv-2,endSeq,2)==0){
            printf("breaking %d\n",totRecv);
            break;
        }
    }
    buff[totRecv]='\0';
    return totRecv;
}
*/


int extractAndCheck(char addr[],char buff[],int size){
    int stIdx = strchr(buff,'<')-buff+1;
    int endIdx = strchr(buff,'>')-buff-1;

    if(stIdx>=endIdx) return 0;

    sprintf(addr,"%.*s",endIdx-stIdx+1,buff+stIdx);
    
    {
        char* ptr = strchr(addr,'@');
        if(ptr==NULL){
            printf("No @ found\n");
            return 0;
        }
        if(strcmp(ptr+1,domain)!=0){
            printf("Domain not matched\n");
            return 0;
        }
        
        *ptr = '\0';
    }

    printf("Extracted address: %s\n",addr); fflush(stdout);
    int len = strlen(addr);
    char fileName [] = "user.txt";
    FILE* fp = fopen(fileName,"r");
    while(!feof(fp)){
        char user[100],pw[100];
        fscanf(fp,"%s %s",user,pw);
        if(strcmp(user,addr)==0){
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void receiveMail(int sockfd){
    int n;
    char buff[100];
    sprintf(buff,"220 <%s> Service ready%s",domain,endSeq);
    send(sockfd,buff,strlen(buff),0);

    /************************************** HELO *****************************************/
    // n = recvMsg(sockfd,buff);
    n = recvMsg(sockfd,buff,sizeof(buff),endSeq);
    if(n<0 || strncmp("HELO",buff,4)!=0) return;
    printf("%s\n",buff); fflush(stdout);

    sprintf(buff,"250 OK Hello %s%s",domain,endSeq);
    send(sockfd,buff,strlen(buff),0);

    /************************************** FROM *****************************************/
    // n = recvMsg(sockfd,buff);
    n = recvMsg(sockfd,buff,sizeof(buff),endSeq);
    if(n<0 || strncmp("MAIL FROM: ",buff,6)!=0) return;
    printf("%s\n",buff);
    fflush(stdout);

    /* Extract sender address and do the needful
    {
        // char sender[100];
        // if(!extractAndCheck(sender,buff,n)){
        //     sprintf(buff,"550 No such user here%s",endSeq);
        //     printf("%s\n",buff);
        //     fflush(stdout);
        //     send(sockfd,buff,strlen(buff),0);
        //     return;
        // }
    }
    */

    sprintf(buff,"250 Sender ok%s",endSeq);
    send(sockfd,buff,strlen(buff),0);

    /************************************** RCPT *****************************************/
    // n = recvMsg(sockfd,buff);
    n = recvMsg(sockfd,buff,sizeof(buff),endSeq);
    if(n<0 || strncmp("RCPT TO: ",buff,8)!=0) return;
    printf("%s\n",buff);

    // extract receipient address and do the needful
    char receipient[100];
    if(!extractAndCheck(receipient,buff,n)){
        sprintf(buff,"550 No such user here%s",endSeq);
        send(sockfd,buff,strlen(buff),0);
        return;
    }
    sprintf(buff,"250 Recipient ok%s",endSeq);
    send(sockfd,buff,strlen(buff),0);

    /************************************** DATA *****************************************/
    // n = recvMsg(sockfd,buff);
    n = recvMsg(sockfd,buff,sizeof(buff),endSeq);
    if(n<0 || strncmp("DATA",buff,4)!=0) return;
    printf("%s\n",buff);

    sprintf(buff,"354 Enter mail, end with \".\" on a line by itself%s",endSeq);
    send(sockfd,buff,strlen(buff),0);

    /************************************** MSSG *****************************************/
    
    char msg[100][100];
    n = extractLines(sockfd,msg);
    if(n<0) return;
    int nlines = n;
    

    /*
    // while(1){
    //     int n = recvMsg(sockfd,buff);
    //     if(n<0) return;
    //     sprintf(msg[nlines++],"%s",buff);
    //     printf("%s",msg[nlines-1]);
    //     if(n==3 && buff[0]=='.' && strcmp(&buff[1],endSeq)==0) break;
    // }
    */
   
    printf("Message received\n"); fflush(stdout);
    {
        char filename[200];
        sprintf(filename,"./%s/mymailbox",receipient);
        FILE* fp = fopen(filename,"a");
        for(int i=0;i<nlines;i++){
            if(i==3){
                // time_t currentTime;
                // time(&currentTime);
                // fprintf(fp,"Received: %s",ctime(&currentTime));

                time_t rawtime;
                struct tm * timeinfo;
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                
                // Format time into desired format
                char buffer[80];
                strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", timeinfo);
                fprintf(fp,"Received: %s%s",buffer,endSeq);
            }
            fprintf(fp,"%s",msg[i]);
        }
        fclose(fp);
    }

    sprintf(buff,"250 OK Message accepted for delivery%s",endSeq);
    send(sockfd,buff,strlen(buff),0);

    /************************************** QUIT *****************************************/
    // n = recvMsg(sockfd,buff);
    n = recvMsg(sockfd,buff,sizeof(buff),endSeq);
    if(n<0) return;
    printf("%s\n",buff);

    sprintf(buff,"221 %s closing connection%s",domain,endSeq);
    send(sockfd,buff,strlen(buff),0);
    
}



int main()
{
	int	sockfd, newsockfd ; /* Socket descriptors */
	int	clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}


	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(PORT);


	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5); 
	printf("Server running...waiting for connections.\n");
	

	while (1) {

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}
		printf("Accepted client %s:%d\n",inet_ntoa(cli_addr.sin_addr),cli_addr.sin_port);


		if (fork() == 0) {

			close(sockfd);

			receiveMail(newsockfd);
			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}
	return 0;
}
			
