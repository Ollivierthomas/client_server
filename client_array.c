#include "client_array.h"
#include <stdio.h>
#include <stdlib.h>

struct ClientArray ca_init() {
    struct ClientArray clients = {
        .clients = malloc(2 * sizeof(struct Client)),
        .size = 0,
        .capacity = 2,
    };

    if (clients.clients == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    return clients;
}

void ca_add(struct ClientArray* array, struct Client client) {
    if (array == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    if (array->size == array->capacity) {
        int new_capacity = array->capacity * 2;
        struct Client* new_clients = realloc(array->clients, new_capacity * sizeof(struct Client));
        if (new_clients == NULL) {
            perror("Memory allocation failed"),
            exit(EXIT_FAILURE);
        }
        array->clients = new_clients;
        array->capacity = new_capacity;
    }

    array->clients[array->size] = client;
    array->size++;
}

void ca_remove(struct ClientArray* array, int socket) {
    for (int i = 0; i < array->size; i++) {
        if (array->clients[i].socket == socket) {
            array->clients[i] = array->clients[array->size - 1];
            array->size--;
        }
    }
}