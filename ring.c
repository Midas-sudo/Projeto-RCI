#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

int BUFFER_SIZE = 256;

int main(int argc, char** argv)
{
    int node_i, node_port;
    char command[BUFFER_SIZE], *node_ip;
    
    printf("check");

    if(argc != 4){
        printf("Usage: ring i i.ip i.port");
        exit(0);
    }
    else{
        node_i = atoi(argv[1]);
        node_ip = argv[2];
        node_port = atoi(argv[3]);
    }

    printf("check");

    while(1){
        fgets(command, BUFFER_SIZE-1, stdin);
        printf("%s", command);
    }


}   


