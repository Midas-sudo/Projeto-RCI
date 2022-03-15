#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
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
    int node_i, UDP_socket, TCP_socket, max_socket, error, n;
    char command[BUFFER_SIZE], message[BUFFER_SIZE], *node_ip, *node_port;

    Node_struct *node;
    fd_set available_sockets, ready_sockets; // Variáveis para guardar o conjunto de sockets
    struct sockaddr_in client_addr;
    socklen_t client_addrlen;

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
    fgets(command, BUFFER_SIZE, stdin);

    while (strcmp(command, "new\n") != 0)
    {
        printf("The first command needs to be 'new' to create the new node.\n");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
    }
    node = new (node_i, node_ip, node_port);

    UDP_socket = UDP_setup(node->self_port);
    // **Implementar função para socket de TCP

    // Initialização do conjunto de sockets a 0
    FD_ZERO(&available_sockets);

    // Definição das sockets de ligação e o stdin na lista de sockets a observar pelo select()

    FD_SET(UDP_socket, &available_sockets);
    // FD_SET(TCP_socket, &available_sockets);
    FD_SET(STDIN_FILENO, &available_sockets);

    max_socket = UDP_socket > STDIN_FILENO ? UDP_socket + 1 : STDIN_FILENO + 1; 
    // **Fazer verificação com a socket de TCP
    while (1)
    {
        // Cópia de lista de sockets devido ao comportamento destrutivo do select()
        ready_sockets = available_sockets;

        error = select(max_socket, &ready_sockets, NULL, NULL, NULL);

        if (error < 0) /*ERROR*/
            exit(1);

        if (FD_ISSET(UDP_socket, &ready_sockets))
        {
            memset(message, '\0', BUFFER_SIZE);

            n = recvfrom(UDP_socket, message, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addrlen);
            write(1, message, sizeof(message));
            sendto(UDP_socket, (const char *)message, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, client_addrlen);
        }

        /*if(FD_ISSET(TCP_socket, &ready_sockets)){


        }*/

        if (FD_ISSET(STDIN_FILENO, &ready_sockets))
        {
            memset(command, '\0', BUFFER_SIZE);
            fgets(command, BUFFER_SIZE, stdin);
            
            write(1, command, sizeof(command));
        }
    }

    
}