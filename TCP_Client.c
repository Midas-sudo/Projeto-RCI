#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#define PORT “58001”

int main(int argc, char **argv)
{
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1)
        exit(1); // error

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket
    printf("check0\n");
    errcode = getaddrinfo("192.168.1.113", "58001", &hints, &res);
    if (errcode != 0) /*error*/
        exit(1);
    printf("check1\n");
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if (n == -1) /*error*/
        exit(1);
    printf("check2\n");
    n = write(fd, "Helllo!\n", 7);
    if (n == -1) /*error*/
        exit(1);
    printf("check3\n");
    while (1)
        continue;
    // n = read(fd, buffer, 128);
    // if (n == -1) /*error*/
    //     exit(1);
    // write(1, "echo: ", 6);
    // write(1, buffer, n);
    // freeaddrinfo(res);
    // close(fd);
}
