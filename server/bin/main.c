#include "../include/server.h"

int exitfd[2];

void sigfunc(int signum){
    write(exitfd[1], &signum, 1);
}

void sigpipefunc(int signum){
    signum = 0;
}

int main(int argc, char **argv){
    int ret;
    time_t now;
    ARGS_CHECK(argc, 2);

    //忽略SIGPIPE,防止因客户端断开引起进程崩溃
    signal(SIGPIPE, sigpipefunc);

    //读取配置文件
    config_t config[MAX_CONFIG];
    int config_num = read_config(argv[1], config);
    ERROR_CHECK(config_num, 0, "read_config");
    server_config config_message;
    bzero(&config_message, sizeof(config_message));
    ret = get_server_config(config, config_num, &config_message);
    ERROR_CHECK(ret, -1, "get_server_config");

    //启动日志系统
    int logfd = open(config_message.logpathstr, O_RDWR);
    int tmpoutfd = 100;
    dup2(STDOUT_FILENO, tmpoutfd);
    printf("\n"); //清空缓冲区
    dup2(logfd, STDOUT_FILENO);
    time(&now);
    printf("%s: ", ctime(&now));
    printf("start log\n");

    //连接mysql
    MYSQL *conn;
    ret = mysql_connect(&conn, config_message.mysqlpathstr);
    ERROR_CHECK(ret, -1, "mysql_connect");

    //初始化hashtable
    int max_clients = atoi(config_message.maxclientstr);
    int *hashtable = (int *)calloc(max_clients + 10, sizeof(int));

    //开启服务器端tcp监听
    int socketfd = tcp_init(config_message.ipstr, config_message.portstr);

    //配置epoll多路复用，监听socketfd、退出机制用管道读端和每一个子进程管道对端
    int epfd = epoll_create(1);
    struct epoll_event event, *evt;
    evt = (struct epoll_event *)calloc(2 + max_clients, sizeof(struct epoll_event));
    event.events = EPOLLIN;
    event.data.fd = socketfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, socketfd, &event);

    //退出机制
    pipe(exitfd);
    signal(SIGUSR1, sigfunc);
    event.data.fd = exitfd[0]; //若exitfd管道读端可读，代表进程要退出了
    epoll_ctl(epfd, EPOLL_CTL_ADD, exitfd[0], &event);

    //初始化线程池
    threadPool_t thdata1, thdata2;
    int threadnum1 = atoi(config_message.threadnum1str);
    int threadnum2 = atoi(config_message.threadnum2str);
    int quesize = atoi(config_message.quesizestr);
    
    thread_pool_init(&thdata1, threadnum1, quesize, hashtable, conn, epfd);
    thread_pool_init(&thdata2, threadnum2, quesize, hashtable, conn, epfd);

    //启动线程池
    thread_pool_start1(&thdata1);
    thread_pool_start2(&thdata2);

    int i, j;
    int readynum, tag, newfd;
    struct sockaddr_in client;
    datapackage_t data;
    task_t task;

    //主程序
    chdir("./files");
    pqueue_t pque1 = &thdata1.que;
    pqueue_t pque2 = &thdata2.que;
    while (1){
        readynum = epoll_wait(epfd, evt, 2 + max_clients, -1);
        for (i = 0; i < readynum; i++){
            tag = evt[i].data.fd;
            //接收到客户端连接请求
            if (tag == socketfd){
                bzero(&client, sizeof(client));
                socklen_t len = sizeof(client);
                newfd = accept(socketfd, (struct sockaddr *)&client, &len);
                ERROR_CHECK(newfd, -1, "accept");
                time(&now);
                printf("%s: ", ctime(&now));
                printf("connet with client ( %s:%d )\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                ret = recv_cycle(newfd, &data.datalen, sizeof(int)); //接受指令
                
                if (ret == -1){
                    time(&now);
                    printf("%s: ", ctime(&now));
                    printf("client close\n");
                    close(newfd);
                    continue;
                }
                
                if (data.datalen != -1 && data.datalen != -2){
                    time(&now);
                    printf("%s: ", ctime(&now));
                    printf("illegal instruction\n");
                    close(newfd);
                    continue;
                }
                else{ //线程池1负责注册登录
                    task.op = data.datalen;
                    task.fd = newfd;
                    pthread_mutex_lock(&pque1->queMutex);
                    in_queue(pque1, task); //将接受到的任务放入队列
                    pthread_mutex_unlock(&pque1->queMutex);
                    pthread_cond_signal(&thdata1.cond); //唤醒一个子进程
                    continue;
                }
            }
            //监听到已连接的client
            if (hashtable[tag] != 0){
                ret = recv_cycle(tag, &data.datalen, sizeof(int)); //接受指令
                
                if (ret == -1){
                    time(&now);
                    printf("%s: ", ctime(&now));
                    printf("client close\n");
                    hashtable[tag] = 0;
                    close(tag);
                    continue;
                }
                
                if (data.datalen == -3 || data.datalen == -4){ //线程池2负责上传下载
                    task.op = data.datalen;
                    task.fd = tag;
                    pthread_mutex_lock(&pque2->queMutex);
                    in_queue(pque2, task); //将接受到的任务放入队列
                    pthread_mutex_unlock(&pque2->queMutex);
                    pthread_cond_signal(&thdata2.cond); //唤醒一个子进程
                    continue;
                }
                else{ //main线程负责瞬间响应命令
                    switch (data.datalen){
                    case -5:
                        hashtable[tag] = 2;
                        ret = do_mkdir(tag, conn);
                        if (ret == -1){
                            hashtable[tag] = 0;
                            event.data.fd = tag;
                            epoll_ctl(epfd, EPOLL_CTL_DEL, tag, &event);
                            close(tag);
                        }
                        hashtable[tag] = 1;
                        break;
                    case -6:
                        hashtable[tag] = 2;
                        ret = do_cd(tag, conn);
                        if (ret == -1){
                            hashtable[tag] = 0;
                            event.data.fd = tag;
                            epoll_ctl(epfd, EPOLL_CTL_DEL, tag, &event);
                            close(tag);
                        }
                        hashtable[tag] = 1;
                        break;
                    case -7:
                        hashtable[tag] = 2;
                        ret = do_ls(tag, conn);
                        if (ret == -1){
                            hashtable[tag] = 0;
                            event.data.fd = tag;
                            epoll_ctl(epfd, EPOLL_CTL_DEL, tag, &event);
                            close(tag);
                        }
                        hashtable[tag] = 1;
                        break;
                    case -8:
                        hashtable[tag] = 2;
                        ret = do_pwd(tag, conn);
                        if (ret == -1){
                            hashtable[tag] = 0;
                            event.data.fd = tag;
                            epoll_ctl(epfd, EPOLL_CTL_DEL, tag, &event);
                            close(tag);
                        }
                        hashtable[tag] = 1;
                        break;
                    case -9:
                        hashtable[tag] = 2;
                        ret = do_remove(tag, conn);
                        if (ret == -1){
                            hashtable[tag] = 0;
                            event.data.fd = tag;
                            epoll_ctl(epfd, EPOLL_CTL_DEL, tag, &event);
                            close(tag);
                        }
                        hashtable[tag] = 1;
                        break;
                    case -10:
                        hashtable[tag] = 0;
                        event.data.fd = tag;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, tag, &event);
                        close(tag);
                        time(&now);
                        printf("%s: ", ctime(&now));
                        printf("disconnect with client\n");
                        break;
                    default:
                        time(&now);
                        printf("%s: ", ctime(&now));
                        printf("illegal instruction\n");
                        hashtable[tag] = 0;
                        event.data.fd = tag;
                        epoll_ctl(epfd, EPOLL_CTL_DEL, tag, &event);
                        close(tag);
                    }
                    continue;
                }
            }
            //如果接受到退出信号
            if (tag == exitfd[0]){
                //向任务队列中的客户发送一个紧急信息，并关闭连接
                while (!is_empty_queue(pque1)){
                    de_queue(pque1, &task);
                    int failnum = 0;
                    recv(task.fd, data.buf, sizeof(data.buf), 0);
                    send(task.fd, &failnum, sizeof(int), 0);
                    send(task.fd, "Emergency", 9, 0);
                    close(newfd);
                }

                while (!is_empty_queue(pque2)){
                    de_queue(pque2, &task);
                    int failnum = 0;
                    recv(task.fd, data.buf, sizeof(data.buf), 0);
                    send(task.fd, &failnum, sizeof(int), 0);
                    send(task.fd, "Emergency", 9, 0);
                    close(newfd);
                }
                //关闭mysql
                mysql_close(conn);
                //关闭打开的文件描述符
                event.data.fd = socketfd;
                epoll_ctl(epfd, EPOLL_CTL_DEL, socketfd, &event);
                close(epfd);
                close(socketfd);
                close(exitfd[0]);
                close(exitfd[1]);
                //cancel子线程
                for (j = 0; j < threadnum1; j++){
                    pthread_cancel(thdata1.pthid[j]);
                }
                long threadret;
                for (j = 0; j < threadnum1; j++){
                    pthread_join(thdata1.pthid[j], (void **)&threadret);
                    time(&now);
                    printf("%s: ", ctime(&now));
                    printf("%dth thread exited with code %ld\n", j + 1, threadret);
                }
                //清理队列堆空间
                free_queue(pque1);
                free_queue(pque2);
                free(thdata1.pthid);
                free(thdata2.pthid);
                free(hashtable);
                //关闭日志记录
                time(&now);
                printf("%s: ", ctime(&now));
                printf("close log\n");
                dup2(tmpoutfd, STDOUT_FILENO);
                exit(0); //main线程退出
            }
        }
    }
}
