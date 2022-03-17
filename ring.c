#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#define RED "\E[1;31m"
#define GRN "\E[32m"
#define MAG "\E[0;35m"
#define CYN "\E[36m"
#define RESET "\E[0;0m"
#define IST "\E[38;2;0;157;224m"
#define YLW "\E[38;2;255;216;0m"
#define ORG "\E[38;2;255;144;0m"
#define GRAY "\E[38;2;64;64;64m"
#define INPUT "\E[0;35m>>> \E[0m"

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

void command_list(int i, char *ip, char *port)
{
    printf(IST "┌─Node─Information─and─Command─List───────────────────────────────────────────────────┐\n");
    printf("│" RESET "\E[1m  New node created with:\E[87G\E[0m" IST "│\n");
    printf("│" RESET "   ~Key:  " GRN "%d\E[87G" IST "│\n", i);
    printf("│" RESET "   ~IP:   " GRN "%s\E[87G" IST "│\n", ip);
    printf("│" RESET "   ~Port: " GRN "%s\E[87G" IST "│\n", port);
    printf("│\E[87G│\n");
    printf("│" RESET "\E[1m  Command List:\E[87G\E[0m" IST "│\n");
    printf("│" RESET "   (Every command can also be called by its initial only)\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" YLW "    new\E[87G" IST "│\n");
    printf("│" RESET "     -Creates a new ring containing only this node;\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" YLW "    bentry" ORG " boot boot.IP boot.port\E[87G" IST "│\n");
    printf("│" RESET "     -Entry of this node on " ORG "boot" RESET "'s node ring without knowing its predecessor;\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" YLW "    pentry" ORG " pred pred.IP pred.port\E[87G" IST "│\n");
    printf("│" RESET "     -Entry of this node on " ORG "pred" RESET "'s node ring using " ORG "pred" RESET " as its predecessor;\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" YLW "    chord" ORG " i i.IP i.port\E[87G" IST "│\n");
    printf("│" RESET "     -Creation of a shortcut to node " ORG "i" RESET " (each node can only have one shortcut);\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" YLW "    echord\E[87G" IST "│\n");
    printf("│" RESET "     -Deletion of current shortcut;\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" YLW "    show\E[87G" IST "│\n");
    printf("│" RESET "     -Shows current information of node. This information is divided in 4 parts:\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" RESET "        (i)  Key, IP Address, Port;\E[87G" IST "│\n");
    printf("│" RESET "       (ii)  Key, IP Address, Port of node sucessor;\E[87G" IST "│\n");
    printf("│" RESET "      (iii)  Key, IP Address, Port of node predecessor;\E[87G" IST "│\n");
    printf("│" RESET "       (iv)  Key, IP Address, Port of node shortcut;\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" YLW "    find " ORG "k\E[87G" IST "│\n");
    printf("│" RESET "     -Searches for the key " ORG "k" RESET " on the ring returning it's key, IP\E[87G" IST "│\n"); 
    printf("│" RESET "      address, and port;\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" YLW "    leave\E[87G" IST "│\n");
    printf("│" RESET "     -Exit of this node from current ring;\E[87G" IST "│\n");
    printf("│\E[87G│\n");
    printf("│" YLW "    exit\E[87G" IST "│\n");
    printf("│" RESET "     -Closes aplication; \E[87G" IST "│\n");
    printf("│" GRAY "      (The abriviation of this command only works if there is currently no shortcut)\E[87G" IST "│\n");
    //printf("│" GRAY "                                  " IST "│\n");
    printf("└─────────────────────────────────────────────────────────────────────────────────────┘\n" RESET);
    return;
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

    if (listen(TCPfd, 4) == -1)
        exit(1);

    return TCPfd;
}

