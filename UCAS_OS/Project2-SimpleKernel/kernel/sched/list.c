#include <os/list.h>

// Init list
void list_init(list_head *list){
    list -> next = list;
    list -> prev = list;
}

// If a list is empty
int is_list_empty(list_head *list){
    if (list -> next == list) return 1;
    return 0;
}

// Push a node into list tail
void enqueue(list_head *list, list_node_t *item){
    list -> prev -> next = item;
    item -> prev = list -> prev;
    item -> next = list;
    list -> prev = item;
}

// Pop a node from list head
list_node_t * dequeue(list_head *list){
    list_node_t *data = list -> next;
    list -> next = list -> next -> next;
    list -> next -> prev = list;
    return data;
}