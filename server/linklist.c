#include "linklist.h"

pthread_mutex_t link_mutex = PTHREAD_MUTEX_INITIALIZER;

struct node *link_head = NULL;
int link_count = 0;

struct node *init_node(void)
{
    struct node *head = malloc(sizeof(struct node));
    head->next = NULL;
    return head;
}

void add_tial(struct node *node, struct node *head)
{
    pthread_mutex_lock(&link_mutex);
    struct node *p = head;
    while (p->next != NULL)
        p = p->next;
    p->next = node;
    pthread_mutex_unlock(&link_mutex);
}

void release(int fd)
{
    if (link_head == NULL)
        return;
    pthread_mutex_lock(&link_mutex);
    struct node *delptr = link_head->next;
    struct node *p = link_head;
    bool find = false;
    while (delptr != NULL)
    {
        if (delptr->sockfd == fd)
        {
            find = true;
            break;
        }
        p = delptr;
        delptr = delptr->next;
    }
    if (find)
    {
        p->next = delptr->next;
        free(delptr);
        close(fd);
        printf("客户端链表节点释放，当前连接服务器的用户数为:%d\n", --link_count);
    }
    pthread_mutex_unlock(&link_mutex);
}
