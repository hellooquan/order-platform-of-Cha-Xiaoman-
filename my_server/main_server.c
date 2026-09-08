#include "myhead.h"
#include "my_linklist.h"
#include "myfunction.h"

static int sockfd;
unsigned int client_id = 1; // 客户端id

// 线程函数，接收客户端数据
void *recvfunction(void *arg)
{
    pthread_detach(pthread_self());
    int connfd = (int)(intptr_t)arg;

    // 创建ipv4地址结构体变量，存放客户端的ip地址和端口号
    struct sockaddr_in clientAddr;
    bzero(&clientAddr, sizeof(clientAddr));
    socklen_t addrLen = sizeof(clientAddr);
    getpeername(connfd, (struct sockaddr *)&clientAddr, &addrLen);

    // 将客户端信息放进链表节点，并连接
    // 创建新链表节点
    struct node *newnode = malloc(sizeof(struct node));
    newnode->sockfd = connfd;
    strcpy(newnode->ip, inet_ntoa(clientAddr.sin_addr));
    newnode->port = ntohs(clientAddr.sin_port);
    newnode->next = NULL;
    // 将新的节点连接到链表(尾)
    add_tial(newnode, link_head);

    // 打开点餐列表文件
    int fd = open_list(FILENAME);
    if (fd < 0)
    {
        perror("打开点餐列表文件失败");
        release(connfd);
        return NULL;
    }

    char recvbuf[1024];
    // 接收客户端数据
    while (1)
    {
        bzero(recvbuf, sizeof(recvbuf));
        // recv接受客户端数据
        int ret = recv(connfd, recvbuf, sizeof(recvbuf) - 1, 0);
        if (ret < 0)
        {
            perror("recv error");
            break;
        }
        if (ret == 0)
            break;
        recvbuf[ret] = '\0';

        // 将接收到的数据转换为json格式
        cJSON *json = strtojson(recvbuf);
        if (json != NULL)
        {
            char *json_str = cJSON_Print(json);
            write_ordertxt(json_str, fd);
            printf("客户点餐:\n%s\n", json_str);
            free(json_str);
            cJSON_Delete(json);
            if (send(connfd, "点餐成功", strlen("点餐成功"), 0) < 0)
                perror("send failed");
        }
    }

    close_list(fd); // 关闭点餐列表文件
    release(connfd);

    return NULL;
}

int main(int argc, char const *argv[])
{
    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 套接字绑定ip
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(50021); // 端口号PORT
    // 取消端口号绑定限制
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // 地址绑定
    bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    // 监听
    listen(sockfd, 3);

    // 初始化链表
    link_head = init_node();

    // 等待客户端连接
    while (1)
    {
        // 等待连接
        int connfd = accept(sockfd, NULL, NULL);

        // link_count 与链表是共享状态,和断开线程(release)互斥
        pthread_mutex_lock(&link_mutex);
        link_count++;
        printf("有一个新的客户端连接，当前连接服务器的用户数为：%d\n", link_count);
        pthread_mutex_unlock(&link_mutex);

        // 客户端连接后创建线程
        pthread_t tid;
        pthread_create(&tid, NULL, recvfunction, (void *)(intptr_t)connfd);
    }

    close(sockfd);

    _exit(0);
}
