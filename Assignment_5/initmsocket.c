#include "msocket.h"


void filler(int *arr,int val,int nbits){
    while(nbits--){
        *arr = val%2;
        val = val/2;
        arr++;
    }
}

// creates the header by combining the type, frame number and recv window

char getHeader(int type, int frame_num, int recv_window){
    int bits[8]; // 76543210
    bits[7] = type;

    // bits[6]; //frame number
    // bits[5];
    // bits[4];
    // bits[3];
    filler(&bits[3], frame_num, 4);

    // bits[2]; // recv window size
    // bits[1];
    // bits[0];
    filler(&bits[0], recv_window, 3);
    char header = 0;
    for(int i=0;i<8;i++){
        header = header | bits[i]<<i;
    }
    return header;
}

// extracts the type, frame number and recv window from the header

void extract(char header, int *type, int *frame_num, int *recv_window){
    int bits[8];
    for(int i=0;i<8;i++){
        bits[i] = (header>>i)&1;
    }
    *type = bits[7];
    *frame_num = 0;
    *recv_window = 0;
    for(int i=3;i<7;i++){
        *frame_num = *frame_num | bits[i]<<(i-3);
    }
    for(int i=0;i<3;i++){
        *recv_window = *recv_window | bits[i]<<(i);
    }
}

void createSharedMemory(){
    int semId = semget(ftok(SEM_KEY,1), 3, IPC_CREAT|0666);
    int shmId = shmget(ftok(SM_KEY,1), N*sizeof(struct mtp_sock), IPC_CREAT|0666);
    int passerId = shmget(ftok(PASSER_KEY,1), sizeof(struct SOCK_INFO), IPC_CREAT|0666);

    // set the initial values of the semaphores
    semctl(semId, MUTEX, SETVAL, 1);
    semctl(semId, SEM1, SETVAL, 0);
    semctl(semId, SEM2, SETVAL, 0);

    // attach the shared memory
    struct mtp_sock *SM = (struct mtp_sock *)shmat(shmId, NULL, 0);
    for(int i=0;i<N;i++){
        SM[i].state = FREE;
        SM[i].pid = 0;
        SM[i].sockid = 0;
        for(int j=0;j<SEND_BUFF_SIZE;j++){
            SM[i].send_buff[j].num = -1;
            SM[i].send_buff[j].ack = 0;
        }
    }

    struct SOCK_INFO *passer = (struct SOCK_INFO *)shmat(passerId, NULL, 0);
    passer->sockid = 0;
    passer->port = 0;
    passer->error_no = 0;
    
    
}

void deleteSharedMemory(){
    int semId = semget(ftok(SEM_KEY,1), 3, 0);
    int shmId = shmget(ftok(SM_KEY,1), N*sizeof(struct mtp_sock), 0);
    int passerId = shmget(ftok(PASSER_KEY,1), sizeof(struct SOCK_INFO), 0);

    semctl(semId, 0, IPC_RMID, 0);
    shmctl(shmId, IPC_RMID, 0);
    shmctl(passerId, IPC_RMID, 0);
}

void signalHandler(int sig){
    printf("\n🔴 Keyboard interrupt\n");
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);
    for(int i=0;i<N;i++){
        if(SM[i].state==BOUND){
            close(SM[i].sockid);
        }
    }
    deleteSharedMemory();
    exit(0);
}

