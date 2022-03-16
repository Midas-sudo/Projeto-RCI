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
    char *succ_port;
    char *succ_ip;

    int chord_i;
    char *chord_port;
    char *chord_ip;

    int fd_pred;
    int fd_succ;
} Node_struct;

Node_struct *new_node(int i, char *ip, char *port)
{
    Node_struct *new_node;
    new_node = malloc(sizeof(Node_struct));
    new_node->self_i = i;
    new_node->self_ip = ip;
    new_node->self_port = port;
    return new_node;
}

int UDP_setup(char *port)
{
    struct sockaddr_in *address;
    int UDPfd, TCP_listenfd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;

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

int TCP_setup(char *port)
{
    int TCPfd, errcode;
    struct sockaddr_in *address;
    struct addrinfo hints, *res;
    socklen_t addrlen;
    ssize_t n;

    // create TCP socket
    TCPfd = socket(AF_INET, SOCK_STREAM, 0);
    if (TCPfd == -1) /*error*/
        exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // get localhost ip address
    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0) /*Error*/
        exit(1);

    n = bind(TCPfd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*Error*/
        exit(1);

    return TCPfd;
}

int main(int argc, char **argv)
{
    int node_i, UDP_socket, TCP_socket, max_socket, error, n, new_fd;
    char command[BUFFER_SIZE], message[BUFFER_SIZE], *node_ip, *node_port;
    FILE *fp;
    Node_struct *node;
    fd_set available_sockets, ready_sockets; // Variáveis para guardar o conjunto de sockets
    struct sockaddr_in client_addr, tcp_client_addr;
    socklen_t client_addrlen, tcp_client_addrlen;

    int status = 0; // node is off

    // check arguments
    if (argc != 4)
    {
        printf("Usage: ring i i.ip i.port\n");
        exit(0);
    }
    else
    {
        node_i = atoi(argv[1]);
        node_ip = argv[2];
        node_port = argv[3];
    }
    printf(">>>$");
    fgets(command, BUFFER_SIZE, stdin);

    while (strcmp(command, "new\n") != 0)
    {
        printf("The first command needs to be 'new' to create the new node.\n");
        memset(command, 0, BUFFER_SIZE);
        fgets(command, BUFFER_SIZE, stdin);
    }

    // create a new node in memory -> remember to free in the end
    node = new_node(node_i, node_ip, node_port);
    printf("created node on port: %s\n", node_port);

    // create sockets to listen (only needs port number as ip is localhost)
    UDP_socket = UDP_setup(node->self_port);
    TCP_socket = TCP_setup(node->self_port);

    // Initialização do conjunto de sockets a 0
    FD_ZERO(&available_sockets);

    // Definição das sockets de ligação e o stdin na lista de sockets a observar pelo select()

    FD_SET(UDP_socket, &available_sockets);
    // FD_SET(TCP_socket, &available_sockets);
    FD_SET(STDIN_FILENO, &available_sockets);

    // verifica qual a socket com id maior
    max_socket = UDP_socket > TCP_socket ? UDP_socket + 1 : TCP_socket + 1;
    printf("Waiting for connections...\n");
    while (1)
    {
        // Cópia de lista de sockets devido ao comportamento destrutivo do select()
        ready_sockets = available_sockets;

        error = select(max_socket, &ready_sockets, NULL, NULL, NULL);
        if (error < 0) /*ERROR*/
            exit(1);

        // UDP
        if (FD_ISSET(UDP_socket, &ready_sockets))
        {
            memset(message, '\0', BUFFER_SIZE);
            n = recvfrom(UDP_socket, message, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addrlen);
            write(1, message, sizeof(message));
            sendto(UDP_socket, (const char *)message, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, client_addrlen);
        }

        // TCP
        if (FD_ISSET(TCP_socket, &ready_sockets))
        {
            memset(message, '\0', BUFFER_SIZE);
            // accept connection from incoming socket and read it
            tcp_client_addrlen = sizeof(tcp_client_addr);
            printf("checking\n");
            if ((new_fd = accept(TCP_socket, (struct sockaddr *)&tcp_client_addr, &tcp_client_addrlen)) == -1)
                /*error*/ printf("error");
            if (n = read(new_fd, message, BUFFER_SIZE) == -1) /*error*/
                printf("error");                              // exit(1);
            printf("got: %s", message);
            close(new_fd);
        }

        // STDIN
        if (FD_ISSET(STDIN_FILENO, &ready_sockets))
        {
            memset(command, '\0', BUFFER_SIZE);
            fgets(command, BUFFER_SIZE, stdin);

            write(1, command, sizeof(command));
        }
        printf(".\n");
    }
}