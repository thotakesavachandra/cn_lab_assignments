#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <pthread.h>
#include <bits/pthreadtypes.h>
#include <signal.h>
#include <sys/select.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////
#define SOCK_MTP 7
#define N 25

#define MESSAGE_SIZE 1024

#define SEM_KEY "msocket.h"
#define MUTEX 0
#define SEM1 1
#define SEM2 2
#define SM_KEY "msocket.c"
#define FREE 0
#define ALLOCATED 1
#define BOUND 2
#define SEND_BUFF_SIZE 10
#define RECV_BUFF_SIZE 5

#define PASSER_KEY "initmsocket.c"

#define GARBAGE_REFRESH 10
#define RECEIVER_TIMEOUT_SEC 5
#define SENDER_TIMEOUT_SEC 5

#define MESSAGE_BIT 0
#define ACK_BIT 1

// 4 bit number to indicate frame number so %16
#define FRAME_NUM_WIDTH 16

#define DROP_PROBABILITY 0.5

////////////////////////////////////////////////////////////////////////////////


struct message{
    char msg[MESSAGE_SIZE];
    int num;
    int ack;    // for sender buffer indicates whether message sent is acknowledged by the other end
                // for receiver buffer indicates whether the follwing message is received or not
};

struct mtp_sock{
    int state;
    int pid;
    int sockid;
    struct sockaddr_in remoteaddr,myaddr;
    struct message send_buff[SEND_BUFF_SIZE];
    struct message recv_buff[RECV_BUFF_SIZE];
    int s_write;
    int s_start;
    int s_end;
    int r_read;
    int r_start;
    int r_end;
};

/* Usage of passer
    case newsocket:
        sockid = port = 0
    case bind:
        sockid = sockid of the socket
        port = port number to be bound to
    case close:
        sockid = sockid of the socket
        port = -1

*/


struct SOCK_INFO{
    int sockid;
    char ip[20];
    int port;
    int error_no;
};


int m_socket(int domain, int type, int protocol);

int m_bind(int fd, const struct sockaddr *remoteaddr, socklen_t remoteaddr_len, const struct sockaddr *myaddr, socklen_t  myaddr_len);

ssize_t m_sendto(int fd, const void *buf, size_t n, int flags, const struct sockaddr *addr, socklen_t addr_len);

ssize_t m_recvfrom(int fd, void *__restrict__ buf, size_t n, int flags, struct sockaddr *__restrict__ addr, socklen_t *__restrict__ addr_len);

int m_close(int fd);

int dropMessage(float p);


////////////////////////////////   MY FUNCTIONS   ///////////////////////////////////////////////

void wait_sem(int semId,int idx,int n);

void signal_sem(int semId,int idx,int n);

void attachSharedMemory(struct mtp_sock **SM, struct SOCK_INFO **passer,int *semId);

void detachSharedMemory(struct mtp_sock **SM, struct SOCK_INFO **passer, int *semId);

void passer_reset(struct SOCK_INFO *passer);

// returns 0 if the address are same. -1 otherwise
int addr_cmp(const struct sockaddr_in* a,const struct sockaddr_in* b);

////////////////////////////////////////////////////////////////////////////////////////////////
