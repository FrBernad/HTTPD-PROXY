#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "listADT.h"

// Node
typedef struct node {
    void* data;
    struct node* next;
} node;

struct list_cdt {
    node* first;
    node* current;
    int dataSize;
    int size;
};

static void freeListNode(node* n);

list_adt new_list(int dataSize) {
    struct list_cdt* list = calloc(1, sizeof(struct list_cdt));

    if (list == NULL) {
        return NULL;
    }
    list->dataSize = dataSize;

    return list;
}

void free_list(list_adt l) {
    freeListNode(l->first);
    free(l);
}

static void freeListNode(node* n) {
    if (n == NULL) {
        return;
    }
    freeListNode(n->next);
    free(n);
}

int size(list_adt l) {
    return l->size;
}

int isEmptyList(list_adt l) {
    return l->size == 0;
}

bool add_to_list(list_adt l, void* data) {  //Devuelve false si no lo pudo agregar

    node* newNode = malloc(sizeof(node));
    if (newNode == NULL) {
        return false;
    }
    newNode->data = malloc(sizeof(l->dataSize));
    if (newNode->data == NULL) {
        free(newNode);
        return false;
    }
    memcpy(newNode->data, data, l->dataSize);
    newNode->next = l->first;
    l->first = newNode;
    l->size += 1;
    return true;
}

void to_begin(list_adt l) {
    l->current = l->first;
}

void* next(list_adt l) {
    void* data = l->current->data;
    l->current = l->current->next;
    return data;
}

bool has_next(list_adt l) {
    return l->current != NULL;
}
