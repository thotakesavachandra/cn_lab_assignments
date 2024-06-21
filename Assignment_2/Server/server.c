/*
			NETWORK PROGRAMMING WITH SOCKETS

In this program we illustrate the use of Berkeley sockets for interprocess
communication across the network. We show the communication between a server
process and a client process.


*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


int isDigit(char c){
	return (c>='0' && c<='9');
}

char encrypt(char c,int k){
	if(c>='a' && c<='z'){
		return (c-'a'+k)%26 + 'a';
	}
	else if(c>='A' && c<='Z'){
		return (c-'A'+k)%26 + 'A';
	}
	else return c;
}


			/* THE SERVER PROCESS */

int main()
{
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;

	/* The following system call opens a socket. The first parameter
	   indicates the family of the protocol to be followed. For internet
	   protocols we use AF_INET. For TCP sockets the second parameter
	   is SOCK_STREAM. The third parameter is set to 0 for user
	   applications.
	*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	/* The structure "sockaddr_in" is defined in <netinet/in.h> for the
	   internet family of protocols. This has three main fields. The
 	   field "sin_family" specifies the family and is therefore AF_INET
	   for the internet family. The field "sin_addr" specifies the
	   internet address of the server. This field is set to INADDR_ANY
	   for machines having a single IP address. The field "sin_port"
	   specifies the port number of the server.
	*/
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(20000);

	/* With the information provided in serv_addr, we associate the server
	   with its port using the bind() system call. 
	*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5); 
	

	while (1) {

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}
		printf("Accepted client %s:%d\n",inet_ntoa(cli_addr.sin_addr),cli_addr.sin_port);


		/* Having successfully accepted a client connection, the
		   server now forks. The parent closes the new socket
		   descriptor and loops back to accept the next connection.
		*/
		if (fork() == 0) {

			close(sockfd);

			char inFileName[100],outFileName[100];
			sprintf(inFileName,"%u.%d.txt",cli_addr.sin_addr.s_addr,cli_addr.sin_port);
			sprintf(outFileName,"%u.%d.txt.enc",cli_addr.sin_addr.s_addr,cli_addr.sin_port);

			int inFd = open(inFileName,O_CREAT|O_WRONLY|O_TRUNC,0644);
			int outFd = open(outFileName,O_CREAT|O_WRONLY|O_TRUNC,0644);

			char buff[100];
			int n;
			int found_k = 0,k_val=0;
			while(1){
				n = recv(newsockfd, buff, 1,0);		
				if(isDigit(buff[0]) && !found_k){
					k_val = k_val*10 + (buff[0]-'0');
				}
				else if(buff[0] == '$'){
					break;
				}
				else{
					found_k = 1;
					// if(buff[0] == '\n') printf("newline%c",buff[0]);
					write(inFd,buff,n);
				}
			}
			close(inFd);
			printf("Received input file %s from %s:%d\n",inFileName,inet_ntoa(cli_addr.sin_addr),cli_addr.sin_port);
			
			inFd = open(inFileName,O_RDONLY);
			while(1){
				n = read(inFd,buff,1);
				if(n==0) break;
				buff[0] = encrypt(buff[0],k_val);
				write(outFd,buff,n);
			}
			close(inFd);
			close(outFd);
			outFd = open(outFileName,O_RDONLY);
			while(1){
				n = read(outFd,buff,100);
				if(n==0){
					send(newsockfd,"$",1,0);
					break;
				}
				send(newsockfd,buff,n,0);
			}
			close(outFd);
			close(newsockfd);
			exit(0);
		}

		close(newsockfd);
	}
	return 0;
}
			

