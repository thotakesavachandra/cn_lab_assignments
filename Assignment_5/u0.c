#include "msocket.h"
#ifndef MYID
    #define MYID 0;{printf("Compile by -DMYID=<user_id>\n");exit(1);}
#endif

int count = 4;
int txn_count = 100;
int send_sockid, recv_sockid;
FILE* fp;
int readFd;

int getport(int base, int sender,int receiver){
    // sender--;
    // receiver--;
    int port = base;
    port += (sender%count)*10 + (receiver%count);
    return port;
}

void get_addr(struct sockaddr_in *addr, int port){
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    inet_aton("127.0.0.1", &addr->sin_addr);
}

void signalhandler(int sig){
    printf("\n🔴 Keyboard interrupt\n");
    m_close(send_sockid);
    m_close(recv_sockid);
    fclose(fp);
    close(readFd);
    exit(0);
}


int main(){
    int id = MYID;
    int send_other = (id+1)%count;
    int recv_other = (id-1+count)%count;

    int send_my_port = getport(10000,id,send_other);
    int send_other_port = getport(20000,id,send_other);
    struct sockaddr_in send_my_addr, send_other_addr;
    get_addr(&send_my_addr,send_my_port);
    get_addr(&send_other_addr,send_other_port);   

    printf("sending port connections %d => %d\n",send_my_port,send_other_port);

    int recv_my_port = getport(20000,recv_other,id);
    int recv_other_port = getport(10000,recv_other,id);
    struct sockaddr_in recv_my_addr, recv_other_addr;
    get_addr(&recv_my_addr,recv_my_port);
    get_addr(&recv_other_addr,recv_other_port);

    printf("receiving port connections other(%d) => me(%d)\n",recv_other_port,recv_my_port);



    send_sockid = m_socket(AF_INET,SOCK_MTP,IPPROTO_UDP);
    m_bind(send_sockid,(struct sockaddr *)&send_other_addr,sizeof(send_other_addr),(struct sockaddr *)&send_my_addr,sizeof(send_my_addr));
    

    recv_sockid = m_socket(AF_INET,SOCK_MTP,IPPROTO_UDP);
    m_bind(recv_sockid,(struct sockaddr *)&recv_other_addr,sizeof(recv_other_addr),(struct sockaddr *)&recv_my_addr,sizeof(recv_my_addr));

    char send_buff[1025],recv_buff[1025];
    int send_count = 0;
    int recv_count = 0;
    int remotelen = sizeof(send_other_addr);

    signal(SIGINT,signalhandler);

    char outfilename[100];
    sprintf(outfilename,"./output_files/u%d_recv.txt",id);
    fp = fopen(outfilename,"w");

    char infilename[100];
    sprintf(infilename,"./test_files/%c.txt",'A'+id);
    readFd = open(infilename,O_RDONLY);

    int prev_read = -1;
    // int total_read = 0;

    int infinite = 1;
    while(1){
        if(send_count >= txn_count && recv_count >= txn_count){
            break;
        }
        int flag = 0;
        if(send_count < txn_count){
            if(send_count > prev_read){
                // sprintf(send_buff,"%d=>%d : %02d",id,send_other,send_count+1);
                int n = read(readFd,send_buff,1024);
                prev_read = send_count;
            }
            if(m_sendto(send_sockid,send_buff,1024,0,(struct sockaddr *)&send_other_addr, remotelen) < 0){
                int error_no = errno;
                printf("send %d failed {%s}\n",send_count+1,strerror(error_no));
            }
            else{
                // printf("sent me\n",send_buff);
                printf("⚪ sent message %d\n",send_count+1);
                send_count++;
                flag++;
            }
        }
        if(recv_count < txn_count){
            int n;
            n = m_recvfrom(recv_sockid,recv_buff,1024,0,(struct sockaddr *)&recv_other_addr,&remotelen);
            if(n < 0){
                int error_no = errno;  
                printf("receive %d failed {%s} \n",recv_count+1,strerror(error_no));
            }
            else{
                recv_buff[n] = '\0';
                // printf("%s\n",recv_buff);
                printf("🟢 received %d\n",recv_count+1);
                fprintf(fp,"%s",recv_buff);
                recv_count++;
                flag++;
            }
        }
        if(flag==0) sleep(2);
    }
    printf("\n\nPress enter to exit\n");
    getchar();

    fclose(fp);
    close(readFd);
    m_close(send_sockid);
    m_close(recv_sockid);


}