
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h>


void getInput(char fileName[],int * fd,int* k_val){
	while(1){
		printf("Enter the file name: ");
		scanf("%s",fileName);
		int checkfd = open(fileName,O_RDONLY);
		if(checkfd == -1){
			printf("File not found\n");
			continue;
		}
		else{
			*fd = checkfd;
			break;
		}	
	}
	while(1){
		printf("Enter the value of k: ");
		scanf("%d",k_val);
		if(*k_val < 1){
			printf("Invalid value of k\n");
			continue;
		}
		else{
			break;
		}
	}
}

int main(int argc,char* argv[])
{
	int			sockfd ;
	struct sockaddr_in	serv_addr;

	int i;


	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);

	/* With the information specified in serv_addr, the connect()
	   system call establishes a connection with the server process.
	*/
	char fileName[100];
	int fd,k_val;
	char buff[100];

	while(1){

		getInput(fileName,&fd,&k_val);
		
		if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("Unable to create socket\n");
			exit(0);
		}

		if ((connect(sockfd, (struct sockaddr *) &serv_addr,
							sizeof(serv_addr))) < 0) {
			perror("Unable to connect to server\n");
			exit(0);
		}


		sprintf(buff,"%d",k_val);
		send(sockfd,buff,strlen(buff),0);

		while(1){
			int nread = read(fd,buff,100);
			if(nread == 0){
				send(sockfd,"$",1,0);
				break;
			}
			send(sockfd,buff,nread,0);
		}
		close(fd);
		printf("File sent\n");

		char newFileName[100];
		strcpy(newFileName,fileName);
		strcat(newFileName,".enc");
		int newFd = open(newFileName,O_WRONLY | O_CREAT,0666);
		while(1){
			int nread = recv(sockfd,buff,1,0);
			if(buff[0] == '$'){
				break;
			}
			write(newFd,buff,nread);
		}
		close(newFd);
		close(sockfd);

		printf("Received encrypted file\n");
		printf("Original File : %s\n",fileName);
		printf("Encrypted File : %s\n\n",newFileName);

	}
		
	return 0;

}

