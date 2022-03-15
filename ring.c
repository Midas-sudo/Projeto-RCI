#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

int BUFFER_SIZE = 256;
// char** command_list = ["new", "bentry", "pentry", "chord", "echord", "show", "find", "leave"];

typedef struct
{
    int self_i;
    char *self_port;
    char *self_ip;

    int succ_i;
    int succ_port;
    char *succ_ip;

    int chord_i;
    int chord_port;
    char *chord_ip;

    int fd_pred;
    int fd_succ;
} Node_struct;

Node_struct *node_constructor(int i, char *ip, char *port)
{
    Node_struct *new_node;
    new_node = malloc(sizeof(Node_struct));
    new_node->self_i = i;
    new_node->self_ip = ip;
    new_node->self_port = port;
    return new_node;
}

Node_struct* new (int i, char *ip, char *port)
{
    // talvez um pouco redundante chamar outra função, mas não sei se devia tentar-se por aqui a initialização das sockets

    Node_struct *node;
    return node = node_constructor(i, ip, port);
}

int UDP_setup(char *port)
{
    struct sockaddr_in *address;
    int UDPfd, TCP_listenfd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;

    UDPfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (UDPfd == -1) /*error*/
        exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0) /*Error*/
        exit(1);

    n = bind(UDPfd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*Error*/
        exit(1);

    return UDPfd;
}

int main(int argc, char **argv)
{
    int node_i, node_port;
    char command[BUFFER_SIZE], *node_ip;

    int status = 0; // node is off

    // check arguments
    if (argc != 4)
    {
        printf("Usage: ring i i.ip i.port");
        exit(0);
    }
    else
    {
        node_i = atoi(argv[1]);
        node_ip = argv[2];
        node_port = argv[3];
    }

    while (1)
    {
        fgets(command, BUFFER_SIZE, stdin);

        switch (command[0])
        {
        case 'n':
            new ();
            break;
        case 'b':
            bentry();
            break;
        case 'p':
            pentry();
            break;
        case 'c':
            chord();
            break;
        case 'e':
            echord();
            break;
        case 's':
            show();
            break;
        case 'f':
            find();
            break;
        case 'l':
            leave();
            break;
        default:
            exit(1);
            break;
        }
    }
}