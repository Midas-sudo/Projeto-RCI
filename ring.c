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
#define GRAY2 "\E[38;2;100;100;100m"
#define INPUT "\E[0;35m\E[1G>>>                 \E[4G\E[0m"
#define START "\E[1G"

int BUFFER_SIZE = 256;
int verb = 0;
int disp_info = 1;

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
    int fd_TCP;
    int fd_UDP;

    int is_online;
    fd_set *socket_list;
} Node_struct;

typedef struct
{
    int searched_key;
    int seq_number;
    struct sockaddr_in addr;
} UDP_addr_list;

Node_struct *global_node;
void pred_send(Node_struct *node);

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

    strcpy(new_node->succ_ip, "-1");
    strcpy(new_node->succ_port, "-1");
    strcpy(new_node->pred_ip, "-1");
    strcpy(new_node->pred_port, "-1");
    strcpy(new_node->chord_ip, "-1");
    strcpy(new_node->chord_port, "-1");

    new_node->fd_succ = -1;
    new_node->fd_pred = -1;
    new_node->fd_TCP = -1;
    new_node->fd_UDP = -1;

    new_node->is_online = 0;
    global_node = new_node;
    return new_node;
}

void free_node(Node_struct *node)
{
    free(node->self_ip);
    free(node->self_port);
    free(node->succ_ip);
    free(node->succ_port);
    free(node->pred_ip);
    free(node->pred_port);
    free(node->chord_ip);
    free(node->chord_port);
    free(node);
    return;
}

void clean_args(char **args, int size)
{
    for (int i = 0; i < 6; i++)
        memset(args[i], '\0', size);
}

void verbose(char *str)
{
    if (verb == 1)
        printf(YLW "%s\n" RESET, str);
}

