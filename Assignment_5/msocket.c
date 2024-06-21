#include "msocket.h"

void wait_sem(int semId,int idx,int n){
    struct sembuf pop;
    pop.sem_num = idx;
    pop.sem_flg = 0;
    pop.sem_op = -n ;
    semop(semId, &pop, 1);
}

void signal_sem(int semId,int idx,int n){
    struct sembuf vop;
    vop.sem_num = idx;
    vop.sem_flg = 0;
    vop.sem_op = n ;
    semop(semId, &vop, 1);
}

int addr_cmp(const struct sockaddr_in *l, const struct sockaddr_in *r){
    if(l->sin_port != r->sin_port){
        return -1;
    }
    if(l->sin_addr.s_addr != r->sin_addr.s_addr){
        return -1;
    }
    return 0;
}


void attachSharedMemory(struct mtp_sock **SM, struct SOCK_INFO **passer,int *semId){
    *semId = semget(ftok(SEM_KEY,1), 3, 0);
    // semId[0] is for mutex;
    // semId[1] is for sem1
    // semId[2] is for sem2

    int shmId = shmget(ftok(SM_KEY,1), N*sizeof(struct mtp_sock), 0);
    *SM = (struct mtp_sock *)shmat(shmId, NULL, 0);

    int passerId = shmget(ftok(PASSER_KEY,1), sizeof(struct SOCK_INFO), 0);
    *passer = (struct SOCK_INFO *)shmat(passerId, NULL, 0);
}


void detachSharedMemory(struct mtp_sock **SM, struct SOCK_INFO **passer, int *semId){
    shmdt(*SM);
    shmdt(*passer);
}

int dropMessage(float p){
    // srand(time(NULL));
    int pool = 100;
    float r = rand()%pool + 1;
    {
        FILE* fp = fopen("rand.txt","a");
        fprintf(fp,"%f %f\n",r,r/pool);
        fclose(fp);
    }
    r = r/pool;
    if(r<p){
        return 1;
    }
    return 0;
}


void reset_passer(struct SOCK_INFO *passer){
    passer->sockid = 0;
    passer->port = 0;
    passer->error_no = 0;
}


int m_socket(int domain, int type, int protocol){
    printf("m_socket called\n");
    if(type!=SOCK_MTP){
        errno = EPROTOTYPE;
        return -1;
    }
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);

    wait_sem(semId, MUTEX, 1); 
    for(int i=0;i<N;i++){
        if(SM[i].state==FREE){
            {
                passer->sockid = 0;
                passer->port = 0;
                passer->error_no = 0;
            }
            signal_sem(semId, SEM1, 1);
            wait_sem(semId, SEM2, 1);
            if(passer->sockid==-1){
                int error_no = passer->error_no;
                reset_passer(passer);
                signal_sem(semId, MUTEX, 1);
                detachSharedMemory(&SM, &passer, &semId);
                errno = error_no;
                return -1;
            }
            SM[i].state = ALLOCATED;
            SM[i].sockid = passer->sockid;
            SM[i].pid = getpid();
            reset_passer(passer);
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            return i;
        }
    }
    reset_passer(passer);
    signal_sem(semId, MUTEX, 1);
    detachSharedMemory(&SM, &passer, &semId);
    errno = ENOBUFS;
    return -1;
}


