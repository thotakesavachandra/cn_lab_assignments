#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

typedef struct mail
{
    char s_name[100];
    char rec_time[100];
    char sub[100];
    int marked;
} mails;
char endSeq[] = "\r\n";

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
    // printf("Received message: %s",buff);
    return totRecv;
}

int recvCarefully(int sockfd,char buff[],int size,const char* pattern){
    int totRecv = 0;
    int len = strlen(pattern);
    while(1){
        int curr = recv(sockfd,buff+totRecv,1,0); 
        if(curr<0) return -1;
        // printf("%d",curr); fflush(stdout);
        totRecv += curr;
        if(totRecv>=len && strncmp(buff+totRecv-len,pattern,len)==0){
            // printf("breaking %d\n",totRecv);
            break;
        }
    }
    buff[totRecv]='\0';
    // printf("Received message: %s\n",buff);
    return totRecv;

}

void compare(char a[], char b[], int sockfd)
{
    if (strncmp(a, b, strlen(b)))
    {
        printf("ERROR: %s", a);
        close(sockfd);
        exit(0);
    }
}

int extractLines(int sockfd,char msg[][100],int size){
    int nlines=0;
    char buff[size*100];
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

int main(int args, char *argv[])
{
    if (args != 4)
    {
        printf("not enough arguments or more arguments\n");
        printf("Required arguments: <server-ip> <smtp-port> <pop3-port>\n");
        exit(1);
    }

    int sockfd;
    struct sockaddr_in serv_addr;

    int i;
    char buf[100];

    int choice;
    serv_addr.sin_family = AF_INET;
    inet_aton(argv[1], &serv_addr.sin_addr);
    int port = atoi(argv[2]);

    int port1 = atoi(argv[3]);

    /* With the information specified in serv_addr, the connect()
       system call establishes a connection with the server process.
    */
    char username[100], password[100];
    printf("enter username and password\n");
    scanf("%s", username);
    scanf("%s", password);

    while (1)
    {
        int r;
        printf("-------------------------------------------\n");
        printf("Enter your choice\n");
        printf("1 - Manage mail\n");
        printf("2 - Send mail\n");
        printf("3 - Quit\n");
        printf("-------------------------------------------\n");
        scanf("%d", &choice);
        // getchar();
        if (choice == 3)
        {
            close(sockfd);
            exit(0);
        }
        if (choice == 1)
        {
            // printf("your mails :\n");
            serv_addr.sin_port = htons(port1);
            if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Unable to create socket\n");
                exit(0);
            }
            if ((connect(sockfd, (struct sockaddr *)&serv_addr,
                         sizeof(serv_addr))) < 0)
            {
                perror("Unable to connect to server\n");
                exit(0);
            }
            printf("server connected...\n");
            char buff[100], inp[1000];
            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "+OK", sockfd);

            
            strcpy(buff, "USER ");
            strcat(buff, username);

            send(sockfd, buff, strlen(buff), 0);
            send(sockfd, "\r\n", 2, 0);

            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "+OK", sockfd);

            strcpy(buff, "PASS ");
            strcat(buff, password);

            send(sockfd, buff, strlen(buff), 0);
            send(sockfd, "\r\n", 2, 0);

            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "+OK", sockfd);

            strcpy(buff, "STAT");

            send(sockfd, buff, strlen(buff), 0);
            send(sockfd, "\r\n", 2, 0);

            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "+OK", sockfd);
            int nmails, smails;
            sscanf(buff, "+OK %d %d", &nmails, &smails);

            mails *m;

            m = (mails *)malloc(nmails * sizeof(mails));


            for (int i = 0; i < nmails; i++)
            {
                // strcpy(buff, "TOP");
                sprintf(buff, "TOP %d 0", (i + 1));

                send(sockfd, buff, strlen(buff), 0);
                send(sockfd, "\r\n", 2, 0);

                // special case receive only first 3 letters and endsequence
                r = recvCarefully(sockfd, buff,sizeof(buff),endSeq);
                compare(buff, "+OK", sockfd);

                char headers[500][100];
                int nlines = extractLines(sockfd,headers,500);

                // printf("SL.No : %d\n", (i + 1));

                // printf("\t%s",headers[0]);
                // printf("%s",headers[1]);
                // printf("\t%s",headers[2]);
                // printf("\t%s",headers[3]);
                

                strcpy(m[i].s_name, headers[0]);
                strcpy(m[i].sub, headers[2]);
                strcpy(m[i].rec_time, headers[3]);
                // printf("---------------------\n");
                // for(int j=0;j<strlen(m[i].rec_time);j++){
                //     printf("%d,",m[i].rec_time[j]);
                // }
                // printf("\n------------------------\n");
                
            }
            while (1)
            {
                for(int i=0;i<nmails;i++){
                    if(m[i].marked==1){
                        continue;
                    }
                    printf("Sl. No. : %d\n", (i + 1));
                    printf("\t%s",m[i].s_name);
                    printf("\t%s",m[i].sub);
                    printf("\t%s",m[i].rec_time);
                    printf("\n");
                }

                int sn;
                printf("ENTER YOUR SERIAL NUMBER : ");
                scanf("%d", &sn);
                getchar();

                if (sn == -1)
                    break;
                if (sn > nmails || sn <= 0)
                {
                    printf("enter a valid serial number >0 and <= %d\n", nmails);
                    continue;
                }

                // sprintf(buff,"STAT",sn);

                sprintf(buff, "RETR %d", sn);

                send(sockfd, buff, strlen(buff), 0);
                send(sockfd, "\r\n", 2, 0);

                r = recvCarefully(sockfd, buff,strlen(buff),endSeq);
                compare(buff, "+OK", sockfd);
                // char mail[1000];
                {
                    char mail[100][100];
                    int nlines = extractLines(sockfd,mail,100);
                    // printf("Mail:\n");
                    printf("-------------------------------------------\n");
                    for(int i=0;i<nlines;i++){
                        printf("%s",mail[i]);
                    }
                    printf("-------------------------------------------\n");
                }
                char t;

                t = getchar();
                // printf("t is %d %c \n", t,t);
                getchar();

                if (t == 'd')
                {
                    sprintf(buff, "DELE %d", sn);
                    if (m[sn - 1].marked == 1)
                    {
                        printf("mail is already marked for delete\n");
                    }
                    else
                    {
                        m[sn - 1].marked = 1;
                        send(sockfd, buff, strlen(buff), 0);
                        send(sockfd, "\r\n", 2, 0);

                        r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
                        compare(buff, "+OK", sockfd);
                    }

                }
                // printf("Got to end of 1 terations\n");
            }

            strcpy(buff, "QUIT");

            send(sockfd, buff, strlen(buff), 0);
            send(sockfd, "\r\n", 2, 0);

            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "+OK", sockfd);

            close(sockfd);
            printf("bye for now\n");
        }
        if (choice == 2)
        {
            serv_addr.sin_port = htons(port);
            if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                perror("Unable to create socket\n");
                exit(0);
            }
            if ((connect(sockfd, (struct sockaddr *)&serv_addr,
                         sizeof(serv_addr))) < 0)
            {
                perror("Unable to connect to server\n");
                exit(0);
            }
            printf("server connected...\n");
            char buff[100], inp[1000];
            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "220", sockfd);
            // printf("%s\n", buff);
            i = 4;
            while (buff[i] != '>')
            {
                i++;
            }

            char domain[50];
            strncpy(domain, buff + 5, i - 5);
            domain[i - 5] = '\0';
            sprintf(buff, "HELO %s", domain);
            // printf("%s\n", buff);
            // strcpy(buff,"HELO ");

            // strcat(buff,domain);
            // printf("buff - %s")
            send(sockfd, buff, strlen(buff), 0);
            send(sockfd, "\r\n", 2, 0);

            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "250", sockfd);
            // printf("%s\n", buff);

            printf("enter your msg\n");
            char lines[100][100];
            int lineCount = 0;
            while (1)
            {
                memset(buff, '\0', sizeof(buff));
                fflush(stdin);
                fgets(buff, sizeof(buff), stdin);
                char *end = strchr(buff, '\n');
                if (end != NULL)
                    *end = '\0';
                strcpy(lines[lineCount++], buff);
                if (!strcmp(buff, "."))
                    break;
            }
            // printf("NUMBEROF lines :%d\n",lineCount);
            i = 0;
            int j = 0;
            // char *lines[100]; // Assuming a maximum of 100 lines
            // int lineCount = 0;

            // char *token = strtok(inp, "\n");

            // while (token != NULL)
            // {
            // 	lines[lineCount + 1] = malloc(strlen(token) + 1); // Allocate memory for each line
            // 	strcpy(lines[lineCount + 1], token);
            // 	lineCount++;
            // 	// printf("%s\n",lines[lineCount-1]);
            // 	token = strtok(NULL, "\n");
            // }

            char fr[7];
            strncpy(fr, lines[1], 6);
            fr[7] = '\0';
            // printf("\n---%.*s---\n",6,fr);
            if (strcmp(fr, "From: "))
            {
                printf("1 - should be in the form of \'From: <your mail id>@<domain>\'\n");
                exit(0);
            }
            j = 6;
            i = 6;
            while (lines[1][j] != '@')
            {
                while (lines[1][j] == ' ')
                {
                    i++;
                    j++;
                }
                if (lines[1][j] == ' ')
                {
                    printf("no spaces in username");
                    exit(0);
                }
                j++;
                if (j == strlen(lines[1]))
                {
                    printf("2 - @ not found");
                    exit(0);
                }
            }
            char sender[100];
            strncpy(sender, lines[1] + i, j - i);
            sender[j - i] = '\0';
            // printf("sender -%s \n",sender);
            char msg2[100] = "MAIL FROM: <";
            char dsender[100];
            strncpy(dsender, lines[1] + j + 1, strlen(lines[1]) - j - 1);
            strcat(msg2, sender);
            strcat(msg2, "@");
            strcat(msg2, dsender);
            strcat(msg2, ">");

            char to[5];
            strncpy(to, lines[2], 4);
            to[4] = '\0';
            // printf("----%s--\n",to);
            if (strcmp(to, "To: "))
            {
                printf("3 -reciever should be like \'To: <reciever id>@<domain>\n");
                exit(0);
            }
            j = 4;
            i = 4;
            while (lines[2][j] != '@')
            {
                while (lines[2][j] == ' ')
                {
                    i++;
                    j++;
                }
                if (lines[2][j] == ' ')
                {
                    printf("no spaces in username\n");
                    exit(0);
                }
                j++;
                if (j == strlen(lines[2]))
                {
                    printf("2 - @ not found\n");
                    exit(0);
                }
            }
            char recver[100];
            strncpy(recver, lines[2] + i, j - i);
            recver[j - i] = '\0';

            char drecver[100];
            strncpy(drecver, lines[2] + j + 1, strlen(lines[2]) - j - 1);
            drecver[strlen(lines[2]) - j - 1] = '\0';
            // printf("%d %s\n", j, recver);
            char subject[100];
            char msg3[100] = "RCPT TO: <";
            strcat(msg3, recver);
            strcat(msg3, "@");
            strcat(msg3, drecver);
            strcat(msg3, ">");
            // printf("%s\n", msg3);
            if (strncmp(lines[3], "Subject: ", 9))
            {
                printf("5-sbject should be like \' Subject: \'\n");
                exit(0);
            }
            // strcpy(subject,lines[3]+9);
            // printf("subject - %s\n",subject);
            send(sockfd, msg2, strlen(msg2), 0);
            send(sockfd, "\r\n", 2, 0);
            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "250", sockfd);
            // printf("%s\n", buff);

            send(sockfd, msg3, strlen(msg3), 0);
            send(sockfd, "\r\n", 2, 0);
            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "250", sockfd);
            // printf("%s\n", buff);

            strcpy(msg2, "DATA");
            send(sockfd, msg2, strlen(msg2), 0);
            send(sockfd, "\r\n", 2, 0);

            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "354", sockfd);
            // printf("%s\n", buff);

            for (int i = 1; i < lineCount; i++)
            {
                // printf("%s\n", lines[i]);
                send(sockfd, lines[i], strlen(lines[i]), 0);
                send(sockfd, "\r\n", 2, 0);
            }
            // send(sockfd, ".", 1, 0);
            // send(sockfd, "\r\n", 2, 0);

            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "250", sockfd);
            // printf("%s\n", buff);
            printf("Mail sent successfully 😊😊\n");
            strcpy(msg3, "QUIT");
            send(sockfd, msg3, strlen(msg3), 0);
            send(sockfd, "\r\n", 2, 0);

            r = recvMsg(sockfd, buff,sizeof(buff),endSeq);
            compare(buff, "221", sockfd);
            // printf("%s\n", buff);

            // strcpy(msg3,"<client hangs up>");
            // send(sockfd, msg3, strlen(msg3), 0);
            // send(sockfd, "\r\n", 2, 0);

            close(sockfd);
        }
    }
}
