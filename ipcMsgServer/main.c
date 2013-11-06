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

void treatSigint() {
    printf("Received SIGINT!\n");
    tearDown();
    exit(1);
}

void receiveMessage(message* msg) {
    printf("[SERVER] Message %ld received from %d\n", msg->mdata.messageId, msg->mdata.source);
    message response;
    response.mdata.destination = msg->mdata.source;
    response.mdata.referencedMessageId = msg->mdata.messageId;
    if (msg->mtype == 1) {
        response.mtype = 1;
        if (connectClient(msg->mdata.source)) {
            response.mdata.text[0] = 'y';
        } else {
            response.mdata.text[0] = 'n';
        }
        printf("[SERVER] Client %d connected: %c\n", msg->mdata.source, response.mdata.text[0]);
    } else if (msg->mtype == 2) {
        response.mtype = 2;
        if (disconnectClient(msg->mdata.source)) {
            response.mdata.text[0] = 'y';
        } else {
            response.mdata.text[0] = 'n';
        }
        printf("[SERVER] Client %d disconnected: %c\n", msg->mdata.source, response.mdata.text[0]);
    } else if (msg->mtype == 3) {
        response.mtype = 3;
        if (clientIsConnected(msg->mdata.source)) {
            printf("%s", msg->mdata.text);
            response.mdata.text[0] = 'y';
        } else {
            response.mdata.text[0] = 'n';
        }
    } else {
        return;
    }
    sendMessage(&response);
}

int main(int argc, const char * argv[])
{
    struct sigaction action;

    action.sa_handler = treatSigint;
    sigaction(SIGINT, &action, NULL);
    
    setup(INT_MAX);
    
    listenForMessages(0, receiveMessage);
    
    tearDown();
    
    return 0;
}

int clientIsConnected(int client) {
    return hasValue(client, connectedClients.clients);
}

int connectClient(int client) {
    if (hasValue(client, connectedClients.clients))
        return 1;
    if (connectedClients.number >= POOL_SIZE)
        return 0;
    connectedClients.number++;
    connectedClients.clients = putIntoIntList(client, connectedClients.clients);
    return 1;
}

int disconnectClient(int client) {
    if (hasValue(client, connectedClients.clients)) {
        connectedClients.number--;
        removeValue(client, &(connectedClients.clients));
        return 1;
    }
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

