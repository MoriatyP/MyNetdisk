#ifndef __CLIENT_H__
#define __CLIENT_H__
#include "../include/head.h"
#include "../include/md5.h"

#define DATA_BUF_LEN 1024

typedef struct
{
    int datalen;
    char buf[DATA_BUF_LEN];
} datapackage_t;

int connect_with_server(int *socketfd,char *ip,char *port);
void system_pause();
void start_window();
void mainpage_window();
int do_register(int socketfd);
int do_login(int socketfd, int *home_id,int *user_id);
int do_gets(int socketfd, int dir_id, int user_id);
int do_puts(int socketfd, int dir_id, int user_id);
int do_mkdir(int socketfd, int dir_id, int user_id);
int do_cd(int socketfd, int *dir_id, int user_id);
int do_ls(int socketfd, int dir_id, int user_id);
void do_lls();
int do_pwd(int socketfd, int dir_id, int user_id);
int do_remove(int socketfd, int dir_id, int user_id);

int send_cycle(int newfd, void *pbuf, int len);
int recv_cycle(int newfd, void *pbuf, int len);

#endif