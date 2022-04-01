#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>

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
int verb = 0;
// char** command_list = ["new", "bentry", "pentry", "chord", "echord", "show", "find", "leave"];

typedef struct
{
    int self_i;
    char *self_port;
    char *self_ip;

    int succ_i;
    char *succ_port;
    char *succ_ip;

    int pred_i;
    char *pred_port;
    char *pred_ip;

    int chord_i;
    char *chord_port;
    char *chord_ip;

    int fd_pred;
    int fd_succ;
    int fd_cord;

    int is_online;
} Node_struct;

typedef struct
{
    int searched_key;
    int seq_number;
    struct sockaddr_in addr;
} UDP_addr_list;

Node_struct *new_node(int i, char *ip, char *port)
{
    Node_struct *new_node;
    new_node = malloc(sizeof(Node_struct));
    new_node->self_i = i;
    new_node->self_ip = (char *)malloc((strlen(ip) + 1) * sizeof(char));
    strcpy(new_node->self_ip, ip);
    new_node->self_port = (char *)malloc((strlen(port) + 1) * sizeof(char));
    strcpy(new_node->self_port, port);

    new_node->succ_i = -1;
    new_node->succ_ip = (char *)malloc((BUFFER_SIZE + 1) * sizeof(char));
    new_node->succ_port = (char *)malloc((BUFFER_SIZE + 1) * sizeof(char));

    new_node->pred_i = -1;
    new_node->pred_ip = (char *)malloc((BUFFER_SIZE + 1) * sizeof(char));
    new_node->pred_port = (char *)malloc((BUFFER_SIZE + 1) * sizeof(char));

    new_node->chord_i = -1;
    new_node->chord_ip = (char *)malloc((BUFFER_SIZE + 1) * sizeof(char));
    new_node->chord_port = (char *)malloc((BUFFER_SIZE + 1) * sizeof(char));

    new_node->fd_succ = -1;
    new_node->fd_pred = -1;
    new_node->fd_cord = -1;

    new_node->is_online = 0;
    return new_node;
}

void free_node(Node_struct *node)
{
    return;
}

