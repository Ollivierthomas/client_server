#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include "common.h"
#include <pthread.h>
#include "client_array.h"
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>

static volatile bool should_stop = false;

struct ThreadData {
    int client_socket;
    struct ClientArray* client_array;
};

void receive_messages_loop(int client_socket, struct ClientArray* client_array) {
    char buffer[1024];
    while (1) {
        int result = receive_message(client_socket, buffer, sizeof(buffer));
        if (result == -1) {
            ca_remove(client_array, client_socket);
            return;
        }
        printf("%s\n", buffer);
        for (int i = 0; i < client_array->size; i++) {
            send_message(client_array->clients[i].socket, buffer);
        }
    }
}

void* receive_messages_thread(void* arg) {
    struct ThreadData data = *((struct ThreadData*)arg);
    receive_messages_loop(data.client_socket, data.client_array);
    return NULL;
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


    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(8989);      

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    const int ON = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &ON, sizeof(ON)) < 0) {
        perror("Failed setting socket options");
        exit(EXIT_FAILURE);
    }

    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Failed to bind adress");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8989...\n");

    struct ClientArray array = ca_init();

    while (!should_stop) {
        struct sockaddr_in client_adress;
        
        unsigned int size = sizeof(client_adress);

        int client_socket = accept(server_socket, (struct sockaddr *)&client_adress, &size);
        
        if (client_socket < 0) {
            perror("\nError accepting client connection\n");
        }
        else {
            pthread_t thread_id;

            printf("A client is connected !\n");

            struct ThreadData data = {
                .client_socket = client_socket,
                .client_array = &array,
            };

            if (pthread_create(&thread_id, NULL, receive_messages_thread, &data) != 0) {
                perror("Failed to create thread\n");
                exit(EXIT_FAILURE);
            }

            pthread_detach(thread_id);

            struct Client client = {
                .socket = client_socket,
                .thread = thread_id,
            };

            ca_add(&array, client);
        }
    }
    for (int i = 0; i < array.size; i++) {
        shutdown(array.clients[i].socket, SHUT_RDWR);
        close(array.clients[i].socket);
    }
    printf("Server off");
}