void* Receiver(){
    printf("Receiver thread up\n");
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);
    

    fd_set readfds;
    struct timeval timeout;
    timeout.tv_sec = RECEIVER_TIMEOUT_SEC;
    timeout.tv_usec = 0;

    while(1){
        wait_sem(semId, MUTEX, 1);
        FD_ZERO(&readfds);
        int maxi = 0;
        for(int i=0;i<N;i++){
            if(SM[i].state==BOUND){
                FD_SET(SM[i].sockid, &readfds);
                if(SM[i].sockid>maxi){
                    maxi = SM[i].sockid;
                }
            }
        }
        signal_sem(semId, MUTEX, 1);
        
        int ret = select(maxi+1, &readfds, NULL, NULL, &timeout);
        printf("\t\tselect returned %d\n",ret);
        if(ret==0 || (timeout.tv_sec==0 && timeout.tv_usec==0)){
            timeout.tv_sec = RECEIVER_TIMEOUT_SEC;
            timeout.tv_usec = 0;
            wait_sem(semId, MUTEX, 1);
                printf("\t\tmutex acquired\n");
                for(int i=0;i<N;i++){
                    if(SM[i].state!=BOUND){
                        continue;
                    }
                    printf("\t\t%d => %d updating recv_window size\n",i,SM[i].sockid);
                    int last_frame_num = SM[i].r_start-1;
                    printf("\t\ttype = %d, frame_num = %d, recv_window = %d\n",ACK_BIT, last_frame_num, SM[i].r_end-SM[i].r_start+1);

                    int last_frame_bits = last_frame_num%FRAME_NUM_WIDTH;
                    char header = getHeader(ACK_BIT, last_frame_bits, SM[i].r_end-SM[i].r_start+1);
                    sendto(SM[i].sockid, &header, 1, 0, (struct sockaddr *)&SM[i].remoteaddr, sizeof(SM[i].remoteaddr));
                }
                printf("\t\t------------RRR-------------\n");
            signal_sem(semId, MUTEX, 1);
            continue;
        }
        else if(ret==-1){
            continue;
        }
        else{
            wait_sem(semId, MUTEX, 1);
                printf("\t\tmutex acquired\n");
                // process receiver and acknowledgment
                for(int i=0;i<N;i++){
                    if(SM[i].state!=BOUND || FD_ISSET(SM[i].sockid, &readfds)==0){
                        continue;
                    }
                    printf("\t\tfound data on %d => %d\n",i,SM[i].sockid);
                    
                    char buff[MESSAGE_SIZE+1];
                    struct sockaddr_in remoteaddr;
                    int remotelen=sizeof(remoteaddr);
                    // one message of UDP can only be received by only one recvfrom call
                    // if the message is larger than the buffer, it will be truncated and you cannot call recvfrom again to get the remaining message
                    int n = recvfrom(SM[i].sockid, buff, MESSAGE_SIZE+1, 0, (struct sockaddr *)&remoteaddr, &remotelen);
                    printf("\t\tn = %d\n ",n);
                    if(n==-1){
                        continue;
                    }

                    if(dropMessage(DROP_PROBABILITY)){
                        printf("\t\tMessage dropped\n");
                        continue;
                    }

                    {
                        // compare the remote address
                        if(addr_cmp(&SM[i].remoteaddr, &remoteaddr) != 0){
                            printf("\t\tAddress mismatch\n");
                            continue;
                        }
                    }

                    int type, frame_num, recv_window;
                    extract(buff[0], &type, &frame_num, &recv_window);
                    printf("\t\ttype = %d, frame_num = %d, recv_window = %d\n",type, frame_num, recv_window);
                    
                    if(type==MESSAGE_BIT){
                        int idx = -1;
                        for(int j=SM[i].r_start;j<=SM[i].r_end;j++){
                            int loc = j%RECV_BUFF_SIZE;
                            if(SM[i].recv_buff[loc].num%FRAME_NUM_WIDTH == frame_num){
                                idx = loc;
                                break;
                            }
                        }
    
                        printf("\t\tMsg received for sock: %d idx: %d\n",i,idx);
                        remotelen = sizeof(remoteaddr);
                        // n = recvfrom(SM[i].sockid, buff+1, 1, 0, (struct sockaddr *)&remoteaddr, &remotelen);
                        printf("\t\tn = %d\n",n);
                        if(n==-1){
                            continue;
                        }
                        
                        // send acknowledgement even if not in window or already received
                        
                        if(idx==-1 || SM[i].recv_buff[idx].ack==1){ // message is already received or not in receive window
    
                        }
                        else{
                            strncpy(SM[i].recv_buff[idx].msg, &buff[1], MESSAGE_SIZE);
                            SM[i].recv_buff[idx].ack = 1;
                            while(1){
                                int loc = SM[i].r_start%RECV_BUFF_SIZE;
                                if(SM[i].recv_buff[loc].ack==0 || SM[i].recv_buff[loc].num==-1 || SM[i].r_start>SM[i].r_end){
                                    break;
                                }
                                SM[i].r_start++;
                            }
                        }
                        char header = getHeader(ACK_BIT, frame_num, SM[i].r_end-SM[i].r_start+1);
                        sendto(SM[i].sockid, &header, 1, 0, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr));
                    }

                    if(type==ACK_BIT){
                        int idx=-1;
                        for(int j=SM[i].s_start;j<=SM[i].s_end;j++){
                            int loc = j%SEND_BUFF_SIZE;
                            if(SM[i].send_buff[loc].num%FRAME_NUM_WIDTH == frame_num){
                                idx = loc;
                                break;
                            }
                        }
                        if(idx!=-1){
                            printf("\t\tACK received for sock: %d idx: %d\n",i,idx);
                            SM[i].send_buff[idx].ack = 1;
                            int infinite=10; //-----------------------------------------------------------------------------------------------------------------------
                            while(1){
                                int loc = SM[i].s_start%SEND_BUFF_SIZE;
                                if(SM[i].send_buff[loc].ack==0 || SM[i].send_buff[loc].num==-1){
                                    break;
                                }
                                SM[i].send_buff[loc].num = -1;
                                SM[i].s_start++;
                            }
                        }
                        int infinite = 10;
                        while(1){
                            if(SM[i].s_end-SM[i].s_start+1 < recv_window){
                                SM[i].s_end++;
                            }
                            else{
                                break;
                            }
                        }
                    }
                    
                    
                }
                printf("\t\t------------RRR-------------\n");
            signal_sem(semId, MUTEX, 1);
        }
    }
    pthread_exit(NULL);
}


