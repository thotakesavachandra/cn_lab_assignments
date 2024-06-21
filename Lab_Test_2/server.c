#include "headers.h"

struct entity_info{
    int state;
    char name[MAX_NAME_SIZE];
    int votes;
};


struct cli_info{
    int state;
    int sockfd;
    char choice[MAX_NAME_SIZE];
    int n_received;
};

struct entity_info entities[MAX_ENTITIES];
struct cli_info clients[MAX_CONC_CLIENTS];



int max(int a, int b){
    if(b>a) return b;
    return a;
}

void print_admin_menu(){
    printf("1) List of entities and their votes\n");
    printf("2) Add a new entity\n");
    printf("3) Delete an entity\n");
    printf("Enter choice: ");
    fflush(stdout);
}

void reset_client(struct cli_info* cli){
    cli->state = FREE;
    cli->sockfd = -1;
    cli->n_received = 0;
    memset(cli->choice, 0, sizeof(cli->choice));
}

void reset_entity(struct entity_info* ent){
    ent->state = FREE;
    ent->votes = 0;
    memset(ent->name, 0, sizeof(ent->name));
}



void serve_admin(){
    int choice;
    scanf("%d", &choice);
    getchar();
    printf("\n");
    if(choice==1){
        printf("Current Standings\n");
        for(int i=0; i<MAX_ENTITIES; i++){
            if(entities[i].state==FREE) continue;
            printf("%s => %d\n", entities[i].name, entities[i].votes);
        }
    }
    else if(choice==2){
        printf("Enter name : ");
        for(int i=0; i<MAX_ENTITIES; i++){
            if(entities[i].state == OCCUPIED) continue;
            fgets(entities[i].name, MAX_NAME_SIZE, stdin);
            *strchr(entities[i].name, '\n') = '\0';
            entities[i].state = OCCUPIED;
            printf("New entity %s\n",entities[i].name);
            break;
        }
    }
    else if(choice==3){
        for(int i=0; i<MAX_ENTITIES; i++){
            if(entities[i].state==FREE) continue;
            printf("%s => %d\n", entities[i].name, entities[i].votes);
        }
        printf("Enter name : ");
        char name[MAX_NAME_SIZE];
        fgets(name, MAX_NAME_SIZE, stdin);
        *strchr(name, '\n') = '\0';

        int flag = 0;
        
        for(int i=0; i<MAX_ENTITIES; i++){
            if(entities[i].state == FREE) continue;
            
            if(strcmp(entities[i].name, name)==0){
                flag = 1;
                reset_entity(& entities[i]);
                break;
            }
        }
        if(flag){
            printf("Deleted %s\n", name);
        }
        else{
            printf("No person with name %s\n", name);
        }
    }
    else{
        printf("Bad Choice !! \n");
    }
    printf("-----------------------------------------\n");
}



int add_new_client(int newsockfd){
    for(int i=0; i<MAX_CONC_CLIENTS; i++){
        if(clients[i].state == FREE){
            clients[i].state = OCCUPIED;
            clients[i].sockfd = newsockfd;
            return i;
        }
    }
    return -1;

}




int main(){
    printf("--------------- Server ------------------\n");
    
    struct sockaddr_in servaddr, cliaddr;
    int servlen = sizeof(servaddr);
    int clilen;

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int r = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(r<0){
        perror("Socket Creation Failed\n");
        exit(EXIT_FAILURE);
    }
    int vote_req = r;

    r = bind(vote_req, (struct sockaddr*)&servaddr, servlen);
    if(r<0){
        perror("Bind Failed\n");
        exit(EXIT_FAILURE);
    }

    listen(vote_req, 5);

    
    {
        for(int i=0; i<MAX_CONC_CLIENTS; i++){
            reset_client(&clients[i]);
        }
        for(int i=0; i<MAX_ENTITIES; i++){
            reset_entity(&entities[i]);
        }
    }

    fd_set readfds;
    print_admin_menu();

    while(1){
        int maxfd = 0;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO , &readfds);
        FD_SET(vote_req, &readfds);
        maxfd = max(maxfd, vote_req);
        for(int i=0; i<MAX_CONC_CLIENTS; i++){
            if(clients[i].state == OCCUPIED){
                FD_SET(clients[i].sockfd, & readfds);
                maxfd = max(maxfd, clients[i].sockfd);
            }
        }
        int r = select(maxfd+1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(STDIN_FILENO, &readfds)){
            serve_admin();
            print_admin_menu();
        }

        if(FD_ISSET(vote_req, &readfds)){
            int newsockfd = accept(vote_req, NULL, NULL);
            int r = add_new_client(newsockfd);
            if(r < 0){
                printf("\nDropping client. Server Full\n");
                close(newsockfd);
            }
            struct cli_info* cli = &clients[r];
            int count = 0;
            for(int j=0; j<MAX_ENTITIES; j++){
                if(entities[j].state==FREE) continue;
                count++;
            }
            count = htonl(count);
            send(cli->sockfd, (char*)&count, 4, 0);
            for(int j=0; j<MAX_ENTITIES; j++){
                if(entities[j].state==FREE) continue;
                send(cli->sockfd, entities[j].name, strlen(entities[j].name)+1, 0);
            }
        }

        for(int i=0; i<MAX_CONC_CLIENTS; i++){
            if(clients[i].state==FREE) continue;
            if(FD_ISSET(clients[i].sockfd, &readfds)){
                struct cli_info* cli = &clients[i];
                int r = recv(cli->sockfd, cli->choice+cli->n_received, MAX_NAME_SIZE, 0);
                cli->n_received += r;
                if(r==0){
                    // connection closed by client
                    close(cli->sockfd);
                    reset_client(cli);
                    continue;
                }

                if(*(cli->choice+cli->n_received-1)=='\0'){
                    // try to add vote
                    int flag = 0;
                    for(int j=0; j<MAX_ENTITIES; j++){
                        if(entities[j].state == FREE) continue;
                        if(strcmp(entities[j].name, cli->choice)==0){
                            entities[j].votes++;
                            flag = 1;
                            break;

                        }
                    }
                    if(flag==1){
                        send(cli->sockfd, success_msg, strlen(success_msg)+1, 0);
                    }
                    else{
                        send(cli->sockfd, fail_msg, strlen(fail_msg)+1, 0);
                    }
                    close(cli->sockfd);
                    reset_client(cli);
                }
            }
        }    
    }
    
    return 0;
}