int self_send(char **args, Node_struct *node)
{
    int Prev_node_fd, errcode, n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char message[BUFFER_SIZE];

    Prev_node_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (Prev_node_fd == -1) /*ERROR*/
        exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(args[0], args[2], &hints, &res);
    if (errcode != 0) /*ERROR*/
        exit(1);

    n = connect(Prev_node_fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*ERROR*/
        exit(1);

    sprintf(message, "SELF %d %s %s\n", node->self_i, node->self_ip, node->self_port);
    n = write(Prev_node_fd, message, sizeof(message));

    return Prev_node_fd;
}

void self_recieve()
{
}

// prompts user for a command (and its args) and returns the number of args read
int get_command(char** args){ 
    char buffer[BUFFER_SIZE];
    memset(args[0], 0, BUFFER_SIZE);
    memset(args[1], 0, BUFFER_SIZE);
    memset(args[2], 0, BUFFER_SIZE);
    memset(args[3], 0, BUFFER_SIZE);
    printf(MAG ">>> " RESET);
    fgets(buffer, BUFFER_SIZE, stdin);
    return sscanf(buffer, "%s %s %s %s", args[0], args[1], args[2], args[3]);
}

// checks if a command is valid aka if its new or pentry or bentry with the right args
// returns 0 for "new", 1 for "pentry" and 2 for "bentry"
int check_first_command(char **args, int num_read)
{
    if(strcmp(args[0], "new") == 0 || strcmp(args[0], "n") == 0)
        return 0;

    if (strcmp(args[0], "pentry") == 0 || strcmp(args[0], "p") == 0)
    {
        if(num_read != 4){
            printf(YLW "Usage: pentry key key.ip key.port\n" RESET);
            return -1;
        }
        printf("pentry args: %s %s %s\n", args[1], args[2], args[3]);
        return 1;
    }
    if (strcmp(args[0], "bentry") == 0 || strcmp(args[0], "b") == 0)
    {   
        if(num_read != 4){
            printf(YLW "Usage: bentry key key.ip key.port\n" RESET);
            return -1;
        }
        printf("bentry args: %s %s %s\n", args[1], args[2], args[3]);
        return 2;
    }
    printf(RED "\"%s\" is not a valid command.\n" RESET, args[0]);
    return -1;
}

int main(int argc, char **argv)
{
    int node_i, UDP_socket, TCP_socket, TCP_Prev_socket, max_socket, error, n, new_fd;
    char command[BUFFER_SIZE], message[BUFFER_SIZE], *node_ip, *node_port;
    FILE *fp;
    Node_struct *node;
    fd_set available_sockets, ready_sockets; // Variáveis para guardar o conjunto de sockets
    struct sockaddr_in client_addr, tcp_client_addr;
    socklen_t client_addrlen, tcp_client_addrlen;
    char ** args;

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

    printf(IST "┌─Projecto─de─Redes─de─Computadores─e─Internet─────────────┐\n");
    printf("│" RESET "Base de Dados em Anel com Cordas         Ano Letivo 21/22 " IST "│\n");
    printf("│                                                          │\n");
    printf("│" RESET "Autores:                                                  " IST "│\n");
    printf("│" RESET "    •Gonçalo Midões -------------------------- ist196219  " IST "│\n");
    printf("│" RESET "    •Ravi Regalo ----------------------------- ist196305  " IST "│\n");
    printf("└──────────────────────────────────────────────────────────┘\n\n" RESET);

    // create a new node in memory -> remember to free in the end
    node = new_node(node_i, node_ip, node_port);
    // prompt user for entering  a command and shows command list
    command_list(node_i, node_ip, node_port);

    // ** Don't forget to free in the end!!!
    args = malloc(4*sizeof(char*));
    for(int i=0;i<4;i++){
        args[i]= malloc(BUFFER_SIZE*sizeof(char));
    }

    int num_read = get_command(args);
    while(check_first_command(args, num_read) == -1){
        num_read = get_command(args);
    }
    
    // create sockets to listen (only needs port number as ip is localhost)
    UDP_socket = UDP_setup(node->self_port);
    TCP_socket = TCP_setup(node->self_port);

    // Initialização do conjunto de sockets a 0
    FD_ZERO(&available_sockets);

    // Definição das sockets de ligação e o stdin na lista de sockets a observar pelo select()

    FD_SET(UDP_socket, &available_sockets);
    FD_SET(TCP_socket, &available_sockets);
    FD_SET(STDIN_FILENO, &available_sockets);

    // verifica qual a socket com id maior
    max_socket = UDP_socket > TCP_socket ? UDP_socket + 1 : TCP_socket + 1;
    printf("Waiting for connections or command input\n");
    write(1, INPUT, sizeof(INPUT));
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

            memset(&client_addr, '\0', sizeof(struct sockaddr_in));
            client_addrlen = sizeof(client_addr);
            n = recvfrom(UDP_socket, message, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addrlen);
            write(1, message, sizeof(message));
            if (sendto(UDP_socket, "ACK", 3, 0, (struct sockaddr *)&client_addr, client_addrlen) < 0)
            {
                printf("TEMP Send failed\n");
                exit(1);
            };
        }

        // TCP
        if (FD_ISSET(TCP_socket, &ready_sockets))
        {
            memset(message, '\0', BUFFER_SIZE);
            // accept connection from incoming socket and read it
            tcp_client_addrlen = sizeof(tcp_client_addr);
            if ((new_fd = accept(TCP_socket, (struct sockaddr *)&tcp_client_addr, &tcp_client_addrlen)) == -1)
                exit(0);
            if (n = read(new_fd, message, BUFFER_SIZE) == -1) /*error*/
                exit(1);
            printf("Received: %s\n", message);
            close(new_fd);
        }

        // STDIN
        // TODO: tem que se remover o new pentry e bentry porque uma vez
        // usado um desses nunca mais se pode usar (até pelo menos ao leave)
        if (FD_ISSET(STDIN_FILENO, &ready_sockets))
        {
            char *cmd;
            char **args;

            memset(command, '\0', BUFFER_SIZE);
            fgets(command, BUFFER_SIZE, stdin);

            switch (command[0])
            {
            case 'n':
                break;
            case 'b':
                args = malloc(3 * sizeof(char *));
                args[0] = malloc(3 * sizeof(char));
                args[1] = malloc(BUFFER_SIZE * sizeof(char));
                args[2] = malloc(6 * sizeof(char));

                sscanf(command, "%s %s %s %s", cmd, args[0], args[1], args[2]);

                for (int i = 0; i < 3; i++)
                    free(args[i]);
                free(args);
                break;
            case 'p':
                args = malloc(3 * sizeof(char *));
                args[0] = malloc(3 * sizeof(char));
                args[1] = malloc(BUFFER_SIZE * sizeof(char));
                args[2] = malloc(6 * sizeof(char));

                sscanf(command, "%s %s %s %s", cmd, args[0], args[1], args[2]);

                TCP_Prev_socket = self_send(args, node);

                FD_SET(TCP_Prev_socket, &available_sockets);
                max_socket = max_socket > TCP_Prev_socket ? max_socket : TCP_Prev_socket + 1;

                for (int i = 0; i < 3; i++)
                    free(args[i]);
                free(args);
                break;

            case 'c':
                args = malloc(3 * sizeof(char *));
                args[0] = malloc(3 * sizeof(char));
                args[1] = malloc(BUFFER_SIZE * sizeof(char));
                args[2] = malloc(6 * sizeof(char));

                sscanf(command, "%s %s %s %s", cmd, args[0], args[1], args[2]);

                for (int i = 0; i < 3; i++)
                    free(args[i]);
                free(args);
                break;

            case 'e':
                break;

            case 's':
                break;

            case 'f':
                args = malloc(1 * sizeof(char *));
                args[0] = malloc(3 * sizeof(char));

                sscanf(command, "%s %s", cmd, args[0]);

                free(args[0]);
                free(args);
                break;

            case 'l':

                break;
            case 'o':
                break;
            default:
                break;
            }

            write(1, INPUT, sizeof(INPUT));
        }
    }
}