void* Sender(){
    printf("Sender thread up\n");
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);

    time_t last_time = time(NULL);
    int sleepTime = SENDER_TIMEOUT_SEC/3;
    if(sleepTime<1){
        sleepTime = 1;
    }
    while(1){
        sleep(sleepTime);
        time_t current_time = time(NULL);

        if(current_time-last_time<=SENDER_TIMEOUT_SEC){
            continue;
        }
        
        printf("\tSender woke up\n");

        wait_sem(semId, MUTEX, 1);
            printf("\tmutex acquired\n");  
            for(int i=0;i<N;i++){
                if(SM[i].state!=BOUND){
                    continue;
                }
                for(int j=SM[i].s_start;j<=SM[i].s_end;j++){
                    int loc = j%SEND_BUFF_SIZE;
                    if(SM[i].send_buff[loc].num==-1 || SM[i].send_buff[loc].ack==1) continue;
                    // create the header and send the message
                    char buff[MESSAGE_SIZE+1];

                    int frame_num = SM[i].send_buff[loc].num%FRAME_NUM_WIDTH;
                    buff[0] = getHeader(MESSAGE_BIT, frame_num, 0);

                    strncpy(&buff[1], SM[i].send_buff[loc].msg, MESSAGE_SIZE);
                    printf("\tsending message found on %d: %.5s\n",loc,&buff[1]);
                    printf("\ttype = %d, frame_num = %d, recv_window = %d\n",MESSAGE_BIT, frame_num, 0);
                    sendto(SM[i].sockid, buff, MESSAGE_SIZE+1, 0, (struct sockaddr *)&SM[i].remoteaddr, sizeof(SM[i].remoteaddr));

                }
            }
            last_time = time(NULL);
            printf("\t------------SSS-------------\n");
        signal_sem(semId, MUTEX, 1);
    }
    pthread_exit(NULL);
}

void* Garbage(){
    printf("Garbage thread up\n");
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);

    while(1){
        sleep(GARBAGE_REFRESH);
        wait_sem(semId, MUTEX, 1);

        for(int i=0;i<N;i++){
            if(SM[i].state==FREE){
                continue;
            }
            if(SM[i].pid==0){
                SM[i].state = FREE;
                close(SM[i].sockid);
                continue;
            }
            if(kill(SM[i].pid, 0)==-1){
                SM[i].state = FREE;
                SM[i].pid = 0;
                printf("\t\t\tGarbage %d => %d\n",i,SM[i].sockid);
                close(SM[i].sockid);
                SM[i].sockid = -1;
            }
        }

        signal_sem(semId, MUTEX, 1);
    }
    pthread_exit(NULL);
}


int main(){

    srand(time(0));

    createSharedMemory();
    
    pthread_t sid,rid,gid;
    // create threads
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&sid, &attr, Sender, NULL);
    pthread_create(&rid, &attr, Receiver, NULL);
    pthread_create(&gid, &attr, Garbage, NULL);
    
    signal(SIGINT, signalHandler);

    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    int semId;
    attachSharedMemory(&SM, &passer, &semId);
    
    while(1){
        wait_sem(semId, SEM1, 1);
            printf("Signal received on SEM1\n");
            if(passer->sockid==0 && passer->port==0){   // socket creation
                // create a new socket;
                printf("Request for new socket\n");
                int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                int error_no = errno;
                if(sockfd==-1){
                    passer->sockid = -1;
                    passer->error_no = error_no;
                }
                else{
                    passer->sockid = sockfd;
                }
                printf("Socket Succesful\n");
            }
            else if(passer->port>=0){   // bind case 
                printf("Request for bind\n");
                struct sockaddr_in myaddr;
                myaddr.sin_family = AF_INET;
                myaddr.sin_addr.s_addr = INADDR_ANY;
                myaddr.sin_port = htons(passer->port);
                int ret = bind(passer->sockid, (struct sockaddr *)&myaddr, sizeof(myaddr));
                int error_no = errno;
                if(ret==-1){
                    passer->sockid = -1;
                    passer->error_no = error_no;
                }
                {
                    printf("Socket %d binded to port %d\n", passer->sockid,passer->port);
                }
            }
            else{   // close case
                printf("Request for close\n");
                int ret = close(passer->sockid);
                int error_no = errno;
                if(ret==-1){
                    passer->sockid = -1;
                    passer->error_no = error_no;
                }
            }
            printf("------------MAIN----------------\n");
        signal_sem(semId, SEM2, 1);
    }
    return 0;
}