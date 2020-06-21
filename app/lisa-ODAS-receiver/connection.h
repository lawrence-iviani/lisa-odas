
#ifndef CONNECTION 
#define CONNECTION

#include "common.h"

#include <netinet/in.h>

#include <sys/socket.h>

#include <errno.h>
#include <fcntl.h> /* Added for the nonblocking socket */
#include <unistd.h> /*usleep */
#include <sys/ioctl.h> // FIONREAD


int init_connection(sockaddr_in &server_address, int port_number, int backlog);
int accept_connection(int server_id);
bool reception_terminate(unsigned int rcvd_bytes[]);

#endif