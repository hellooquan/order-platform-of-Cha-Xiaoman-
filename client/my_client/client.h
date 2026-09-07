#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include <stdint.h>

void client_send(void);
void set_milktea_name_count(const char * milktea_name,int32_t count);
void set_milktea_idx_count(int idx, int32_t count);
void total_price(uint32_t total);
void send_end(void);

/*============main==================*/
void client_init(const char *ip, int port);
void client_close(void);
void recv_pthread_init(void);

#endif