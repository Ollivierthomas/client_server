#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

static volatile bool should_stop = false;

void receive_messages_loop(int client_socket) {
    char buffer[1024];
    while (!should_stop) {
        if (receive_message(client_socket, buffer, sizeof(buffer)) == 0) {
            printf("%s\n", buffer);
        }
        else {
            should_stop = true;
        }
    }
}

void* receive_messages_loop_thread(void* arg) {
    int socket = *((int*)arg);
    receive_messages_loop(socket);
    return NULL;
}

void send_message_loop(int client_socket) {
    char buffer[1024];
    while (!should_stop) {

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            perror("Error reading from stdin");
            exit(EXIT_FAILURE);
        }
        send_message(client_socket, buffer);
    }
}

void handle_sigint(int signal) {
    (void)signal;
    should_stop = true;
}

int main() {

    struct sigaction signal;
    signal.sa_handler = handle_sigint;
    sigemptyset(&signal.sa_mask);
    signal.sa_flags = 0;

    if (sigaction(SIGINT, &signal, NULL) < 0) {
        perror("Failed to setup sigaction");
        exit(EXIT_FAILURE);
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Failed to create socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    server_address.sin_port = htons(8989); 

    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection to server failed\n");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Client connected !\n");

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, receive_messages_loop_thread, &client_socket) != 0) {
        perror("Failed to create thread\n");
        exit(EXIT_FAILURE);
    }

    send_message_loop(client_socket);

    close(client_socket);
    printf("Disconnected\n");
}