void verbose(char *str)
{
    if (verb == 1)
        printf(YLW "%s\n" RESET, str);
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
    // printf("│" GRAY "                                  " IST "│\n");
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

    if (UDPfd == -1)
    { /*error*/
        printf(RED "Error creating UDP listen socket.\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0)
    { /*Error*/
        printf(RED "Error getting own adress info.\n");
        exit(1);
    }
    verbose("Got own address info");

    n = bind(UDPfd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
    { /*Error*/
        printf(RED "Error binding UDP port.\n");
        exit(1);
    }
    verbose("UDP port binded");

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
    if (TCPfd == -1)
    { /*error*/
        printf(RED "Error creating TCP listen socket.\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // get localhost ip address
    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0)
    { /*Error*/
        printf(RED "Error getting own adress info.\n");
        exit(1);
    }
    verbose("Got own address info");

    n = bind(TCPfd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
    { /*Error*/
        printf(RED "Error binding TCP port.\n");
        exit(1);
    }
    verbose("TCP port binded");

    if (listen(TCPfd, 4) == -1)
    { /*Error*/
        printf(RED "Error listening to TCP port.");
        exit(1);
    }
    return TCPfd;
}

int self_send(char **args, Node_struct *node)
{
    int Prev_node_fd, errcode, n_left, n_written;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char message[BUFFER_SIZE];

    Prev_node_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (Prev_node_fd == -1)
    { /*ERROR*/
        printf(RED "Error creating TCP connection socket.\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    errcode = getaddrinfo(args[2], args[3], &hints, &res);
    if (errcode != 0)
    { /*ERROR*/
        printf(RED "Error getting new connection address information.\n");
        exit(1);
    }
    verbose("Got connection address info");

    errcode = connect(Prev_node_fd, res->ai_addr, res->ai_addrlen);
    if (errcode == -1)
    { /*ERROR*/
        printf(RED "Error establishing connection.\n");
        exit(1);
    }
    verbose("Succefully established connection");

    sprintf(message, "SELF %d %s %s\n", node->self_i, node->self_ip, node->self_port);

    n_left = sizeof(message);
    while (n_left > 0)
    {
        n_written = write(Prev_node_fd, message, sizeof(message));
        if (n_written <= 0)
            exit(1);
        n_left -= n_written;
        *message += n_written;
    }

    verbose("TCP message sent");
    node->fd_pred = Prev_node_fd;
    node->pred_i = atoi(args[1]);
    // n percebo esta Segmentation fault
    strcpy(node->pred_ip, args[2]);
    strcpy(node->pred_port, args[3]);
    return Prev_node_fd;
}

void self_recieve(char **args, Node_struct *node, int fd)
{
    char message[BUFFER_SIZE];
    int n_left, n_written;
    printf("%d -- %d\n", atoi(args[1]), node->succ_i);
    if (!(node->succ_i == -1 || ((atoi(args[1]) - node->succ_i > 0 && node->succ_i > node->self_i) || atoi(args[1]) <= node->self_i)))
    {
        verbose("Predecessor exits.");
        sprintf(message, "PRED %s %s %s\n", args[1], args[2], args[3]);
        n_left = sizeof(message);
        while (n_left > 0)
        {
            n_written = write(node->fd_succ, message, sizeof(message));
            if (n_written <= 0)
            {
                /*ERROR*/
                printf(RED "Error sending TCP message to predecessor!");
                exit(1);
            }
            n_left -= n_written;
            *message += n_written;
            printf("PRED SENT: %s", message);
        }
        verbose("TCP message to predecessor written");
    }
    if (node->succ_i != -1)
    {
        /*Close old TCP connection, before storing new one.*/
        close(node->fd_succ);
    }
    node->succ_i = atoi(args[1]);
    strcpy(node->succ_ip, args[2]);
    strcpy(node->succ_port, args[3]);
    node->fd_succ = fd;
    verbose("Sucessor information stored to node");
}

void pred_send(Node_struct *node)
{
    int errcode, n_left, n_written;
    struct addrinfo hints, *res;
    char message[BUFFER_SIZE];

    sprintf(message, "PRED %d %s %s\n", node->pred_i, node->pred_ip, node->pred_port);

    n_left = sizeof(message);
    while (n_left > 0)
    {
        n_written = write(node->fd_succ, message, sizeof(message));
        if (n_written <= 0)
            exit(1);
        n_left -= n_written;
        *message += n_written;
    }

    // verbose("TCP message sent");
    printf("PRED message sent: %s", message);
    return;
}

void pred_recieve(char **args, Node_struct *node)
{
}

void response_recieve(char **args, Node_struct *node, UDP_addr_list addr)
{

    if (args[1] == node->self_i) /*This was the initial node*/
    {
        printf("Chave %d: nó %d (%d:%d).", addr->searched_key, args[3], args[4], args[5]);
    }
    else
    {
        int dist = distancia(node, args[1]);
        switch (dist)
        {
        case 1: /*Next node is closer to the initial node*/
            res_send_TCP();
            break;
        case 2: /*Chord node is closer to the initial node*/
            res_send_UDP();
            break;
        default:
            printf(RED "RSP logic failed\n");
            break;
        }
    }
}
void find_recieve(char **args, Node_struct *node, int dist)
{
    switch (dist)
    {
    case 0: /*This node is the owner of the searched key*/
        dist = distancia(node, args[1]);
        switch (dist)
        {
        case 1: /*Next node is closer to the initial node*/
            res_send_TCP();
            break;
        case 2: /*Chord node is closer to the initial node*/
            res_send_UDP();
            break;
        default:
            printf(RED "RSP logic failed\n");
            break;
        }
        break;
    case 1: /*Next node is closer to the searched key*/
        find_send_TCP();
        break;
    case 2: /*Chord node is closer to the searched key*/
        find_send_UDP();
        break;
    default:
        printf(RED "FND logic failed\n");
        break;
    }
}

void show(Node_struct *node)
{
    printf("NODE:\n\t key: %d\n\t ip: %s\n\t port: %s\n", node->self_i, node->self_ip, node->self_port);
    printf("PREDECESSOR:\n\t key: %d\n\t ip: %s\n\t port: %s\n", node->pred_i, node->pred_ip, node->pred_port);
    printf("SUCCESOR:\n\t key: %d\n\t ip: %s\n\t port: %s\n", node->succ_i, node->succ_ip, node->succ_port);
    if (node->chord_i != -1)
        printf("NODE:\n\t key: %d\n\t ip: %s\n\t port: %s\n", node->chord_i, node->chord_ip, node->chord_port);
}

// prompts user for a command (and its args) and returns the number of args read
int get_command(char **args)
{
    char buffer[BUFFER_SIZE];
    // memset(args[0], 0, BUFFER_SIZE);
    // memset(args[1], 0, BUFFER_SIZE);
    // memset(args[2], 0, BUFFER_SIZE);
    // memset(args[3], 0, BUFFER_SIZE);
    write(1, INPUT, sizeof(INPUT));
    fgets(buffer, BUFFER_SIZE, stdin);
    return sscanf(buffer, "%s %s %s %s %s", args[0], args[1], args[2], args[3], args[4]);
}

// checks if a command is valid
int check_command(char **args, int num_read, Node_struct *node)
{
    if (strcmp(args[0], "new") == 0 || strcmp(args[0], "n") == 0)
    {
        if (node->is_online)
        {
            printf(RED "Node is already part of a ring. First use \"leave\" to leave the ring\n" RESET);
            return -1;
        }
        return 0;
    }
    if (strcmp(args[0], "bentry") == 0 || strcmp(args[0], "b") == 0)
    {
        if (node->is_online)
        {
            printf(RED "Node is already part of a ring. First use \"leave\" to leave the ring\n" RESET);
            return -1;
        }
        if (num_read != 4)
        {
            printf(YLW "Usage: bentry key key.ip key.port\n" RESET);
            return -1;
        }
        return 1;
    }
    if (strcmp(args[0], "pentry") == 0 || strcmp(args[0], "p") == 0)
    {
        if (node->is_online)
        {
            printf(RED "Node is already part of a ring. First use \"leave\" to leave the ring\n" RESET);
            return -1;
        }
        if (num_read != 4)
        {
            printf(YLW "Usage: pentry key key.ip key.port\n" RESET);
            return -1;
        }
        return 2;
    }
    if (strcmp(args[0], "chord") == 0 || strcmp(args[0], "c") == 0)
    {
        if (!node->is_online)
        {
            printf(RED "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        if (num_read != 4)
        {
            printf(YLW "Usage: chord key key.ip key.port\n" RESET);
            return -1;
        }
        return 3;
    }
    if (strcmp(args[0], "dchord") == 0 || strcmp(args[0], "d") == 0 && node->chord_i != -1)
    {
        if (!node->is_online)
        {
            printf(RED "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        return 4;
    }
    if (strcmp(args[0], "show") == 0 || strcmp(args[0], "s") == 0)
    {
        if (!node->is_online)
        {
            printf(RED "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        return 5;
    }
    if (strcmp(args[0], "find") == 0 || strcmp(args[0], "f") == 0)
    {
        if (!node->is_online)
        {
            printf(RED "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        return 6;
    }
    if (strcmp(args[0], "leave") == 0 || strcmp(args[0], "l") == 0)
    {
        if (!node->is_online)
        {
            printf(RED "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        return 7;
    }
    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "e") == 0)
    {
        if (!node->is_online)
        {
            printf(RED "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        return 8;
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
    char **args;
    int num_read, mode;
    UDP_addr_list addr_list[100];
    int seq_number = 0;

    // check arguments
    if (argc != 4)
    {
        if (argc == 5)
        {
            node_i = atoi(argv[1]);
            node_ip = argv[2];
            node_port = argv[3];
            verb = 1;
        }
        else
        {

            verbose("Usage: ring i i.ip i.port\n");
            exit(0);
        }
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

    args = malloc(6 * sizeof(char *)); // ** Don't forget to free in the end!!!
    for (int i = 0; i < 6; i++)
    {
        args[i] = malloc(BUFFER_SIZE * sizeof(char));
    }

    num_read = get_command(args);
    mode = check_command(args, num_read, node);
    while (mode != 0 && mode != 1 && mode != 2)
    {
        num_read = get_command(args);
        mode = check_command(args, num_read, node);
    }

    // create sockets to listen (only needs port number as ip is localhost)
    UDP_socket = UDP_setup(node->self_port);
    TCP_socket = TCP_setup(node->self_port);
    verbose("TCP and UDP listening sockets created.");

    //  Initialização do conjunto de sockets a 0
    FD_ZERO(&available_sockets);

    // Definição das sockets de ligação e o stdin na lista de sockets a observar pelo select()

    FD_SET(UDP_socket, &available_sockets);
    FD_SET(TCP_socket, &available_sockets);
    FD_SET(STDIN_FILENO, &available_sockets);

    // verifica qual a socket com id maior
    max_socket = UDP_socket > TCP_socket ? UDP_socket + 1 : TCP_socket + 1;

    if (mode == 2) // pentry
    {
        TCP_Prev_socket = self_send(args, node);
        verbose("Self command sent to predecessor.");
        node->fd_pred = TCP_Prev_socket;
        FD_SET(TCP_Prev_socket, &available_sockets);
        max_socket = max_socket > TCP_Prev_socket ? max_socket : TCP_Prev_socket + 1;
    }
    else if (mode = 1) // bentry
    {
    }

    printf("Waiting for connections or command input\n");
    node->is_online = 1;
    write(1, INPUT, sizeof(INPUT));
    while (1)
    {
        // Cópia de lista de sockets devido ao comportamento destrutivo do select()
        ready_sockets = available_sockets;

        error = select(max_socket, &ready_sockets, NULL, NULL, NULL);
        if (error < 0)
        { /*ERROR*/
            printf(RED "File descriptor set ERROR.");
            exit(1);
        }
        for (int i = 0; i < max_socket; i++)
        {
            if (FD_ISSET(i, &ready_sockets))
            {
                if (i == UDP_socket)
                {
                    memset(message, '\0', BUFFER_SIZE);
                    memset(&client_addr, '\0', sizeof(struct sockaddr_in));
                    client_addrlen = sizeof(client_addr);
                    n = recvfrom(UDP_socket, message, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addrlen);
                    verbose("UDP message recieved.");
                    write(1, message, sizeof(message));
                    if (sendto(UDP_socket, "ACK", 3, 0, (struct sockaddr *)&client_addr, client_addrlen) < 0)
                    {
                        printf(RED "ACK send failed\n");
                        exit(1);
                    };

                    sscanf(message, "%s %s %s %s %s", args[0], args[1], args[2], args[3], args[4], args[5]);
                    if (strcmp(args[0], "FND") == 0)
                    {
                        /*Recieved Find request*/
                        int dist = distancia(node, args[1]);
                        find_recieve(args, node, dist);
                    }
                    else if (strcmp(args[0], "RSP") == 0)
                    {
                        /*Recieved response to find request*/
                        response_recieve(args, node, addr_list[args[2]]);
                    }
                    verbose("ACK response sent");
                }
                else if (i == TCP_socket)
                {
                    int n_read, total;
                    char *temp_ptr;

                    memset(message, '\0', BUFFER_SIZE);
                    temp_ptr = message;
                    // accept connection from incoming socket and read it
                    tcp_client_addrlen = sizeof(tcp_client_addr);
                    if ((new_fd = accept(TCP_socket, (struct sockaddr *)&tcp_client_addr, &tcp_client_addrlen)) == -1)
                    { /*ERROR*/
                        printf(RED "TCP connection acceptance error.");
                        exit(0);
                    }
                    n_read = read(new_fd, message, BUFFER_SIZE);
                    // while (n_read = read(new_fd, temp_ptr, BUFFER_SIZE) > 1)
                    // {
                    //     if (n_read < 0)
                    //         exit(1);
                    //     temp_ptr += n_read;
                    //     total += n_read;
                    // }
                    // message[total] = "\0";
                    sscanf(message, "%s %s %s %s %s %s", args[0], args[1], args[2], args[3], args[4], args[5]);
                    // printf("%lu - %lu - %lu - %lu\n", strlen(args[0]), strlen(args[1]), strlen(args[2]), strlen(args[3]));
                    verbose("TCP message received");
                    if (strcmp(args[0], "SELF") == 0)
                    {
                        self_recieve(args, node, new_fd);
                        verbose("SELF command recieved.");
                        if (node->fd_pred == -1)
                        {
                            TCP_Prev_socket = self_send(args, node);
                            verbose("Predecessor was missing, SELF sent");
                            FD_SET(TCP_Prev_socket, &available_sockets);
                            max_socket = max_socket > TCP_Prev_socket ? max_socket : TCP_Prev_socket + 1;
                        }
                    }
                    else if (strcmp(args[0], "FND") == 0)
                    {
                        /*Recieved Find request*/
                        int dist = distancia(node, args[1]);
                        find_recieve(args, node, dist);
                    }
                    else if (strcmp(args[0], "RSP") == 0)
                    {
                        /*Recieved response to find request*/
                        response_recieve(args, node, addr_list[args[2]]);
                    }
                    printf("Received: %s\n", message);
                    write(1, INPUT, sizeof(INPUT));
                }
                else if (i == STDIN_FILENO)
                {

                    num_read = get_command(args);
                    mode = check_command(args, num_read, node);
                    switch (mode)
                    {
                    case 0: // new
                        break;

                    case 1: // bentry
                        break;

                    case 2: // pentry
                        TCP_Prev_socket = self_send(args, node);

                        FD_SET(TCP_Prev_socket, &available_sockets);
                        max_socket = max_socket > TCP_Prev_socket ? max_socket : TCP_Prev_socket + 1;
                        break;

                    case 3: // chord
                        break;

                    case 4: // echord
                        break;

                    case 5: // show
                        show(node);
                        break;

                    case 6: // find
                        int dist = distancia(node, args[1]);

                        switch (dist)
                        {
                        case 0: /*This node is the owner of the searched key*/
                            printf("Chave %d: nó %d (%d:%d).", args[1], node->self_i, node->self_ip, node->self_port);
                            break;
                        case 1: /*Next node is closer to the searched key*/
                            int process_id = seq_number;
                            addr_list[process_id].seq_number = process_id;
                            addr_list[process_id].searched_key = args[1];
                            addr_list[process_id].addr = memset(&client_addr, '\0', sizeof(struct sockaddr_in));
                            seq_number++;
                            find_send_TCP(node, process_id, args[1]);
                            break;
                        case 2: /*Chord node is closer to the searched key*/
                            int process_id = seq_number;
                            addr_list[process_id].seq_number = process_id;
                            addr_list[process_id].searched_key = args[1];
                            addr_list[process_id].addr = memset(&client_addr, '\0', sizeof(struct sockaddr_in));
                            seq_number++;
                            find_send_UDP(node, process_id, args[1]);
                            break;
                        default:
                            printf(RED "FND logic failed\n");
                            break;
                        }
                        break;

                    case 7: // leave
                        pred_send(node);
                        node->pred_i = -1;
                        strcpy(node->pred_ip, "-1");
                        strcpy(node->pred_port, "-1");
                        node->succ_i = -1;
                        strcpy(node->succ_ip, "-1");
                        strcpy(node->succ_port, "-1");
                        close(node->fd_succ);
                        break;

                    case 8: // exit
                        break;

                    default:
                        break;
                    }

                    write(1, INPUT, sizeof(INPUT));
                }
                // Connection with predecessor
                else if (i == node->fd_pred)
                {
                    int n_read, total;
                    char *temp_ptr, **temp;

                    memset(message, '\0', BUFFER_SIZE);
                    temp_ptr = message;
                    n_read = read(i, message, BUFFER_SIZE);
                    // while (n_read = read(new_fd, temp_ptr, BUFFER_SIZE) > 1)
                    // {
                    //     if (n_read < 0)
                    //         exit(1);
                    //     temp_ptr += n_read;
                    //     total += n_read;
                    // }
                    // message[total] = "\0";
                    if (n_read == 0)
                    {
                        /*Conexão de TCP fechada*/
                        node->pred_i = -1;
                        strcpy(node->pred_ip, "-1");
                        strcpy(node->pred_port, "-1");
                        close(node->fd_pred);
                        FD_CLR(node->fd_pred, &available_sockets);
                    }
                    sscanf(message, "%s %s %s %s", args[0], args[1], args[2], args[3]);

                    verbose("TCP message recieved on connection with predecessor");
                    if (strcmp(args[0], "PRED") == 0)
                    {
                        verbose("PRED command recived.");
                        close(node->fd_pred);
                        FD_CLR(node->fd_pred, &available_sockets);
                        TCP_Prev_socket = self_send(args, node);
                        FD_SET(TCP_Prev_socket, &available_sockets);
                        max_socket = max_socket > TCP_Prev_socket ? max_socket : TCP_Prev_socket + 1;
                        node->fd_pred = TCP_Prev_socket;
                        verbose("Self sent to new predecessor and connection saved");
                        printf("Received: %s", message);
                    }
                }
            }
        }

        // free merdas
        // for (int i = 0; i < 3; i++)
        //     free(args[i]);
        // free(args);
    }
}