#include "ring.h"
#include "utils.h"
#include "communicate.h"
#include "node.h"

int BUFFER_SIZE = 256;
int verb = 0;
int disp_info = 1;

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
                        if (atoi(args[1]) == node->self_i)
                            printf(START RED "Chord can't be the same as node!\n" RESET);
                        else
                        {
                            node->chord_i = atoi(args[1]);
                            strcpy(node->chord_ip, args[2]);
                            strcpy(node->chord_port, args[3]);
                            printf(START IST "|Chord sucessfully added!\n" RESET);
                            printf(START "└────\n");
                        }
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