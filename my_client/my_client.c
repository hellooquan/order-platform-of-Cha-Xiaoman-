#include "my_client.h"

static int sockfd;
// 点奶茶数量
static int Pearl_Milk_Tea_count = 0;
static int Taro_Boba_Milk_Tea_count = 0;
static int Mango_Pomelo_Sago_count = 0;
static int Grape_Jelly_Tea_count = 0;
static int Four_Seasons_Lemon_Tea_count = 0;
static uint32_t order_total = 0;

// 客户端初始化，连接服务器
void client_init(const char *ip, int port)
{
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    static struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)(&servaddr), sizeof(servaddr)) < 0)
    {
        perror("Connection to server failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

// Send message to server
void client_send(void)
{
    char message[1024];
    bzero(message, sizeof(message));
    int len = 0;
    if (Pearl_Milk_Tea_count > 0)
    {
        len += snprintf(message + len, sizeof(message) - len, "Pearl milk tea: %d\n", Pearl_Milk_Tea_count);
    }
    if (Taro_Boba_Milk_Tea_count > 0)
    {
        len += snprintf(message + len, sizeof(message) - len, "Taro boba milk tea: %d\n", Taro_Boba_Milk_Tea_count);
    }
    if (Mango_Pomelo_Sago_count > 0)
    {
        len += snprintf(message + len, sizeof(message) - len, "Mango pomelo sago tea: %d\n", Mango_Pomelo_Sago_count);
    }
    if (Grape_Jelly_Tea_count > 0)
    {
        len += snprintf(message + len, sizeof(message) - len, "Grape jelly tea: %d\n", Grape_Jelly_Tea_count);
    }
    if (Four_Seasons_Lemon_Tea_count > 0)
    {
        len += snprintf(message + len, sizeof(message) - len, "Four seasons lemon tea: %d\n", Four_Seasons_Lemon_Tea_count);
    }
    len += snprintf(message + len, sizeof(message) - len, "Total price: %d\n", order_total);
    printf("[line:%d]:%s\n", __LINE__, __FUNCTION__);
    // Send message to server
    send(sockfd, message, strlen(message), 0);

    printf("[line:%d]:%s\n", __LINE__, __FUNCTION__);
}

void set_milktea_name_count(const char *milktea_name, int32_t count)
{
    if (strcmp(milktea_name, "Pearl Milk Tea") == 0)
        Pearl_Milk_Tea_count = count;
    else if (strcmp(milktea_name, "Taro Boba Milk Tea") == 0)
        Taro_Boba_Milk_Tea_count = count;
    else if (strcmp(milktea_name, "Mango Pomelo Sago Tea") == 0)
        Mango_Pomelo_Sago_count = count;
    else if (strcmp(milktea_name, "Grape Jelly Tea") == 0)
        Grape_Jelly_Tea_count = count;
    else if (strcmp(milktea_name, "Four Seasons Lemon Tea") == 0)
        Four_Seasons_Lemon_Tea_count = count;
    printf("[line:%d]:%s\n", __LINE__, __FUNCTION__);
}

void set_milktea_idx_count(int idx, int32_t count)
{
    switch (idx)
    {
    case 0:
        Pearl_Milk_Tea_count = count;
        break;
    case 1:
        Taro_Boba_Milk_Tea_count = count;
        break;
    case 2:
        Mango_Pomelo_Sago_count = count;
        break;
    case 3:
        Grape_Jelly_Tea_count = count;
        break;
    case 4:
        Four_Seasons_Lemon_Tea_count = count;
        break;
    default:
        break;
    }
}

// Calculate total price based on the counts of each item
void total_price(uint32_t total)
{
    order_total = total;
    printf("[line:%d]:%s\n", __LINE__, __FUNCTION__);
}

void send_end(void)
{
    send(sockfd, "end", 4, 0);
}

void client_close(void)
{
    close(sockfd);
}

void *client_recv(void *arg)
{
    int sockfd = (int)(intptr_t)arg;
    char buffer[1024];
    while (1)
    {
        bzero(buffer, sizeof(buffer));
        int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n < 0)
        {
            perror("Receive failed");
            exit(EXIT_FAILURE);
        }
        if (n == 0)
        {
            printf("Server closed connection\n");
            exit(0);
        }
        printf("Received message: %s\n", buffer);
    }
    return NULL;
}

void recv_pthread_init(void)
{
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, (void *(*)(void *))client_recv, (void *)(intptr_t)sockfd);
    pthread_detach(recv_thread);
}