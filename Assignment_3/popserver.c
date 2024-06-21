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

#define PORT 30000

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


int checkUser(char uname[],char pword[]){
    char fileName [] = "user.txt";
    FILE* fp = fopen(fileName,"r");
    while(!feof(fp)){
        char un[100],pw[100];
        fscanf(fp,"%s %s",un,pw);
        if(strcmp(un,uname)==0){
            strcpy(pword,pw);
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}



void connectClient(int sockfd){
    int n;
    char buff[100];

    sprintf(buff,"+OK POP3 server ready%s",endSeq);
    send(sockfd,buff,strlen(buff),0);

    /************************************** AUTHORIZATION *****************************************/
    
        n = recvMsg(sockfd,buff,sizeof(buff),endSeq);
        if(n<0 || strncmp("USER ",buff,5)!=0) return;

        char uname[80],pword[80],filename[200];
        sscanf(buff,"USER %s",uname);
        if(checkUser(uname,pword)==0){
            // error case;
            sprintf(buff,"-ERR No user here%s",endSeq);
            send(sockfd,buff,strlen(buff),0);
            return;
        }
        sprintf(buff,"+OK USER accepted%s",endSeq);
        send(sockfd,buff,strlen(buff),0);

        n = recvMsg(sockfd,buff,sizeof(buff),endSeq);
        if(n<0 || strncmp("PASS ",buff,5)!=0) return;
        
        sscanf(buff,"PASS %s",pword);
        if(strcmp(pword,pword)!=0){
            // error case;
            sprintf(buff,"-ERR Invalid Password%s",endSeq);
            send(sockfd,buff,strlen(buff),0);
            return;
        }
        sprintf(buff,"+OK PASS accepted%s",endSeq);
        send(sockfd,buff,strlen(buff),0);

        sprintf(filename,"./%s/mymailbox",uname);

    /************************************** GET DATA ********************************************/
    int nMessages = 0;
    int totalOctets = 0;
    int* marked;
    int* octets;
    {
        FILE* fp = fopen(filename,"r");
        char temp[100];
        while(fgets(temp,sizeof(temp),fp)!=NULL){
            // count octets
            totalOctets += strlen(temp);
            // printf("%s",temp);
            if(strncmp(temp,".\r\n",3)==0){
                nMessages++;
                // printf("----\n");
            }
        }
        fclose(fp);
        printf("Number of messages : %d\n",nMessages);

        marked = (int*)malloc(nMessages*sizeof(int));
        octets = (int*)malloc(nMessages*sizeof(int));
        for(int i=0;i<nMessages;i++){
            marked[i] = 0;
            octets[i] = 0;
        }

        fp = fopen(filename,"r");
        int count = 0;
        // char temp[100];
        while(fgets(temp,sizeof(temp),fp)!=NULL){
            octets[count] += strlen(temp);
            if(strncmp(temp,".\r\n",3)==0) count++;
        }
        fclose(fp);
    }
     



    /************************************** TRANSACTION *****************************************/
    while(1){
        n = recvMsg(sockfd,buff,sizeof(buff),endSeq);
        if(n<0) return;

        char op[100];
        sprintf(op,"%s",buff);

        if(strncmp(op,"STAT",4)==0){
            // stat
            sprintf(buff,"+OK %d %d%s",nMessages,totalOctets,endSeq);
            send(sockfd,buff,strlen(buff),0);
        }
        else if(strncmp(op,"LIST",4)==0){
            // list
            sprintf(buff,"+OK %d messages (%d octets)%s",nMessages,totalOctets,endSeq);
            send(sockfd,buff,strlen(buff),0);
            for(int i=0;i<nMessages;i++){
                sprintf(buff,"%d %d%s",i+1,octets[i],endSeq);
                send(sockfd,buff,strlen(buff),0);
            }
        }
        else if(strncmp(op,"RETR",4)==0){
            int num = 0;
            sscanf(op,"RETR %d",&num);
            if(num<1 || num>nMessages || marked[num-1]==1){
                // error
                sprintf(buff,"-ERR no such message, only %d messages in maildrop%s",nMessages,endSeq);
                send(sockfd,buff,strlen(buff),0);
                break;
            }
            // sprintf(buff,"+OK %d octets%s",octets[num-1],endSeq);
            sprintf(buff,"+OK%s",endSeq);
            send(sockfd,buff,strlen(buff),0);
            FILE* fp = fopen(filename,"r");
            int count = 0;
            char temp[100];
            while(fgets(temp,sizeof(temp),fp)!=NULL){
                if(count==num-1){
                    send(sockfd,temp,strlen(temp),0);
                }
                if(strncmp(temp,".\r\n",3)==0) count++;
                if(count==num) break;
            }
        }
        else if(strncmp(op,"DELE",4)==0){
            // dele
            int num = 0;
            sscanf(op,"DELE %d",&num);
            if(num<1 || num>nMessages || marked[num-1]==1){
                // error
                sprintf(buff,"-ERR no such message, only %d messages in maildrop%s",nMessages,endSeq);
                send(sockfd,buff,strlen(buff),0);
                break;
            }
            marked[num-1] = 1;
            sprintf(buff,"+OK message %d deleted%s",num,endSeq);
            send(sockfd,buff,strlen(buff),0);
        }
        else if(strncmp(op,"TOP",3)==0){
            // top 
            int num = 0,nlines = 0;
            sscanf(op,"TOP %d %d",&num,&nlines);
            if(num<1 || num>nMessages || marked[num-1]==1){
                // error
                sprintf(buff,"-ERR no such message, only %d messages in maildrop%s",nMessages,endSeq);
                send(sockfd,buff,strlen(buff),0);
                break;
            }
            
            sprintf(buff,"+OK%s",endSeq);
            send(sockfd,buff,strlen(buff),0);

            FILE* fp = fopen(filename,"r");
            int count = 0;
            char temp[100];
            while(count<num-1 && fgets(temp,sizeof(temp),fp)!=NULL){
                if(strncmp(temp,".\r\n",3)==0) count++;
                // if(count==num-1) break;
            }
            // from 
            fgets(buff,sizeof(buff),fp);
            // printf("%ld %s",strlen(buff),buff);
            send(sockfd,buff,strlen(buff),0);

            // ignore to
            fgets(buff,sizeof(buff),fp);
            // printf("%ld %s",strlen(buff),buff);
            send(sockfd,buff,strlen(buff),0);
            
            // subject
            fgets(buff,sizeof(buff),fp);
            // printf("%ld %s",strlen(buff),buff);
            send(sockfd,buff,strlen(buff),0);

            // received
            fgets(buff,sizeof(buff),fp);
            // printf("%ld %s",strlen(buff),buff);
            send(sockfd,buff,strlen(buff),0);

            // blank line for separation
            sprintf(buff,"%s",endSeq);
            send(sockfd,buff,strlen(buff),0);

            for(int i=0;i<nlines;i++){
                fgets(buff,sizeof(buff),fp);
                if(strncmp(buff,".\r\n",3)==0) break;
                // printf("%ld %s",strlen(buff),buff);
                send(sockfd,buff,strlen(buff),0);
            }
            sprintf(buff,".%s",endSeq);
            send(sockfd,buff,strlen(buff),0);

            fclose(fp);

        }
        else if(strncmp(op,"RSET",4)==0){
            // rset
            for(int i=0;i<nMessages;i++){
                if(marked[i]==1){
                    marked[i] = 0;
                }
            }
            sprintf(buff,"+OK maildrop has %d messages (%d octets)%s",nMessages,totalOctets,endSeq);
            send(sockfd,buff,strlen(buff),0);
        }
        else if(strncmp(op,"QUIT",4)==0){
            // quit

            char tempfilename[200];
            strcpy(tempfilename,filename);
            strncat(tempfilename,".tmp\0",5);

            FILE* fp = fopen(filename,"r");
            FILE* newfp = fopen(tempfilename,"w");
            
        
            int count = 0;
            char temp[100];
            while(fgets(temp,sizeof(temp),fp)!=NULL){
                if(marked[count]==1){
                    //ignore
                }
                else{
                    fprintf(newfp,"%s",temp);
                }
                if(strncmp(temp,".\r\n",3)==0) count++;
            }

            fclose(fp);
            fclose(newfp);
            remove(filename);
            rename(tempfilename,filename);
        
        

            sprintf(buff,"+OK %s POP3 server signing off%s",domain,endSeq);
            send(sockfd,buff,strlen(buff),0);
            break;
        }
        else{
            // error
            sprintf(buff,"-ERR Invalid Command%s",endSeq);
            send(sockfd,buff,strlen(buff),0);
            break;
        }

    }


    return;

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

			connectClient(newsockfd);
			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}
	return 0;
}
			