int m_bind(int fd, const struct sockaddr *remoteaddr, socklen_t remoteaddr_len, const struct sockaddr *myaddr, socklen_t  myaddr_len){
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);
    
    wait_sem(semId, MUTEX, 1);
    {// checking for errors
        if(fd<0 || fd>=N || SM[fd].pid!=getpid() || SM[fd].state!=ALLOCATED){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EBADF;
            return -1;
        }
        if(remoteaddr==NULL || remoteaddr_len!=sizeof(struct sockaddr_in)){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EINVAL;
            return -1;
        } 
        if(myaddr==NULL || myaddr_len!=sizeof(struct sockaddr_in)){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EINVAL;
            return -1;
        }
    }

    {
        passer->sockid = SM[fd].sockid;
        passer->port = ntohs(((struct sockaddr_in *)myaddr)->sin_port);
        passer->error_no = 0;
    }

    signal_sem(semId, SEM1, 1);
    wait_sem(semId, SEM2, 1);
    if(passer->sockid==-1){
        int error_no = passer->error_no;
        reset_passer(passer);
        signal_sem(semId, MUTEX, 1);
        detachSharedMemory(&SM, &passer, &semId);
        errno = error_no;
        return -1;
    }
    {
        SM[fd].state = BOUND;
        SM[fd].myaddr = *(struct sockaddr_in *)myaddr;
        SM[fd].remoteaddr = *(struct sockaddr_in *)remoteaddr;
        
        SM[fd].s_write = 1;
        SM[fd].s_start = 1;
        SM[fd].s_end = RECV_BUFF_SIZE;
        for(int i=SM[fd].s_start; i<=SM[fd].s_end; i++){
            int loc = i%SEND_BUFF_SIZE;
            SM[fd].send_buff[loc].num = -1;
        }

        SM[fd].r_read = 1;
        SM[fd].r_start = 1;
        SM[fd].r_end = RECV_BUFF_SIZE;
        for(int i=SM[fd].r_start; i<=SM[fd].r_end; i++){
            int loc = i%RECV_BUFF_SIZE;
            SM[fd].recv_buff[loc].num = i;
            SM[fd].recv_buff[loc].ack = 0;
        }
    }
    reset_passer(passer);
    signal_sem(semId, MUTEX, 1);
    detachSharedMemory(&SM, &passer, &semId);
    return 0;
}


ssize_t m_sendto(int fd, const void *buff, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len){
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);
    
    wait_sem(semId, MUTEX, 1);
    {// checking errors
        if(fd<0 || fd>=N || SM[fd].pid!=getpid()){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EBADF;
            return -1;
        }
        if(addr==NULL || addr_len!=sizeof(struct sockaddr_in)){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EINVAL;
            return -1;
        }
        if(SM[fd].state!=BOUND || addr_cmp((struct sockaddr_in*)addr, &SM[fd].remoteaddr)!=0){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = ENOTCONN;
            return -1;
        }

    }
    
    // check if the space for writing is available. -1 incicates that the space is available.
    // after writing store the message number in the num field of the message.
    int loc = SM[fd].s_write%SEND_BUFF_SIZE;
    if(SM[fd].send_buff[loc].num != -1){
        signal_sem(semId, MUTEX, 1);
        detachSharedMemory(&SM, &passer, &semId);
        errno = ENOBUFS;
        return -1;
    }
    memset(SM[fd].send_buff[loc].msg, 0, sizeof(SM[fd].send_buff[loc].msg));
    strncpy(SM[fd].send_buff[loc].msg, buff, n);
    // printf("sendto called with message: %.5s\n",SM[fd].send_buff[loc].msg);
    SM[fd].send_buff[loc].num = SM[fd].s_write;
    SM[fd].send_buff[loc].ack = 0;
    SM[fd].s_write++;
    signal_sem(semId, MUTEX, 1);
    detachSharedMemory(&SM, &passer, &semId);
    return n;
}

ssize_t m_recvfrom(int fd, void *__restrict__ buff, size_t n, int flags, struct sockaddr *__restrict__ addr, socklen_t *__restrict__ addr_len){
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);
    
    wait_sem(semId, MUTEX, 1);
    {// checking for errors
        if(fd<0 || fd>=N || SM[fd].pid!=getpid()){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EBADF;
            return -1;
        }
        if(SM[fd].state!=BOUND || addr_cmp((struct sockaddr_in*)addr, &SM[fd].remoteaddr)!=0){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = ENOTCONN;
            return -1;
        }
    }
    
    // r_read will not read if it is in the receiving window. Only those that are spit out will be read.
    // after reading the num in message will be set to -1. so that the window end can be expanded by putting the expected frame numbers in the num field.

    int loc = SM[fd].r_read%RECV_BUFF_SIZE;
    if(SM[fd].recv_buff[loc].ack == 0 && SM[fd].recv_buff[loc].num != -1){
        signal_sem(semId, MUTEX, 1);
        detachSharedMemory(&SM, &passer, &semId);
        errno = ENOMSG;
        return -1;
    }
    // int loc = SM[fd].r_read%RECV_BUFF_SIZE;
    int size = n>strlen(SM[fd].recv_buff[loc].msg)?strlen(SM[fd].recv_buff[loc].msg):n;
    strncpy(buff, SM[fd].recv_buff[loc].msg, n);
    SM[fd].recv_buff[loc].num = -1;
    while(SM[fd].recv_buff[(SM[fd].r_end+1)%RECV_BUFF_SIZE].num == -1){
        SM[fd].r_end++;
        SM[fd].recv_buff[(SM[fd].r_end)%RECV_BUFF_SIZE].num = SM[fd].r_end;
        SM[fd].recv_buff[(SM[fd].r_end)%RECV_BUFF_SIZE].ack = 0;
    }
    SM[fd].r_read++;
    signal_sem(semId, MUTEX, 1);
    detachSharedMemory(&SM, &passer, &semId);
    return size;
}


