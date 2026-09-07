#ifndef _MYFUNCTION_H_
#define _MYFUNCTION_H_

#include "myhead.h"

#define FILENAME "/home/hyq/project/order_platrorm/client/server/order_list.txt" // 点餐列表文件名

cJSON *strtojson(char *str);
int open_list(char *filename);
void write_ordertxt(char *str,int fd);
void close_list(int fd);

#endif // _MYFUNCTION_H_