#include "linked_list.h"

#include <stdlib.h>

struct LinkedListNode* createLinkedListNode(size_t size)
{
    struct LinkedListNode* node
        = (struct LinkedListNode*)malloc(sizeof(struct LinkedListNode));
    node->data = malloc(size);
    node->next = NULL;
    return node;
}

void cleanupLinkedListNodes(struct LinkedListNode* node)
{
    while (node != NULL) {
        free(node->data);
        struct LinkedListNode* next = node->next;
        free(node);
        node = next;
    }
}
