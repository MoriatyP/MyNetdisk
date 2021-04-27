#include "../include/server.h"

//初始化线程池
int thread_pool_init(pthreadPool_t p, int threadnum, int quesize, int *hashtable, MYSQL *conn, int epfd){
    bzero(p, sizeof(threadPool_t));
    init_queue(&p->que, quesize);
    pthread_cond_init(&p->cond, NULL);
    p->pthid = (pthread_t *)calloc(threadnum, sizeof(pthread_t));
    p->threadnum = threadnum;
    p->hashtable = hashtable;
    p->conn = conn;
    p->epfd = epfd;
    return 0;
}

//启动线程池
int thread_pool_start1(pthreadPool_t p){
    int i;
    if (!p->startflag){
        for (i = 0; i < p->threadnum; i++){
            pthread_create(p->pthid + i, NULL, threadFunc1, p);
        }
        p->startflag = 1;
    }
    return 0;
}

int thread_pool_start2(pthreadPool_t p){
    int i;
    if (!p->startflag){
        for (i = 0; i < p->threadnum; i++){
            pthread_create(p->pthid + i, NULL, threadFunc2, p);
        }
        p->startflag = 1;
    }
    return 0;
}

//mkdir
int do_mkdir(int tag, MYSQL *conn){
    int ret;
    time_t now;
    datapackage_t data;
    int dir_id, user_id;
    int flag = 0;

    //接收当前文件夹id
    ret = recv_cycle(tag, &dir_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收用户id
    ret = recv_cycle(tag, &user_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收新文件夹名字
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(tag, &data.datalen, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    ret = recv_cycle(tag, data.buf, data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //在当前目录下查找是否有重名文件夹
    MYSQL_RES *res = mysql_select_dir(conn, "files", "pre_id", dir_id, "user_id", user_id);
    if (res){
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != NULL){
            if (strcmp(data.buf, row[3]) == 0){
                flag = 1;
                break;
            }
        }
    }
    if (flag){ //已经有该文件夹了
        bzero(data.buf, sizeof(data.buf));
        strcpy(data.buf, "you already have that folder");
        data.datalen = strlen(data.buf);
        ret = send_cycle(tag, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
    else{ //成功
        ret = mysql_insert_dir(conn, dir_id, user_id, data.buf);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("mysql_insert_dir error\n");
        }
        bzero(data.buf, sizeof(data.buf));
        strcpy(data.buf, "success\n");
        data.datalen = strlen(data.buf);
        send_cycle(tag, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
}

//cd
int do_cd(int tag, MYSQL *conn){
    int ret;
    time_t now;
    int user_id, cur_dir_id, cd_dir_id;
    int flag = 0;
    datapackage_t data;
    //接收用户id
    ret = recv_cycle(tag, &user_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收当前用户所在目录id
    ret = recv_cycle(tag, &cur_dir_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收cd后的路径
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(tag, &data.datalen, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    ret = recv_cycle(tag, data.buf, data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //查找cd后目录id并返回处理结果
    MYSQL_RES *res = NULL;
    if (strcmp(data.buf, "..") == 0){ //去上一级目录
        res = mysql_select_dir(conn, "files", "file_id", cur_dir_id, "user_id", user_id);
        MYSQL_ROW row = mysql_fetch_row(res);
        cd_dir_id = atoi(row[1]);
        bzero(data.buf, sizeof(data.buf));
        if (cd_dir_id == 0){ //已经在根目录了
            strcpy(data.buf, "it's already in home directory");
            data.datalen = strlen(data.buf);
            ret = send_cycle(tag, &data, 4 + data.datalen);
            if (ret == -1){
                time(&now);
                printf("%s: ",ctime(&now));
                printf("client close\n");
                return -1;
            }
            return 0;
        }
        //cd成功，发送成功，并发送新的当前目录id
        strcpy(data.buf, "success");
        data.datalen = strlen(data.buf);
        ret = send_cycle(tag, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        ret = send_cycle(tag, &cd_dir_id, sizeof(int));
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
    else if (data.buf[0] != '/'){ //双击打开一个目录
        res = mysql_select_dir(conn, "files", "pre_id", cur_dir_id, "user_id", user_id);
        if (res){
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != NULL){
                if (strcmp(data.buf, row[3]) == 0){
                    cd_dir_id = atoi(row[0]);
                    flag = 1;
                    break;
                }
            }
        }
        bzero(data.buf, sizeof(data.buf));
        if (flag == 0){ //不存在该目录
            strcpy(data.buf, "the directory does not exist");
            data.datalen = strlen(data.buf);
            ret = send_cycle(tag, &data, 4 + data.datalen);
            if (ret == -1){
                time(&now);
                printf("%s: ",ctime(&now));
                printf("client close\n");
                return -1;
            }
            return 0;
        }
        //cd成功
        strcpy(data.buf, "success");
        data.datalen = strlen(data.buf);
        ret = send_cycle(tag, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        ret = send_cycle(tag, &cd_dir_id, sizeof(int));
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
    else{ //cd一个绝对路径
        int i, j = 0;
        cd_dir_id = 0;
        char tmpbuf[DATA_BUF_LEN] = {0};
        for (i = 1; i <= data.datalen; i++){
            if (data.buf[i] == '/' || i == data.datalen){
                res = mysql_select_two(conn, "files", "filename", tmpbuf, "pre_id", cd_dir_id);
                if (res){
                    MYSQL_ROW row;
                    int change_flag = 0;
                    while ((row = mysql_fetch_row(res)) != NULL){
                        if (row[6] == NULL){
                            cd_dir_id = atoi(row[0]);
                            change_flag = 1;
                            break;
                        }
                    }
                    if (!change_flag){
                        flag = 1;
                    }
                }
                else{
                    flag = 1;
                }
                if (flag){
                    break;
                }
                j = 0;
                bzero(tmpbuf, sizeof(tmpbuf));
                continue;
            }
            tmpbuf[j++] = data.buf[i];
        }
        if (flag){ //不存在该目录
            strcpy(data.buf, "the directory does not exist");
            data.datalen = strlen(data.buf);
            ret = send_cycle(tag, &data, 4 + data.datalen);
            if (ret == -1){
                time(&now);
                printf("%s: ",ctime(&now));
                printf("client close\n");
                return -1;
            }
            return 0;
        }
        //cd成功
        strcpy(data.buf, "success");
        data.datalen = strlen(data.buf);
        ret = send_cycle(tag, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        ret = send_cycle(tag, &cd_dir_id, sizeof(int));
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
}

//ls
int do_ls(int tag, MYSQL *conn){
    int ret;
    time_t now;
    datapackage_t data;
    int dir_id, user_id;
    int cnt = 0;

    //接收当前文件夹id
    ret = recv_cycle(tag, &dir_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收用户id
    ret = recv_cycle(tag, &user_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //查询当前文件夹文件并放在一个包中
    bzero(data.buf, sizeof(data.buf));
    MYSQL_RES *res = mysql_select_int(conn, "files", "pre_id", dir_id);
    if (res){
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != NULL){
            strcat(data.buf, row[3]);
            strcat(data.buf, "\n");
            cnt++;
        }
    }
    //发送文件数量
    ret = send_cycle(tag, &cnt, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //发送文件名包
    data.datalen = strlen(data.buf);
    ret = send_cycle(tag, &data, 4 + data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    return 0;
}

void get_path_string(MYSQL *conn, char *buf, int dir_id, int user_id){
    MYSQL_RES *res = mysql_select_dir(conn, "files", "file_id", dir_id, "user_id", user_id);
    MYSQL_ROW row;
    row = mysql_fetch_row(res);
    if (strcmp(row[1], "0") == 0){
        strcat(buf, "/");
        strcat(buf, row[3]);
        return;
    }
    get_path_string(conn, buf, atoi(row[1]), user_id);
    strcat(buf, "/");
    strcat(buf, row[3]);
}

int do_pwd(int tag, MYSQL *conn){
    int ret;
    time_t now;
    datapackage_t data;
    int dir_id, user_id;

    //接收当前文件夹id
    ret = recv_cycle(tag, &dir_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收用户id
    ret = recv_cycle(tag, &user_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //递归查找绝对路径并拼接路径字符串
    bzero(data.buf, sizeof(data.buf));
    get_path_string(conn, data.buf, dir_id, user_id);
    //将路径发出
    data.datalen = strlen(data.buf);
    ret = send_cycle(tag, &data, 4 + data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    return 0;
}

void delete_file(MYSQL *conn, MYSQL_RES *res){
    int ret;
    time_t now;
    MYSQL_ROW row;
    row = mysql_fetch_row(res);
    int file_id = atoi(row[0]);
    int nextid;
    MYSQL_RES *nextres;
    if (strcmp(row[5], "0") == 0){//如果是文件夹，递归删除里面的内容
        res = mysql_select_int(conn, "files", "pre_id", file_id);
        if (res){
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != NULL){
                nextid = atoi(row[0]);
                nextres = mysql_select_int(conn, "files", "file_id", nextid);
                delete_file(conn, nextres);
            }
        }
        ret = mysql_delete_file(conn, file_id);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("mysql_insert_dir error\n");
        }
    }
    else{//如果该文件不是文件夹，删除虚拟文件系统中的文件，并查看是否需要删除服务器源端的真实文件
        ret = mysql_delete_file(conn, file_id);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("mysql_insert_dir error\n");
        }
        char *real_filename = row[4];
        res = mysql_select(conn, "files", "real_filename", real_filename);
        if (!res){//如果虚拟文件系统中所有与该真实文件的关联记录均已删除，则删除源真实文件
            ret = unlink(real_filename);
            if (ret == -1){
                perror("unlink");
            }
        }
    }
}

int do_remove(int tag, MYSQL *conn){
    int ret;
    time_t now;
    datapackage_t data;
    int dir_id, user_id;

    //接收当前文件夹id
    ret = recv_cycle(tag, &dir_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收用户id
    ret = recv_cycle(tag, &user_id, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收文件名
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(tag, &data.datalen, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    ret = recv_cycle(tag, data.buf, data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ",ctime(&now));
        printf("client close\n");
        return -1;
    }
    //查找文件并删除,返回处理结果
    MYSQL_RES *res = mysql_select_two(conn, "files", "filename", data.buf, "pre_id", dir_id);
    bzero(data.buf, sizeof(data.buf));
    if (!res){//如果没找到该文件，返回文件不存在
        strcpy(data.buf, "file not found");
        data.datalen = strlen(data.buf);
        ret = send_cycle(tag, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
    else{
        delete_file(conn, res);
        strcpy(data.buf, "success");
        data.datalen = strlen(data.buf);
        ret = send_cycle(tag, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ",ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
}

//线程清理函数
void cleanFunc(void *p){
    pthread_mutex_unlock((pthread_mutex_t *)p);
}
//子线程主程序（注册和登录）
void *threadFunc1(void *p){
    int ret;
    time_t now;
    pthreadPool_t pthdata = (pthreadPool_t)p;
    pqueue_t pque = &pthdata->que;
    task_t task;
    while (1){
        pthread_mutex_lock(&pque->queMutex);
        pthread_cleanup_push(cleanFunc, &pque->queMutex);
        if (is_empty_queue(pque)){ //若任务队列为空，则等待
            pthread_cond_wait(&pthdata->cond, &pque->queMutex);
        }
        de_queue(pque, &task); //拿任务
        pthread_cleanup_pop(1);
        if (task.op == -1){
            ret = do_register(task.fd, pthdata->conn);
            if (!ret){//如果注册成功，由线程关闭连接
                close(task.fd);
            }
        }else{
            do_login(task.fd, pthdata->conn, pthdata->hashtable, pthdata->epfd);
        }
        time(&now);
        printf("%s: ",ctime(&now));
        printf("From %ld: Mission R&L Completed\n", syscall(SYS_gettid));
    }
}

//子线程主程序（上传和下载）
void *threadFunc2(void *p){
    int ret;
    time_t now;
    pthreadPool_t pthdata = (pthreadPool_t)p;
    pqueue_t pque = &pthdata->que;
    task_t task;
    struct epoll_event event;
    while (1){
        pthread_mutex_lock(&pque->queMutex);
        pthread_cleanup_push(cleanFunc, &pque->queMutex);
        if (is_empty_queue(pque)){ //若任务队列为空，则等待
            pthread_cond_wait(&pthdata->cond, &pque->queMutex);
        }
        de_queue(pque, &task); //拿任务
        pthread_cleanup_pop(1);
        if (task.op == -3){
            pthdata->hashtable[task.fd]=2;
            event.events=EPOLLIN;
            event.data.fd = task.fd;
            epoll_ctl(pthdata->epfd, EPOLL_CTL_DEL, task.fd, &event);//屏蔽main线程的epoll监控
            ret = do_gets(task.fd, pthdata->conn);
            if (ret == -1){//如果传输异常，由线程断开连接
                pthdata->hashtable[task.fd]=0;
                epoll_ctl(pthdata->epfd, EPOLL_CTL_DEL, task.fd, &event);
                close(task.fd);
            }
            pthdata->hashtable[task.fd]=1;
            epoll_ctl(pthdata->epfd, EPOLL_CTL_ADD, task.fd, &event);//恢复main线程的epoll监控
        }else{
            pthdata->hashtable[task.fd]=2;
            event.events=EPOLLIN;
            event.data.fd = task.fd;
            epoll_ctl(pthdata->epfd, EPOLL_CTL_DEL, task.fd, &event);//屏蔽main线程的epoll监控
            ret = do_puts(task.fd, pthdata->conn);
            if (ret == -1){//如果传输异常，由线程断开连接
                pthdata->hashtable[task.fd]=0;
                event.data.fd = task.fd;
                epoll_ctl(pthdata->epfd, EPOLL_CTL_DEL, task.fd, &event);
                close(task.fd);
            }
            pthdata->hashtable[task.fd]=1;
            epoll_ctl(pthdata->epfd, EPOLL_CTL_ADD, task.fd, &event);//恢复main线程的epoll监控
        }
        time(&now);
        printf("%s: ",ctime(&now));
        printf("From %ld: Mission U&D Completed\n", syscall(SYS_gettid));
    }
}

//开启tcp监听
int tcp_init(char *ip, char *port){
    int ret;
    time_t now;
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(socketfd, -1, "socket");
    struct sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(atoi(port));
    server.sin_family = AF_INET;
    int reuse = 1;
    ret = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    ERROR_CHECK(ret, -1, "setsockopt");
    ret = bind(socketfd, (struct sockaddr *)&server, sizeof(struct sockaddr));
    ERROR_CHECK(ret, -1, "bind");
    listen(socketfd, 10);
    return socketfd;
}
