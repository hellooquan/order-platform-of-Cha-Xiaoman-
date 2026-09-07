#include "myfunction.h"

cJSON *strtojson(char *str)
{
    cJSON *obj = cJSON_CreateObject();
    char *token;
    char *key;
    token = strtok(str, "\n:"); // 分割字符串
    if (token == NULL)
        return NULL;                                // 如果字符串为空，返回NULL
    key = strtok(NULL, "\n:");                      // 获取键
    cJSON_AddNumberToObject(obj, token, atoi(key)); // 添加键值对到JSON对象
    while (1)
    {
        token = strtok(NULL, "\n:"); // 获取下一个键
        if (token == NULL)
            break;
        key = strtok(NULL, "\n:");                      // 获取值
        cJSON_AddNumberToObject(obj, token, atoi(key)); // 添加键值对到JSON对象
    }
    return obj; // 返回JSON对象
}

// 打开点餐列表
int open_list(char *filename)
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    return fd;
}

void write_ordertxt(char *str, int fd)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", t);
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s\n%s\n", timebuf, str);
    write(fd, buf, strlen(buf));
}

void close_list(int fd)
{
    close(fd);
}