void log_info(char *str, char *mode)
{
    if (disp_info)
    {
        if (strcmp(mode, "TCPin") == 0)
            printf(GRN START "[->][TCP] %s" RESET, str);
        if (strcmp(mode, "TCPout") == 0)
            printf(GRN START "[<-][TCP] %s" RESET, str);
        if (strcmp(mode, "UDPin") == 0)
            printf(GRN START "[->][UDP] %s \n" RESET, str);
        if (strcmp(mode, "UDPout") == 0)
            printf(GRN START "[<-][UDP] %s \n" RESET, str);
        if (strcmp(mode, "") == 0)
            printf(GRN START "[INFO] %s \n" RESET, str);
    }
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

void disp_header()
{
    printf(IST "┌─Projecto─de─Redes─de─Computadores─e─Internet─────────────┐\n");
    printf("│" RESET "Base de Dados em Anel com Cordas         Ano Letivo 21/22 " IST "│\n");
    printf("│                                                          │\n");
    printf("│" RESET "Autores:                                                  " IST "│\n");
    printf("│" RESET "    •Gonçalo Midões -------------------------- ist196219  " IST "│\n");
    printf("│" RESET "    •Ravi Regalo ----------------------------- ist196305  " IST "│\n");
    printf("└──────────────────────────────────────────────────────────┘\n\n" RESET);
}

void gracefull_leave()
{
    if (global_node->fd_pred != -1 && global_node->pred_i != -1 && global_node->succ_i != -1)
    {
        global_node->succ_i = -1;
        pred_send(global_node);
    }
    FD_CLR(global_node->fd_UDP, global_node->socket_list);
    FD_CLR(global_node->fd_UDP, global_node->socket_list);
    FD_CLR(global_node->fd_pred, global_node->socket_list);

    if (global_node->fd_TCP != -1)
    {
        close(global_node->fd_TCP);
    }
    if (global_node->fd_UDP != -1)
    {
        close(global_node->fd_UDP);
    }
    if (global_node->fd_succ != -1)
    {
        close(global_node->fd_succ);
    }
    if (global_node->fd_pred != -1)
    {
        close(global_node->fd_pred);
    }

    free_node(global_node);
    exit(1);
}

// calculates the distance from node/key i to node/key j
int dist(int i, int j)
{
    return j - i < 0 ? j - i + 32 : j - i;
}
//
int check_dist(Node_struct *node, int key)
{
    int dist_self = dist(node->self_i, key);
    int dist_succ = node->succ_i != -1 ? dist(node->succ_i, key) : 100;
    int dist_chord = node->chord_i != -1 ? dist(node->chord_i, key) : 100; // if there is no chord distance is infinite

    if (dist_self < dist_succ && dist_self < dist_chord)
        return 0;

    else if (dist_succ < dist_chord)
        return 1;

    else if (dist_chord < dist_succ)
        return 2;
    return -1;
}

int TCP_setup(char *port)
{
    int TCPfd, errcode;
    struct addrinfo hints, *res;
    ssize_t n;

    TCPfd = socket(AF_INET, SOCK_STREAM, 0);
    if (TCPfd == -1)
    { /*error*/
        printf(RED "Error creating TCP listen socket.\n");
        gracefull_leave();
    }
    global_node->fd_TCP = TCPfd;

    int enable = 1;
    if (setsockopt(TCPfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    { /*Error*/
        printf(RED "TCP - setsockopt(SO_REUSEADDR) Failed!");
        gracefull_leave();
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0)
    { /*Error*/
        printf(RED START "Error getting own adress info.\n");
        gracefull_leave();
    }
    n = bind(TCPfd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
    { /*Error*/
        printf(RED START "Error binding TCP port.\n");
        gracefull_leave();
    }
    if (listen(TCPfd, 4) == -1)
    { /*Error*/
        printf(RED START "Error listening to TCP port.");
        gracefull_leave();
    }
    freeaddrinfo(res);
    verbose("TCP listening socket ready");
    return TCPfd;
}

int UDP_setup(char *port)
{
    int UDPfd, errcode;
    struct addrinfo hints, *res;
    ssize_t n;

    UDPfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (UDPfd == -1)
    { /*error*/
        printf(RED "Error creating UDP listen socket.\n");
        gracefull_leave();
    }

    global_node->fd_UDP = UDPfd;

    int enable = 1;
    if (setsockopt(UDPfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        printf(RED "UDP - setsockopt(SO_REUSEADDR) Failed!");
        gracefull_leave();
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    errcode = getaddrinfo(NULL, port, &hints, &res);
    if (errcode != 0)
    { /*Error*/
        printf(RED "Error getting own adress info.\n");
        gracefull_leave();
    }
    n = bind(UDPfd, res->ai_addr, res->ai_addrlen);
    if (n == -1)
    { /*Error*/
        printf(RED "Error binding UDP port.\n");
        gracefull_leave();
    }
    freeaddrinfo(res);
    verbose("UDP listening socket ready");
    return UDPfd;
}

void read_tcp(int fd, char *message, int message_size)
{
    int n_read, n_left, n_bytes;
    char *ptr, *buffer = message;

    n_read = read(fd, message, BUFFER_SIZE);
    n_bytes = 10;
    n_left = n_bytes;
    ptr = buffer;
    while (n_left > 0)
    {
        n_read = read(fd, ptr, n_left);
        if (n_read == 1)
            exit(1);
        else if (n_read == 0)
            break; // closed by peer
        n_left -= n_read;
        ptr += n_read;
    }
    n_read = n_bytes - n_left;
    buffer[n_read] = '\0';
}

int send_tcp(int fd, char *message)
{
    int n_left, n_written;
    char *ptr = message;

    n_left = strlen(message);
    while (n_left > 0)
    {
        printf("TEste\n");
        n_written = write(fd, ptr, n_left);
        if (n_written <= 0)
            return -1;
        n_left -= n_written;
        ptr += n_written;
    }
    return 0;
}

int send_udp(char *ip, char *port, char *message, int max_tries)
{
    struct addrinfo hints, *res;
    struct sockaddr recv_addr;
    socklen_t recv_addrlen;
    int fd, errcode, n_tries = 0;
    ssize_t n;
    char *response;
    struct timeval tv;

    response = malloc(BUFFER_SIZE * sizeof(char));
    // create udp socket for sending and receiving
    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1)
        exit(1);
    // set a timeout for the socket
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    // get peer's (chord node) adress info to send data
    errcode = getaddrinfo(ip, port, &hints, &res);
    if (errcode != 0)
        exit(1);

    memset(&recv_addr, '\0', sizeof(struct sockaddr_in));
    recv_addrlen = sizeof(recv_addr);
    do
    {
        n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
        n_tries++;
        if (n < strlen(message))
            printf(RED START "├─ERROR sending full UDP message\n" RESET);
        do
        { // wait for acknowledgement from peer, ignores messages from other addresses
            memset(response, '\0', BUFFER_SIZE);
            if ((n = recvfrom(fd, response, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addrlen)) < 0)
                break;                                                   // connection timed out
        } while (strcmp(res->ai_addr->sa_data, recv_addr.sa_data) != 0); // aposto que esta comparação não vai funcionar
        // received message from peer
        if (n < 0) // error because socked timed out
            continue;
    } while (strcmp(response, "ACK") != 0 && n_tries <= max_tries);

    if (n_tries > max_tries)
        return -1;

    close(fd);
    free(response);
    freeaddrinfo(res);
    return 0;
}

int self_send(char **args, Node_struct *node)
{
    int Pred_fd, errcode;
    struct addrinfo hints, *res;
    char message[BUFFER_SIZE];

    Pred_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (Pred_fd == -1)
    { /*ERROR*/
        printf(RED START "├─Error creating TCP connection socket.\n");
        gracefull_leave();
    }
    node->fd_pred = Pred_fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    errcode = getaddrinfo(args[2], args[3], &hints, &res);
    if (errcode != 0)
    { /*ERROR*/
        printf(RED START "├─Error getting new connection address information.\n");
        gracefull_leave();
    }
    verbose("Got predecessor connection address info");

    errcode = connect(Pred_fd, res->ai_addr, res->ai_addrlen);
    if (errcode == -1)
    { /*ERROR*/
        printf(RED START "├─Error establishing connection.\n");
        gracefull_leave();
    }
    verbose("Succefully established connection with predecessor");

    sprintf(message, "SELF %d %s %s\n", node->self_i, node->self_ip, node->self_port);
    errcode = send_tcp(Pred_fd, message);
    if (errcode)
    {
        printf(RED START "├─Error sending TCP message: %s", message);
        printf(RED START "├─Restart the program and try again, if the problem persists check the input parameters");
        gracefull_leave();
    }
    log_info(message, "TCPout");
    verbose("TCP message \"SELF\" sent to predecessor");

    node->fd_pred = Pred_fd;
    node->pred_i = atoi(args[1]);
    strcpy(node->pred_ip, args[2]);
    strcpy(node->pred_port, args[3]);
    return Pred_fd;
}

void self_receive(char **args, Node_struct *node, int fd)
{
    char message[BUFFER_SIZE];
    int n_left, n_written;
    printf("%d %d %d\n", atoi(args[1]), node->succ_i, node->self_i);
    if (node->succ_i != -1 && ((node->self_i < atoi(args[1]) && atoi(args[1]) < node->succ_i) || (atoi(args[1]) < node->succ_i && node->succ_i < node->self_i) || (node->succ_i < node->self_i && node->self_i < atoi(args[1]))))
    {
        verbose("Predecessor exits.");
        sprintf(message, "PRED %s %s %s\n", args[1], args[2], args[3]);
        n_left = strlen(message);
        while (n_left > 0)
        {
            n_written = write(node->fd_succ, message, strlen(message));
            if (n_written <= 0)
            { /*ERROR*/
                printf(RED START "├─Error sending TCP message to Sucessor!");
                gracefull_leave();
            }
            n_left -= n_written;
            *message += n_written;
            printf(START "├─PRED SENT: %s", message);
        }
        verbose("TCP message to sucessor written");
        log_info(message, "TCPout");
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
    int errcode;
    char message[BUFFER_SIZE];

    sprintf(message, "PRED %d %s %s\n", node->pred_i, node->pred_ip, node->pred_port);
    errcode = send_tcp(node->fd_succ, message);
    if (errcode)
    { /*ERROR*/
        printf(RED START "├─Error sending TCP message to Sucessor!");
        gracefull_leave();
    }
    log_info(message, "TCPout");
    verbose("TCP message \"PRED\" sent to successor");
    // printf(START "├─PRED message sent: %s", message);
    return;
}

void pred_receive(char **args, Node_struct *node)
{
}

char **efnd_send(char **args, Node_struct *node)
{
    struct addrinfo hints, *res;
    struct sockaddr recv_addr;
    socklen_t recv_addrlen;
    int fd, errcode, n_tries = 0, max_tries = 5;
    ssize_t n;
    char message[BUFFER_SIZE], response[BUFFER_SIZE];
    struct timeval tv;
    memset(message, '\0', BUFFER_SIZE);

    sprintf(message, "EFND %d", node->self_i);

    // create udp socket for sending and receiving
    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1)
        gracefull_leave();
    // set a timeout for the socket
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    // get peer's (chord node) adress info to send data
    errcode = getaddrinfo(args[2], args[3], &hints, &res);
    if (errcode != 0)
    {
        close(fd);
        gracefull_leave();
    }

    memset(&recv_addr, '\0', sizeof(struct sockaddr_in));
    recv_addrlen = sizeof(recv_addr);
    do
    {
        n = sendto(fd, message, sizeof(message), 0, res->ai_addr, res->ai_addrlen);
        n_tries++;
        if (n < sizeof(message))
            printf(RED START "├─ERROR sending UDP message\n" RESET);
        do
        { // wait for acknowledgement from peer, ignores messages from other addresses
            memset(response, '\0', BUFFER_SIZE);
            if ((n = recvfrom(fd, response, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addrlen)) < 0)
                break;                                                   // connection timed out
        } while (strcmp(res->ai_addr->sa_data, recv_addr.sa_data) != 0); // aposto que esta comparação não vai funcionar
        // received message from peer
        // if using errno, error would be SOCTIMEDOUT but for now just use -1
        if (n < 0)
            continue;
    } while (strcmp(response, "ACK") != 0 && n_tries <= max_tries);

    if (n_tries > max_tries)
    {
        printf(RED START "├─ERROR tried to send message 5 times but didnt receive ACK.\nPlease restart the program and try again, if the problem persists check you have inputed the right node information.\n" RESET);
        close(fd);
        gracefull_leave();
    }
    else
    {
        verbose("Successfully sent UDP message and ACK received");
        printf(START "├─EFND message sent helping node: %s\n", message);
        n_tries = 0;
    }
    do
    {
        memset(response, '\0', BUFFER_SIZE);
        n = recvfrom(fd, response, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addrlen);
        if (n < 0)
            continue;
        sscanf(response, "%s %s %s %s %s %s", args[0], args[1], args[2], args[3], args[4], args[5]);
    } while (strcmp(args[0], "EPRED") != 0 && n_tries <= max_tries);
    if (n_tries > max_tries)
    {
        printf(RED START "├─Recieved the wrong message 5 time. Was expecting EPRED\nPlease restart the program and try again, if the problem persists check you have inputed the right node information.\n" RESET);
        close(fd);
        gracefull_leave();
    }
    else
    {
        printf(START "├─EPRED message recieved: %s\n", response);
    }
    if (sendto(fd, "ACK", 3, 0, (struct sockaddr *)&recv_addr, recv_addrlen) < 3) //(was 0) n devia ser 3? se mandar 2 bytes fica podre
    {
        printf(RED START "ACK send failed\n");
        close(fd);
        gracefull_leave();
    };
    close(fd);
    verbose("ACK response sent");
    freeaddrinfo(res);
    return args;
}

void epred_send(char **args, Node_struct *node, struct sockaddr_in addr, int seq)
{
    struct sockaddr recv_addr;
    socklen_t addrlen;
    int fd, n_tries = 0, max_tries = 5;
    ssize_t n;
    char message[BUFFER_SIZE], response[BUFFER_SIZE];
    struct timeval tv;

    memset(message, '\0', BUFFER_SIZE);
    if (seq) // if its the node that started the search
        sprintf(message, "EPRED %s %s %s", args[3], args[4], args[5]);
    else

        sprintf(message, "EPRED %d %s %s", node->self_i, node->self_ip, node->self_port);

    // create udp socket for sending and receiving
    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd != -1)
    {
        // set a timeout for the socket
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        addrlen = sizeof(recv_addr);
        do
        {
            n = sendto(fd, message, sizeof(message), 0, (struct sockaddr *)&addr, addrlen);
            n_tries++;
            if ((n < sizeof(message)) == 1)
                printf(RED START "├─ERROR sending UDP message\n" RESET);
            do
            { // wait for acknowledgement from peer, ignores messages from other addresses
                memset(response, '\0', BUFFER_SIZE);
                if ((n = recvfrom(fd, response, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &addrlen)) < 0)
                    break;                                                                 // connection timed out
            } while (strcmp(((struct sockaddr *)&addr)->sa_data, recv_addr.sa_data) != 0); // aposto que esta comparação não vai funcionar
            // received message from peer
            // if using errno, error would be SOCTIMEDOUT but for now just use -1
            if (n < 0)
                continue;
        } while (strcmp(response, "ACK") != 0 && n_tries <= max_tries);

        if (n_tries > max_tries)
            printf(RED START "├─ERROR tried to send message 5 times but didnt receive ACK\n Outside node might have recived information\n" RESET);
        else
        {
            verbose("Successfully sent UDP message and ACK received");
            printf(START "├─EPRED message sent to outside node: %s\n", message);
        }
    }
    else
    {
        printf(RED START "├─There was a problem setting up the socket for sending the EPRED response.\n Outside node not going to enter the ring\n" RESET);
    }
}

void find_send_TCP(char **args, Node_struct *node, int seq)
{
    int errcode;
    char message[BUFFER_SIZE];

    if (seq) // if its the node that started the search
        sprintf(message, "FND %s %d %d %s %s\n", args[1], seq, node->self_i, node->self_ip, node->self_port);
    else
        sprintf(message, "FND %s %s %s %s %s\n", args[1], args[2], args[3], args[4], args[5]);

    errcode = send_tcp(node->fd_succ, message);
    if (errcode)
    {
        printf("├─ERROR sending TCP FND message to sucessor\n");
    }
    log_info(message, "TCPout");
    verbose("TCP message \"FND\" sent to successor");
    // printf(START "├─FND message sent to successor: %s", message);
    return;
}

void find_send_UDP(char **args, Node_struct *node, int seq)
{
    int errcode;
    char *message;

    message = malloc(BUFFER_SIZE * sizeof(char));
    memset(message, '\0', BUFFER_SIZE);
    if (seq) // if its the node that started the search
        sprintf(message, "FND %s %d %d %s %s", args[1], seq, node->self_i, node->self_ip, node->self_port);
    else
        sprintf(message, "FND %s %s %s %s %s", args[1], args[2], args[3], args[4], args[5]);

    errcode = send_udp(node->chord_ip, node->chord_port, message, 3);
    if (errcode)
    {
        printf(RED START "├─ERROR tried to send \"FND\" message 3 times but didnt receive ACK. Trying to send it by TCP to sucessor\n" RESET);
        errcode = send_tcp(node->fd_succ, message);
        if (errcode)
        { /*ERROR*/
            printf(RED START "├─Error sending TCP message to Sucessor!\n");

            log_info(message, "UDPout");
        }
    }
    else
    {
        verbose("Successfully sent UDP message \"FND\" and \"ACK\" received");
        printf(START "├─FND message sent to chord: %s\n", message);
    }
    free(message);
}

void response_send_TCP(char **args, Node_struct *node, int isfirst)
{
    int errcode;
    char message[BUFFER_SIZE];

    if (isfirst) // if its the node that has the key and is now sending response
        sprintf(message, "RSP %s %s %d %s %s\n", args[3], args[2], node->self_i, node->self_ip, node->self_port);
    else
        sprintf(message, "RSP %s %s %s %s %s\n", args[1], args[2], args[3], args[4], args[5]);

    errcode = send_tcp(node->fd_succ, message);
    if (errcode)
    {
        printf(RED START "├─Error sending TCP RSP message to sucessor!\n");
        log_info(message, "TCPout");
    }
    log_info(message, "TCPout");
    verbose("TCP message \"RSP\" sent to successor");
    // printf(START "├─RSP message sent to successor: %s", temp);
    return;
}

void response_send_UDP(char **args, Node_struct *node, int isfirst)
{
    int errcode;
    char *message;

    message = malloc(BUFFER_SIZE * sizeof(char));
    memset(message, '\0', BUFFER_SIZE);
    if (isfirst) // if its the node that has the key and is now sending response
        sprintf(message, "RSP %s %s %d %s %s\n", args[3], args[2], node->self_i, node->self_ip, node->self_port);
    else
        printf(message, "RSP %s %s %s %s %s\n", args[1], args[2], args[3], args[4], args[5]);

    errcode = send_udp(node->chord_ip, node->chord_port, message, 3);
    if (errcode)
    {
        /* error handling */
        printf(RED START "├─ERROR tried to send \"RSP\" message 3 times but didnt receive ACK. Trying to send by TCP to sucessor\n" RESET);
        errcode = send_tcp(node->fd_succ, message);
        if (errcode)
        { /*ERROR*/

            printf(RED START "├─Error sending TCP message to Sucessor!\n");
        }
    }
    else
    {
        verbose("Successfully sent UDP message \"RSP\" and \"ACK\" received");
        printf(START "├─RSP message sent to chord: %s\n", message);
    }
    free(message);
}

void response_receive(char **args, Node_struct *node, UDP_addr_list *addr_vect)
{
    int dist;
    struct sockaddr_in empty;
    memset(&empty, '\0', sizeof(struct sockaddr_in));

    if (atoi(args[1]) == node->self_i) /*This was the initial node*/
    {
        // Checks if the response was already haddled//
        if (addr_vect[atoi(args[2])].searched_key == -1)
        {
            return;
        }
        if (addr_vect[atoi(args[2])].addr.sin_port != empty.sin_port)
        {
            epred_send(args, node, addr_vect[atoi(args[2])].addr, 1);
        }
        else
        {
            printf(START IST "├─A chave %d está no nó %s (%s:%s).\n" RESET, addr_vect[atoi(args[2])].searched_key, args[3], args[4], args[5]);
            addr_vect[atoi(args[2])].searched_key = -1;
        }
    }
    else
    {
        dist = check_dist(node, atoi(args[1])); // distance to the node to be reached
        switch (dist)
        {
        case 1: /*Next node is closer to the initial node*/
            response_send_TCP(args, node, 0);
            break;
        case 2: /*Chord node is closer to the initial node*/
            response_send_UDP(args, node, 0);
            break;
        default:
            printf(RED START "├─RSP logic failed\n" RESET);
            break;
        }
    }
    // printf(START "└────\n");
}

void find_receive(char **args, Node_struct *node)
{
    int dist;

    dist = check_dist(node, atoi(args[1])); // check distances to (key is args[1])
    switch (dist)
    {
    case 0:                                     /*This node is the owner of the searched key*/
        dist = check_dist(node, atoi(args[3])); // check distance to requesting node
        switch (dist)
        {
        case 1: /*Next node is closer to the initial node*/
            response_send_TCP(args, node, 1);
            break;
        case 2: /*Chord node is closer to the initial node*/
            response_send_UDP(args, node, 1);
            break;
        default:
            printf(RED START "├─RSP logic failed\n" RESET);
            break;
        }
        break;
    case 1: /*Next node is closer to the searched key*/
        find_send_TCP(args, node, 0);
        break;
    case 2: /*Chord node is closer to the searched key*/
        find_send_UDP(args, node, 0);
        break;
    default:
        printf(RED START "├─FND logic failed\n" RESET);
        break;
    }
    printf(START "└────\n");
}

void show(Node_struct *node)
{
    printf(IST "\n┌─Node─Information─────────────┐\n");
    printf("│" RESET "\E[1m  Node:\E[32G\E[0m" IST "│\n");
    printf("│" RESET "   ~Key:  " GRN "%d\E[32G" IST "│\n", node->self_i);
    printf("│" RESET "   ~IP:   " GRN "%s\E[32G" IST "│\n", node->self_ip);
    printf("│" RESET "   ~Port: " GRN "%s\E[32G" IST "│\n", node->self_port);
    printf("│\E[32G│\n");
    printf("│" RESET "\E[1m  Predecessor:\E[32G\E[0m" IST "│\n");
    printf("│" RESET "   ~Key:  " GRN "%d\E[32G" IST "│\n", node->pred_i);
    printf("│" RESET "   ~IP:   " GRN "%s\E[32G" IST "│\n", node->pred_ip);
    printf("│" RESET "   ~Port: " GRN "%s\E[32G" IST "│\n", node->pred_port);
    printf("│\E[32G│\n");
    printf("│" RESET "\E[1m  Sucessor:\E[32G\E[0m" IST "│\n");
    printf("│" RESET "   ~Key:  " GRN "%d\E[32G" IST "│\n", node->succ_i);
    printf("│" RESET "   ~IP:   " GRN "%s\E[32G" IST "│\n", node->succ_ip);
    printf("│" RESET "   ~Port: " GRN "%s\E[32G" IST "│\n", node->succ_port);
    printf("│\E[32G│\n");
    if (node->chord_i != -1)
    {
        printf("│" RESET "\E[1m  Chord:\E[32G\E[0m" IST "│\n");
        printf("│" RESET "   ~Key:  " GRN "%d\E[32G" IST "│\n", node->chord_i);
        printf("│" RESET "   ~IP:   " GRN "%s\E[32G" IST "│\n", node->chord_ip);
        printf("│" RESET "   ~Port: " GRN "%s\E[32G" IST "│\n", node->chord_port);
        printf(IST "└──────────────────────────────┘\n" RESET);
    }
    else
    {
        printf("│" RESET "\E[1m  This node has no chord path.\E[32G" IST "│\n");
        printf(IST "└──────────────────────────────┘\n" RESET);
    }
}

// prompts user for a command (and its args) and returns the number of args read
int get_command(char **args)
{
    char buffer[BUFFER_SIZE];
    clean_args(args, BUFFER_SIZE);
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
            printf(RED START "Node is already part of a ring. First use \"leave\" to leave the ring\n" RESET);
            return -1;
        }
        return 0;
    }
    if (strcmp(args[0], "bentry") == 0 || strcmp(args[0], "b") == 0)
    {
        if (node->is_online)
        {
            printf(RED START "Node is already part of a ring. First use \"leave\" to leave the ring\n" RESET);
            return -1;
        }
        if (num_read != 4)
        {
            printf(YLW START "Usage: bentry key key.ip key.port\n" RESET);
            return -1;
        }
        return 1;
    }
    if (strcmp(args[0], "pbentry") == 0 || strcmp(args[0], "pb") == 0)
    {
        if (node->is_online)
        {
            printf(RED START "Node is already part of a ring. First use \"leave\" to leave the ring\n" RESET);
            return -1;
        }
        if (num_read != 2)
        {
            printf(YLW START "Usage: pbentry key\n" RESET);
            return -1;
        }
        return 10;
    }
    if (strcmp(args[0], "pentry") == 0 || strcmp(args[0], "p") == 0)
    {
        if (node->is_online)
        {
            printf(RED START "Node is already part of a ring. First use \"leave\" to leave the ring\n" RESET);
            return -1;
        }
        if (num_read != 4)
        {
            printf(YLW START "Usage: pentry key key.ip key.port\n" RESET);
            return -1;
        }
        return 2;
    }
    if (strcmp(args[0], "chord") == 0 || strcmp(args[0], "c") == 0)
    {
        if (!node->is_online)
        {
            printf(RED START "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        if (num_read != 4)
        {
            printf(YLW START "Usage: chord key key.ip key.port\n" RESET);
            return -1;
        }
        return 3;
    }
    if ((strcmp(args[0], "dchord") == 0 || strcmp(args[0], "d") == 0) && node->chord_i != -1)
    {
        if (!node->is_online)
        {
            printf(RED START "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        return 4;
    }
    if (strcmp(args[0], "show") == 0 || strcmp(args[0], "s") == 0)
    {
        if (!node->is_online)
        {
            printf(RED START "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        return 5;
    }
    if (strcmp(args[0], "find") == 0 || strcmp(args[0], "f") == 0)
    {
        if (!node->is_online)
        {
            printf(RED START "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        if (strcmp(args[1], "") == 0)
        {
            printf(RED START "Usage: find k, where k is the key to be searched\n" RESET);
            return -1;
        }
        if (strlen(args[1]) > 2 || atoi(args[1]) > 32 || args[1][0] < 48 || args[1][0] > 59)
        {
            printf(RED START "Usage: find k, k must be a number smaller than 32\n" RESET);
            return -1;
        }
        return 6;
    }
    if (strcmp(args[0], "leave") == 0 || strcmp(args[0], "l") == 0)
    {
        if (!node->is_online)
        {
            printf(RED START "Node is not part of a ring. Use \"new\", \"pentry\" or \"bentry\"\n" RESET);
            return -1;
        }
        return 7;
    }
    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "e") == 0)
    {
        if (node->is_online)
        {
            printf(RED START "Node is still part of a ring. Use \"leave\"\n" RESET);
            return -1;
        }
        return 8;
    }
    printf(RED START "\"%s\" is not a valid command.\n" RESET, args[0]);
    return -1;
}

void find(char **args, Node_struct *node, UDP_addr_list *addr_list, int seq_number, struct sockaddr_in addr)
{
    struct sockaddr_in empty;
    memset(&empty, '\0', sizeof(struct sockaddr_in));
    int find_id;
    int dist = check_dist(node, atoi(args[1]));
    printf(START "│Search for the node with key %s has began!\n", args[1]);

    switch (dist)
    {
    case 0: /*This node is the owner of the searched key*/
        if (addr.sin_port != empty.sin_port)
        {
            epred_send(args, node, addr, 0);
        }
        else
        {
            printf(START IST "├─A chave %s está no nó %d (%s:%s).\n" RESET, args[1], node->self_i, node->self_ip, node->self_port);
            printf(START "└────\n");
            write(1, INPUT, sizeof(INPUT));
        }

        break;
    case 1: /*Next node is closer to the searched key*/
        find_id = seq_number;
        addr_list[find_id].seq_number = find_id;
        addr_list[find_id].searched_key = atoi(args[1]);
        memcpy(&addr_list[find_id].addr, &addr, sizeof(addr));
        seq_number++;
        find_send_TCP(args, node, find_id);
        break;
    case 2: /*Chord node is closer to the searched key*/
        find_id = seq_number;
        addr_list[find_id].seq_number = find_id;
        addr_list[find_id].searched_key = atoi(args[1]);
        memcpy(&addr_list[find_id].addr, &addr, sizeof(addr));
        seq_number++;
        find_send_UDP(args, node, find_id);
        break;
    default:
        printf(RED START "├─FND logic failed\nTry again.\n" RESET);
        printf(START "└────\n");
        break;
    }
}

void INThandler(int sig)
{
    signal(sig, SIG_IGN);
    if (global_node->fd_pred != -1)
        pred_send(global_node);
    printf(RED "\nHey budy, did you mean to hit Ctrl-C? "
               "You were lucky I caught it.\n"
               "I tried to fix any unwanted behaviour in the ring but something might have break check the connections of this node old predecessor and sucessor!\n" RESET);
    exit(0);
}

void check_call(int argc, char **argv)
{
    if (argc != 4)
    {
        if (argc == 5)
            if (strcmp(argv[4], "-v") == 0)
                verb = 1;
            else if (strcmp(argv[4], "-s") == 0)
                disp_info = 0;
            else
            {
                printf(RED "Usage: ring i i.ip i.port (-v for verbose, -s for silent)\n");
                exit(0);
            }
        else
        {
            printf(RED "Usage: ring i i.ip i.port (-v for verbose)\n");
            exit(0);
        }
    }
    return;
}

void setup(fd_set *available_sockets, Node_struct *node, int *max_socket, int *TCP_fd, int *UDP_fd)
{
    *TCP_fd = TCP_setup(node->self_port);
    *UDP_fd = UDP_setup(node->self_port);
    FD_SET(*TCP_fd, available_sockets);
    FD_SET(*UDP_fd, available_sockets);
    node->is_online = 1;
    *max_socket = *TCP_fd > *UDP_fd ? *TCP_fd + 1 : *UDP_fd + 1;
    verbose("TCP and UDP listening sockets created.");
}

void leave(Node_struct *node, fd_set *available_sockets)
{
    if (node->pred_i != node->self_i && node->pred_i != -1)
    {
        pred_send(node);
        printf(START "└────\n");
    }
    node->chord_i = -1;
    strcpy(node->chord_ip, "-1");
    strcpy(node->chord_port, "-1");

    node->pred_i = -1;
    strcpy(node->pred_ip, "-1");
    strcpy(node->pred_port, "-1");
    FD_CLR(node->fd_pred, available_sockets);
    close(node->fd_pred);
    node->fd_pred = -1;

    node->succ_i = -1;
    strcpy(node->succ_ip, "-1");
    strcpy(node->succ_port, "-1");
    FD_CLR(node->fd_succ, available_sockets);
    close(node->fd_succ);
    node->fd_succ = -1;
    node->is_online = 0;

    FD_CLR(node->fd_TCP, available_sockets);
    FD_CLR(node->fd_UDP, available_sockets);
    close(node->fd_TCP);
    close(node->fd_UDP);
}

void sos_send(Node_struct *node, char **args, int seq)
{
    int errcode;
    char message[BUFFER_SIZE];
    if (seq)
    {

        sprintf(message, "SOS %d %d %s %s\n", node->pred_i, node->self_i, node->self_ip, node->self_port);
    }
    else
        sprintf(message, "SOS %s %s %s %s", args[1], args[2], args[3], args[4]);
    errcode = send_tcp(node->fd_succ, message);
    if (errcode)
    {
        printf("├─ERROR sending TCP FND message to sucessor\n");
    }
    log_info(message, "TCPout");
    verbose("TCP message \"FND\" sent to successor");
    // printf(START "├─FND message sent to successor: %s", message);
    return;
}

void sos_recieve(char **args, Node_struct *node)
{
    if (atoi(args[1]) == node->succ_i)
    {
        int fd, errcode;
        struct addrinfo hints, *res;
        char message[BUFFER_SIZE];

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        { /*ERROR*/
            printf(RED START "├─Error creating TCP connection socket.\n");
        }

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        errcode = getaddrinfo(args[3], args[4], &hints, &res);
        if (errcode != 0)
        { /*ERROR*/
            printf(RED START "├─Error getting new connection address information.\n");
        }
        verbose("Got predecessor connection address info");

        errcode = connect(fd, res->ai_addr, res->ai_addrlen);
        if (errcode == -1)
        { /*ERROR*/
            printf(RED START "├─Error establishing connection.\n");
        }
        verbose("Succefully established connection with predecessor");

        sprintf(message, "SOS %d %s %s\n", node->self_i, node->self_ip, node->self_port);
        errcode = send_tcp(fd, message);
        if (errcode)
        {
            printf(RED START "├─Error sending TCP message: %s", message);
            printf(RED START "├─Restart the program and try again, if the problem persists check the input parameters");
            gracefull_leave();
        }
        log_info(message, "TCPout");
        verbose("TCP message \"SELF\" sent to predecessor");

        node->fd_succ = fd;
        node->succ_i = atoi(args[2]);
        strcpy(node->succ_ip, args[3]);
        strcpy(node->succ_port, args[4]);
    }
    else
    {
        sos_send(node, args, 0);
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, INThandler);
    int UDP_fd, TCP_fd, TCP_pred_fd, max_socket, error, new_fd, sos_counter, sos_flag = 0;
    char message[BUFFER_SIZE];
    Node_struct *node;
    fd_set available_sockets, ready_sockets; // Variáveis para guardar o conjunto de sockets
    struct sockaddr_in client_addr, tcp_client_addr, addr;
    socklen_t client_addrlen, tcp_client_addrlen;
    char **args;
    int num_read, mode;
    UDP_addr_list addr_list[100];
    int seq_number = 1;

    check_call(argc, argv);
    disp_header();
    node = new_node(atoi(argv[1]), argv[2], argv[3]);
    // prompt user for entering  a command and shows command list
    command_list(atoi(argv[1]), argv[2], argv[3]);

    args = malloc(6 * sizeof(char *));
    for (int i = 0; i < 6; i++)
    {
        args[i] = malloc(BUFFER_SIZE * sizeof(char));
    }

    FD_ZERO(&available_sockets);              // initialize fd set
    FD_SET(STDIN_FILENO, &available_sockets); // add STDIN to fd set
    max_socket = STDIN_FILENO + 1;
    node->socket_list = &available_sockets;

    while (1)
    {
        write(1, INPUT, sizeof(INPUT));
        ready_sockets = available_sockets; // Cópia de lista de sockets devido ao comportamento destrutivo do select()
        seq_number = seq_number == 99 ? 1 : seq_number;

        error = select(max_socket, &ready_sockets, NULL, NULL, NULL);
        if (error < 0)
        { /*ERROR*/
            printf(RED START "File descriptor set ERROR.\n");
            exit(1);
        }
        for (int i = 0; i < max_socket; i++)
        {
            if (FD_ISSET(i, &ready_sockets))
            {
                if (i == STDIN_FILENO) // General Listen STDIN
                {
                    num_read = get_command(args);
                    mode = check_command(args, num_read, node);
                    switch (mode)
                    {
                    case 0: // new
                        setup(&available_sockets, node, &max_socket, &TCP_fd, &UDP_fd);
                        printf(START "|Waiting for connections or command input\n");
                        printf(START "└────\n");
                        break;

                    case 1: // bentry
                        setup(&available_sockets, node, &max_socket, &TCP_fd, &UDP_fd);
                        args = efnd_send(args, node);
                        TCP_pred_fd = self_send(args, node);
                        node->fd_pred = TCP_pred_fd;
                        FD_SET(TCP_pred_fd, &available_sockets);
                        max_socket = max_socket > TCP_pred_fd ? max_socket : TCP_pred_fd + 1;
                        break;

                    case 2: // pentry
                        setup(&available_sockets, node, &max_socket, &TCP_fd, &UDP_fd);
                        TCP_pred_fd = self_send(args, node);
                        node->fd_pred = TCP_pred_fd;
                        FD_SET(TCP_pred_fd, &available_sockets);
                        max_socket = max_socket > TCP_pred_fd ? max_socket : TCP_pred_fd + 1;
                        break;

                    case 3: // chord
                        node->chord_i = atoi(args[1]);
                        strcpy(node->chord_ip, args[2]);
                        strcpy(node->chord_port, args[3]);
                        printf(START IST "|Chord sucessfully added!\n" RESET);
                        printf(START "└────\n");
                        break;

                    case 4: // dchord
                        node->chord_i = -1;
                        strcpy(node->chord_ip, "-1");
                        strcpy(node->chord_port, "-1");
                        printf(START IST "|Chord sucessfully deleted!\n" RESET);
                        printf(START "└────\n");
                        break;

                    case 5: // show
                        show(node);
                        break;

                    case 6: // find
                        memset(&addr, '\0', sizeof(struct sockaddr_in));
                        seq_number++;
                        find(args, node, addr_list, seq_number, addr);
                        break;

                    case 7: // leave
                        leave(node, &available_sockets);
                        break;

                    case 8: // exit
                        if (node->is_online == 1)
                        {
                            printf(ORG START "* The node is still part of a ring, please use the command " IST " leave" ORG "first!\n");
                        }
                        else
                        {
                            exit(0);
                        }
                        break;
                    case 10: // pbentry
                        strcpy(args[0], "bentry");
                        strcpy(args[2], "193.136.138.142");
                        strcpy(args[3], "58107");
                        printf(START YLW "This command only works inside the sigma network!");
                        setup(&available_sockets, node, &max_socket, &TCP_fd, &UDP_fd);
                        args = efnd_send(args, node);
                        TCP_pred_fd = self_send(args, node);
                        node->fd_pred = TCP_pred_fd;
                        FD_SET(TCP_pred_fd, &available_sockets);
                        max_socket = max_socket > TCP_pred_fd ? max_socket : TCP_pred_fd + 1;
                        break;
                    default:
                        break;
                    }

                    write(1, INPUT, sizeof(INPUT));
                }
                else if (i == UDP_fd) // General Listen UDP
                {
                    memset(message, '\0', BUFFER_SIZE);
                    memset(&client_addr, '\0', sizeof(struct sockaddr_in));
                    client_addrlen = sizeof(client_addr); // dont think this line is needed
                    clean_args(args, BUFFER_SIZE);

                    recvfrom(UDP_fd, message, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addrlen);
                    // verbose("UDP message received: \n");
                    log_info(message, "UDPin");

                    if (node->is_online)
                    {
                        if (sendto(UDP_fd, "ACK", 3, 0, (struct sockaddr *)&client_addr, client_addrlen) < 3) //(was 0) n devia ser 3? se mandar 2 bytes fica podre
                        {
                            printf(RED START "ACK send failed\n");
                            exit(1);
                        };
                        // verbose("ACK response sent");
                        log_info("ACK", "UDPout");

                        sscanf(message, "%s %s %s %s %s %s", args[0], args[1], args[2], args[3], args[4], args[5]);
                        if (strcmp(args[0], "FND") == 0)
                        {
                            verbose("Received FND via UDP");
                            printf(START "│Received new search: %s\n", message);
                            find_receive(args, node);
                        }
                        else if (strcmp(args[0], "RSP") == 0)
                        {
                            /*Received response to find request*/
                            verbose("Received RSP via UDP");
                            printf(START "│Received new response: %s\n", message);
                            response_receive(args, node, addr_list);
                        }
                        else if (strcmp(args[0], "EFND") == 0)
                        {
                            seq_number++;
                            find(args, node, addr_list, seq_number, client_addr);
                        }

                        printf(START "└────\n");
                        write(1, INPUT, sizeof(INPUT));
                    }
                }
                else if (i == TCP_fd) // General Listen TCP
                {
                    memset(message, '\0', BUFFER_SIZE);
                    clean_args(args, BUFFER_SIZE);

                    // accept connection from incoming socket and read it
                    tcp_client_addrlen = sizeof(tcp_client_addr);
                    if ((new_fd = accept(TCP_fd, (struct sockaddr *)&tcp_client_addr, &tcp_client_addrlen)) == -1)
                    { /*ERROR*/
                        printf(RED START "TCP connection acceptance error.");
                        exit(0);
                    }
                    read(new_fd, message, BUFFER_SIZE);

                    if (node->is_online)
                    {
                        log_info(message, "TCPin");
                        sscanf(message, "%s %s %s %s %s %s", args[0], args[1], args[2], args[3], args[4], args[5]);
                        // verbose("TCP message received");
                        //  printf(START "├─Received: %s", message);

                        if (strcmp(args[0], "SELF") == 0)
                        {
                            self_receive(args, node, new_fd);
                            verbose("SELF command received.");
                            if (node->fd_pred == -1 || (node->pred_i == node->self_i && atoi(args[1]) != node->self_i))
                            {
                                TCP_pred_fd = self_send(args, node);
                                verbose("Predecessor was missing, SELF sent");
                                FD_SET(TCP_pred_fd, &available_sockets);
                                max_socket = max_socket > TCP_pred_fd ? max_socket : TCP_pred_fd + 1;
                            }
                            printf(START YLW "├─New connection established!" RESET "\n| ---%d─>%d─>%d---\n", node->pred_i, node->self_i, node->succ_i);
                        }
                        else if (strcmp(args[0], "SOS") == 0)
                        {
                            if (sos_flag == 1)
                            {
                                FD_CLR(node->fd_pred, &available_sockets);
                                FD_SET(new_fd, &available_sockets);
                                max_socket = max_socket > TCP_pred_fd ? max_socket : TCP_pred_fd + 1;
                                sos_flag = 0;
                                node->fd_pred = new_fd;
                                node->pred_i = atoi(args[1]);
                                strcpy(node->pred_ip, args[2]);
                                strcpy(node->pred_port, args[3]);
                                printf(YLW "SOS connection message recieved. Ring integraty achieved!\n");
                            }
                        }
                        printf(START "└────\n");
                        write(1, INPUT, sizeof(INPUT));
                    }
                }
                // Connection with predecessor
                else if (i == node->fd_pred) // Listen on Predecessor TCP
                {
                    int n_read;

                    memset(message, '\0', BUFFER_SIZE);
                    clean_args(args, BUFFER_SIZE);
                    n_read = read(node->fd_pred, message, BUFFER_SIZE);
                    if (n_read == 0)
                    {
                        if (sos_flag == 1)
                        {
                            sleep(1);
                            sos_counter++;
                            if (sos_counter == 20)
                            {
                                gracefull_leave();
                            }
                        }
                        else
                        {
                            sleep(1);
                            sos_counter = 0;
                            sos_flag = 1;
                            printf(START RED "Connection with predecessor was closed without warning!\n");
                            sos_send(node, NULL, 1);
                        }
                        // printf(RED "The connections with my predecessor was unexpectedly closed!");
                    }
                    sscanf(message, "%s %s %s %s %s %s", args[0], args[1], args[2], args[3], args[4], args[5]);
                    log_info(message, "TCPin");
                    verbose("TCP message received on connection with predecessor");

                    if (strcmp(args[0], "PRED") == 0)
                    {
                        verbose("PRED command recived.");
                        close(node->fd_pred);
                        FD_CLR(node->fd_pred, &available_sockets);
                        TCP_pred_fd = self_send(args, node);
                        FD_SET(TCP_pred_fd, &available_sockets);
                        max_socket = max_socket > TCP_pred_fd ? max_socket : TCP_pred_fd + 1;
                        node->fd_pred = TCP_pred_fd;
                        verbose("Self sent to new predecessor and connection saved");
                        printf(START "├─Received: %s", message);
                        printf(START YLW "├─New connection established!" RESET "\n| ---%d─>%d─>%d---\n", node->pred_i, node->self_i, node->succ_i);
                        printf(START "└────\n");
                        write(1, INPUT, sizeof(INPUT));
                    }
                    else if (strcmp(args[0], "FND") == 0)
                    {
                        /*Received Find request*/
                        verbose("Received FND via TCP");
                        printf(START "├─Received new search: %s", message);
                        find_receive(args, node);
                        write(1, INPUT, sizeof(INPUT));
                    }
                    else if (strcmp(args[0], "RSP") == 0)
                    {
                        /*Received response to find request*/
                        verbose("Received RSP via TCP");
                        printf(START "├─Received new response: %s", message);
                        response_receive(args, node, addr_list);
                        write(1, INPUT, sizeof(INPUT));
                    }
                    else if (strcmp(args[0], "SELF") == 0)
                    {
                        node->succ_i = atoi(args[1]);
                        strcpy(node->succ_ip, args[2]);
                        strcpy(node->succ_port, args[3]);
                        node->fd_succ = node->fd_pred;
                    }
                    else if (strcmp(args[0], "SOS") == 0)
                    {
                        sos_recieve(args, node);
                    }
                }
            }
        }
    }

    for (int i = 0; i <= 5; i++)
        free(args[i]);
    free(args);
    free_node(node);
}