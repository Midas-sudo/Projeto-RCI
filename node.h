#ifndef NODE_H
#define NODE_H

#include "ring.h"

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

Node_struct *global_node;

Node_struct *new_node(int i, char *ip, char *port);

void free_node(Node_struct *node);

#endif