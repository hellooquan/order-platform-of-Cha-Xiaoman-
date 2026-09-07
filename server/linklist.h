#ifndef LINKLIST_H
#define LINKLIST_H

#include "myhead.h"

extern struct node *link_head;
extern int link_count;

struct node
{
    int sockfd;
    char ip[16];
    unsigned short port;

    struct node *next;
};

struct node *init_node(void);
void add_tial(struct node *node, struct node *head);
void release(int fd);

#endif