#include "common.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int send_message(int socket, char* message) {
    if (send(socket, message, strlen(message), 0) < 0) {
        perror("Error sending message\n");
        return -1;
    }
    else {
        return 0;
    }
}

int receive_message(int socket, char* message, size_t max_length) {
    ssize_t receive = recv(socket, message, max_length - 1, 0);

    if (receive < 0) {
        perror("Error receiving message\n");
        return -1;
    }
    else if (receive == 0) {
        printf("Peer disconnected\n");
        return -1;
    }
    else {
        message[receive] = '\0';

        if (receive > 0 && message[receive - 1] == '\n') {
            message[receive - 1] = '\0';
        }

        return 0;
    }

}