/* OLD SEND_TO

ssize_t m_sendto(int fd, const void *buff, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len){
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);
    
    wait_sem(semId, MUTEX, 1);
    { // checking for errors
        if(fd<0 || fd>=N){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EBADF;
            return -1;
        }
        if(SM[fd].state!=BOUND || SM[fd].pid!=getpid()){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EBADF;
            return -1;
        }
        if(addr==NULL || addr_len!=sizeof(struct sockaddr_in)){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EINVAL;
            return -1;
        } 
        if(buff==NULL || n==0){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EINVAL;
            return -1;
        }
    }

    if(SM[fd].s_write>=SEND_BUFF_SIZE){
        signal_sem(semId, MUTEX, 1);
        detachSharedMemory(&SM, &passer, &semId);
        errno = ENOBUFS;
        return -1;
    }
    strncpy(SM[fd].send_buff[SM[fd].s_write].msg, buff, n);
    // SM[fd].send_buff[SM[fd].s_write].num = n;
    SM[fd].send_buff[SM[fd].s_write].ack = 0;
    SM[fd].s_write++;
     
}

*/

/* OLD RECV_FROM
ssize_t m_recvfrom(int fd, void *__restrict__ buff, size_t n, int flags, struct sockaddr *__restrict__ addr, socklen_t *__restrict__ addr_len){
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);
    
    wait_sem(semId, MUTEX, 1);
    { // checking for errors
    }

    if(SM[fd].r_read<=0){
        signal_sem(semId, MUTEX, 1);
        detachSharedMemory(&SM, &passer, &semId);
        errno = ENOMSG;
        return -1;
    }
    int size = n>strlen(SM[fd].recv_buff[SM[fd].r_read].msg)?strlen(SM[fd].recv_buff[SM[fd].r_read].msg):n;
    strncpy(buff, SM[fd].recv_buff[SM[fd].r_read].msg, n);
    
    SM[fd].r_read--;
    for(int i=0;i<SM[fd].r_read;i++){
        SM[fd].recv_buff[i] = SM[fd].recv_buff[i+1];
    }
    signal_sem(semId, MUTEX,  1);
    wait_sem(semId, MUTEX, 1);
    { // checking for errors
    }
    detachSharedMemory(&SM, &passer, &semId);
    return size;
}

*/

int m_close(int fd){
    int semId;
    struct mtp_sock *SM;
    struct SOCK_INFO *passer;
    attachSharedMemory(&SM, &passer, &semId);
    
    wait_sem(semId, MUTEX, 1);
    { // checking for errors
        if(fd<0 || fd>=N || SM[fd].pid!=getpid()){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EBADF;
            return -1;
        }
        if(SM[fd].state==FREE){
            signal_sem(semId, MUTEX, 1);
            detachSharedMemory(&SM, &passer, &semId);
            errno = EBADF;
            return -1;
        }
    }
    
    SM[fd].state = FREE;
    {
        passer->sockid = SM[fd].sockid;
        passer->port = -1;
        passer->error_no = 0;
    }
    signal_sem(semId, SEM1, 1);
    wait_sem(semId, SEM2, 1);
    reset_passer(passer);
    signal_sem(semId, MUTEX, 1);
    detachSharedMemory(&SM, &passer, &semId);
    return 0;
}


