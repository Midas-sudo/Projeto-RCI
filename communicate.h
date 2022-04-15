#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include "ring.h"
#include "utils.h"
#include "node.h"

typedef struct
{
    int searched_key;
    int seq_number;
    struct sockaddr_in addr;
} UDP_addr_list;

void gracefull_leave();

int TCP_setup(char *port);

int UDP_setup(char *port);

void read_tcp(int fd, char *message, int message_size);

int send_tcp(int fd, char *message);

int send_udp(char *ip, char *port, char *message, int max_tries);

int self_send(char **args, Node_struct *node);

void self_receive(char **args, Node_struct *node, int fd);

void pred_send(Node_struct *node);

void pred_receive(char **args, Node_struct *node);

char **efnd_send(char **args, Node_struct *node);

void epred_send(char **args, Node_struct *node, struct sockaddr_in addr, int seq);

void find_send_TCP(char **args, Node_struct *node, int seq);

void find_send_UDP(char **args, Node_struct *node, int seq);

void response_send_TCP(char **args, Node_struct *node, int isfirst);

void response_send_UDP(char **args, Node_struct *node, int isfirst);

void response_receive(char **args, Node_struct *node, UDP_addr_list *addr_vect);

void find_receive(char **args, Node_struct *node);

void show(Node_struct *node);

void find(char **args, Node_struct *node, UDP_addr_list *addr_list, int seq_number, struct sockaddr_in addr);

void setup(fd_set *available_sockets, Node_struct *node, int *max_socket, int *TCP_fd, int *UDP_fd);

void leave(Node_struct *node, fd_set *available_sockets);

void sos_send(Node_struct *node, char **args, int seq);

void sos_recieve(char **args, Node_struct *node);

void INThandler(int sig);

#endif