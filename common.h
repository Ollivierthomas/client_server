#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

int send_message(int socket, char* message);

int receive_message(int socket, char* message, size_t max_length);

#endif