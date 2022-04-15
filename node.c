#include "node.h"

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
