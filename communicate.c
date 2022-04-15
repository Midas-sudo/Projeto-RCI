#include "communicate.h"

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

int send_tcp(int fd, char *message)
{
    int n_left, n_written;
    char *ptr = message;

    n_left = strlen(message);
    while (n_left > 0)
    {
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
        log_info(message, "UDPout");
        n_tries++;
        if (n < strlen(message))
            printf(RED START "├─ERROR sending full UDP message\n" RESET);
        do
        { // wait for acknowledgement from peer, ignores messages from other addresses
            memset(response, '\0', BUFFER_SIZE);
            if ((n = recvfrom(fd, response, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addrlen)) < 0)
                break; // connection timed out
            log_info(response, "UDPin");
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
        log_info(message, "UDPout");
        n_tries++;
        if (n < sizeof(message))
            printf(RED START "├─ERROR sending UDP message\n" RESET);
        do
        { // wait for acknowledgement from peer, ignores messages from other addresses
            memset(response, '\0', BUFFER_SIZE);
            if ((n = recvfrom(fd, response, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addrlen)) < 0)
                break; // connection timed out
            log_info(response, "UDPin");
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
    // Wait for EPRED response
    do
    {
        memset(response, '\0', BUFFER_SIZE);
        n = recvfrom(fd, response, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addrlen);
        if (n < 0)
            continue;
        log_info(response, "UDPin");
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
    log_info("ACK", "UDPout");
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
            log_info(message, "UDPout");
            n_tries++;
            if ((n < sizeof(message)) == 1)
                printf(RED START "├─ERROR sending UDP message\n" RESET);
            do
            { // wait for acknowledgement from peer, ignores messages from other addresses
                memset(response, '\0', BUFFER_SIZE);
                if ((n = recvfrom(fd, response, BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &addrlen)) < 0)
                    break; // connection timed out
                log_info(response, "UDPin");
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
            printf(RED START "├─Error sending TCP message to Sucessor!\n");
        log_info(message, "TCPout");
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
            printf(RED START "├─Error sending TCP message to Sucessor!\n");
        log_info(message, "TCPout");
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
