//
//  main.c
//  ipcMsgServer
//
//  Created by Daniel Sandoval on 05/11/2013.
//  Copyright (c) 2013 Departamento de Ciência da Computação - Universidade de Brasília. All rights reserved.
//

#include <stdio.h>
#include "../../ipcMessaging/ipcMessaging/ipcMessaging.h"
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>

#define POOL_SIZE 5
typedef struct intList intList;
struct connectedClients {
    int number;
    intList* clients;
};

struct intList {
    int value;
    intList* next;
};

//Methods that handle client pooling
int clientIsConnected(int client);
int connectClient(int client);
int disconnectClient(int client);

//Methods that handle lists of int (intList)
int hasValue(int value, intList* list);
void removeValue(int value, intList **list);
intList *putIntoIntList(int value, intList *list);

struct connectedClients connectedClients;
pthread_mutex_t clientConnectionMutex = PTHREAD_MUTEX_INITIALIZER;

void treatSigint() {
    printf("Received SIGINT!\n");
    tearDown();
    exit(1);
}

void entryMessageHandler(message* msg) {
    //printf("[SERVER] Handling entryMessage\n");
    message response;
    response.mdata.destination = msg->mdata.source;
    response.mdata.referencedMessageId = msg->mdata.messageId;
    response.mtype = 1;
    if (connectClient(msg->mdata.source)) {
        response.mdata.text[0] = 'y';
    } else {
        response.mdata.text[0] = 'n';
    }
    //printf("[SERVER] entryMessage check\n");
    sendMessage(&response);
    //printf("[SERVER] Client %d connected: %c\n", msg->mdata.source, response.mdata.text[0]);
}

void *entryManager(void *threadid) {
    listenForMessages(1, entryMessageHandler);
    return NULL;
}

void exitMessageHandler(message* msg) {
    //printf("[SERVER] Handling exitMessage\n");
    message response;
    response.mdata.destination = msg->mdata.source;
    response.mdata.referencedMessageId = msg->mdata.messageId;
    response.mtype = 2;
    if (disconnectClient(msg->mdata.source)) {
        response.mdata.text[0] = 'y';
    } else {
        response.mdata.text[0] = 'n';
    }
    //printf("[SERVER] exitMessage check\n");
    sendMessage(&response);
    //printf("[SERVER] Client %d disconnected: %c\n", msg->mdata.source, response.mdata.text[0]);
}

void *exitManager(void *threadid) {
    listenForMessages(2, exitMessageHandler);
    return NULL;
}

void printMessageHandler(message* msg) {
    //printf("[SERVER] Handling printMessage\n");
    message response;
    response.mdata.destination = msg->mdata.source;
    response.mdata.referencedMessageId = msg->mdata.messageId;
    response.mtype = 3;
    if (clientIsConnected(msg->mdata.source)) {
        printf("%s", msg->mdata.text);
        response.mdata.text[0] = 'y';
    } else {
        response.mdata.text[0] = 'n';
    }
    //printf("[SERVER] printMessage check\n");
    sendMessage(&response);
}

void *printManager(void *threadid) {
    listenForMessages(3, printMessageHandler);
    return NULL;
}

int main(int argc, const char * argv[])
{
    struct sigaction action;
    pthread_t threads[2];

    action.sa_handler = treatSigint;
    sigaction(SIGINT, &action, NULL);
    
    setup(INT_MAX);
    
    if (pthread_create(&threads[0], NULL, entryManager, NULL)) {
        printf("ERROR creating entryManager thread.\n");
        exit(1);
    }
    if (pthread_create(&threads[1], NULL, exitManager, NULL)) {
        printf("ERROR creating exitManager thread.\n");
        exit(1);
    }
    
    //Start print manager on main thread.
    (void)printManager(NULL);
    
    tearDown();
    
    return 0;
}

int clientIsConnected(int client) {
    int ret;
    pthread_mutex_lock(&clientConnectionMutex);
    ret = hasValue(client, connectedClients.clients);
    pthread_mutex_unlock(&clientConnectionMutex);
    return ret;
}

int connectClient(int client) {
    pthread_mutex_lock(&clientConnectionMutex);
    if (hasValue(client, connectedClients.clients)) {
        pthread_mutex_unlock(&clientConnectionMutex);
        return 1;
    }
    if (connectedClients.number >= POOL_SIZE) {
        pthread_mutex_unlock(&clientConnectionMutex);
        return 0;
    }
    connectedClients.number++;
    connectedClients.clients = putIntoIntList(client, connectedClients.clients);
    pthread_mutex_unlock(&clientConnectionMutex);
    return 1;
}

int disconnectClient(int client) {
    pthread_mutex_lock(&clientConnectionMutex);
    if (hasValue(client, connectedClients.clients)) {
        connectedClients.number--;
        removeValue(client, &(connectedClients.clients));
        pthread_mutex_unlock(&clientConnectionMutex);
        return 1;
    }
    pthread_mutex_unlock(&clientConnectionMutex);
    return 0;
}

int hasValue(int value, intList* list) {
    while (list != NULL) {
        if (list->value == value)
            return 1;
        list = list->next;
    }
    return 0;
}

void removeValue(int value, intList **list) {
    intList *tmp, *previous;
    tmp = (*list);
    if (tmp != NULL && tmp->value == value) {
        (*list) = (*list)->next;
        free(tmp);
    } else if (tmp != NULL) {
        do {
            previous = tmp;
            tmp = tmp->next;
        } while (tmp != NULL && tmp->value != value);
        if (tmp != NULL) {
            previous->next = tmp->next;
            free(tmp);
        }
    }
}

intList *putIntoIntList(int value, intList *list) {
    if (hasValue(value, list))
        return list;
    
    intList* element = malloc(sizeof(intList));
    element->value = value;
    element->next = list;
    return element;
}

