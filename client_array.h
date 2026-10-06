#ifndef CLIENT_ARRAY_H
#define CLIENT_ARRAY_H

#include <pthread.h>

struct Client {
    int socket;
    pthread_t thread;
};

struct ClientArray {
    struct Client* clients;
    int size;
    int capacity;
};

struct ClientArray ca_init();

void ca_add(struct ClientArray* array, struct Client client);

void ca_remove(struct ClientArray* array, int socket);


#endif