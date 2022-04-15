#ifndef UTILS_H
#define UTILS_H

#include "ring.h"
#include "node.h"

extern int verb, disp_info;

void disp_header();

void command_list(int i, char *ip, char *port);

void log_info(char *str, char *mode);

void verbose(char *str);

void clean_args(char **args, int size);

// calculates the distance from node/key i to node/key j
int dist(int i, int j);

//
int check_dist(Node_struct *node, int key);

// prompts user for a command (and its args) and returns the number of args read
int get_command(char **args);

// checks if a command is valid
int check_command(char **args, int num_read, Node_struct *node);

void INThandler(int sig);

void check_call(int argc, char **argv);

#endif
