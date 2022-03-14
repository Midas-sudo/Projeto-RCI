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
    int self_port;
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

Node_struct *node_constructor(int i, char *ip, int port)
{
    Node_struct *new_node;
    new_node = malloc(sizeof(Node_struct));
    new_node->self_i = i;
    new_node->self_ip = ip;
    new_node->self_port = port;
    return new_node;
}

int new (int i, char *ip, int port)
{
    node = node_constructor()
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
        node_port = atoi(argv[3]);
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