#ifndef AUDIOSYNTH_LINKED_LIST_H
#define AUDIOSYNTH_LINKED_LIST_H

#include <sys/types.h>

struct LinkedListNode {
    void* data;
    struct LinkedListNode* next;
};

struct LinkedListNode* createLinkedListNode(size_t size);
void cleanupLinkedListNodes(struct LinkedListNode* node);

#endif
