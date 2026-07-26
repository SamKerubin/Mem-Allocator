#include <memalloc.h>
#include <stdio.h>

typedef struct list_node list_node;
struct list_node {
    int val;
    list_node *next;
};

list_node *head;

void list_add(int val) {
    if (!head) {
        head = calloc(1, sizeof(list_node));
        head->val = val;
        return;
    }

    list_node *curr = head;
    while (curr && curr->next) {
        curr = curr->next;
    }

    list_node *next = calloc(1, sizeof(list_node));
    next->val = val;
    curr->next = next;
}

void free_list() {
    list_node *curr = head;
    while (curr) {
        list_node *tmp = curr->next;
        free(curr);
        curr = tmp;
    }
}

int main() {
    list_add(1);
    list_add(16);
    list_add(19);
    list_add(20);
    list_add(56);
    list_add(60);
    list_add(90);
    list_add(50);
    list_add(14);

    list_node *curr = head;
    while (curr) {
        printf("%d\n", curr->val);
        curr = curr->next;
    }

    free_list();
}
