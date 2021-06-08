#ifndef LIST_ADT_H
#define LIST_ADT_H

#include <stdbool.h>

typedef struct list_cdt * list_adt; /*Puntero al head */

list_adt new_list(int dataSize);
bool add_to_list(list_adt l, void* data);
void free_list(list_adt l);
int size(list_adt l);
void to_begin(list_adt l);
void * next(list_adt l);
bool has_next(list_adt l);

#endif