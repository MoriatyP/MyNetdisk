#ifndef __SERVER_H__
#define __SERVER_H__
#include "head.h"
#include "md5.h"

//queue相关
typedef struct
{
    int op;
    int fd;
}task_t,*ptask_t;

typedef struct
{
        ptask_t BUF;
        pthread_mutex_t queMutex;
        int front;
        int rear;
        int quesize;
}queue_t,*pqueue_t;

void init_queue(queue_t *q, int quesize);
void free_queue(queue_t *q);
int is_empty_queue(queue_t *q);
int is_full_queue(queue_t *q);
void in_queue(queue_t *q, task_t value);
void de_queue(queue_t *q, ptask_t value);

//config相关
#define MAX_CONFIG 50
#define CONFIG_LEN 100

typedef struct
{
    char *key;
    char *data;
}config_t,*pconfig_t;

typedef struct 
{
    char ipstr[CONFIG_LEN];
    char portstr[CONFIG_LEN];
    char threadnum1str[CONFIG_LEN];
    char threadnum2str[CONFIG_LEN];
    char quesizestr[CONFIG_LEN];
    char maxclientstr[CONFIG_LEN];
    char mysqlpathstr[CONFIG_LEN];
    char logpathstr[CONFIG_LEN];
}server_config;

int read_config(char *path,pconfig_t config);
int get_config_data(pconfig_t config,int confignum,char *key,char *data);
int get_server_config(pconfig_t config,int config_num,server_config *message);

//mysql相关
#define SQL_LEN 1024

int mysql_connect(MYSQL **conn,char *mysql_path);
MYSQL_RES *mysql_select(MYSQL* conn, const char* table, const char* field, const char* data);
MYSQL_RES *mysql_select_int(MYSQL* conn, const char* table, const char* field, int data);
MYSQL_RES *mysql_select_two(MYSQL* conn, const char* table, const char* field1, const char* data1,const char* field2, int data2);
MYSQL_RES *mysql_select_dir(MYSQL* conn, const char* table, const char* field1, int data1,const char* field2, int data2);
int mysql_insert_user(MYSQL* conn, const char* user_name, const char* password,const char* salt);
int mysql_insert_dir(MYSQL* conn, int pre_id, int user_id, const char* dir_name);
int mysql_insert_file(MYSQL* conn, int pre_id, int user_id, const char* filename,const char* real_filename,off_t filesize,const char* md5sum);
int mysql_delete_user(MYSQL* conn, const char* user_name, const char* password);
int mysql_delete_file(MYSQL* conn, int file_id);

//注册与登录相关
#define SALT_LEN 8//定义盐值长度

char* generate_salt();
int do_register(int taskfd, MYSQL *conn);
int do_login(int taskfd, MYSQL *conn, int *hashtable,int epfd);

//下载和上传相关
int do_gets(int taskfd, MYSQL *conn);
int do_puts(int taskfd, MYSQL *conn);

//线程池相关
#define DATA_BUF_LEN 1024

typedef struct
{
    queue_t que;
    int *hashtable;
    MYSQL *conn;
    int epfd;
    pthread_cond_t cond;
    pthread_t *pthid;
    int threadnum;
    int startflag;
} threadPool_t, *pthreadPool_t;

typedef struct
{
    int datalen;
    char buf[DATA_BUF_LEN];
} datapackage_t;

int thread_pool_init(pthreadPool_t p, int threadnum, int quesize, int *hashtable, MYSQL *conn,int epfd);
int thread_pool_start1(pthreadPool_t p);
int thread_pool_start2(pthreadPool_t p);
int tcp_init(char *, char *);
void *threadFunc1(void *);
void *threadFunc2(void *);
int do_mkdir(int tag, MYSQL *conn);
int do_cd(int tag, MYSQL *conn);
int do_ls(int tag,MYSQL *conn);
void get_path_string(MYSQL *conn,char *buf,int dir_id,int user_id);
int do_pwd(int tag,MYSQL *conn);
void delete_file(MYSQL *conn, MYSQL_RES *res);
int do_remove(int tag,MYSQL *conn);

//传输相关
void sigpipefunc(int);
int send_cycle(int newfd, void *pbuf, int len);
int recv_cycle(int newfd, void *pbuf, int len);

#endif