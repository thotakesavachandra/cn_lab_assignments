#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 30000
#define SERVER_ADDR "127.0.0.1"
#define MAX_NAME_SIZE 201

#define MAX_ENTITIES 10
#define MAX_CONC_CLIENTS 5

#define FREE 0
#define OCCUPIED 1

const char* success_msg = "Vote Registered Successfully";
const char* fail_msg = "Problem in voting. Try again later"; 


// There is a small mistake in the vote receiving code at server side. 
// The user choice is directly getting compared with the entity names.
// It should not be a problem as the entities are reset whenever a entity is deleted. 
// But if the client sends an empty string as his choice then a vote is getting added to the uninitialised entity where the name is empty string.
// This is happening at 'for loop' in line 221.
// checking whether a entity is initialized before comparing the name will solve the problem. 
// Adding this line "if(entities[j].state == FREE) continue;" as 1st statement in 'for loop' will solve this problem.
