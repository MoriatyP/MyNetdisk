#include "../include/server.h"

//生成盐值
char *generate_salt(){
    static char str[SALT_LEN + 1] = {0};
    int i, flag;
    srand(time(NULL)); //通过时间函数设置随机数种子，使得每次运行结果随机。
    for (i = 0; i < SALT_LEN; i++){
        flag = rand() % 3;
        switch (flag){
        case 0:
            str[i] = rand() % 26 + 'a';
            break;
        case 1:
            str[i] = rand() % 26 + 'A';
            break;
        case 2:
            str[i] = rand() % 10 + '0';
            break;
        }
    }
    return str;
}

int do_register(int taskfd, MYSQL *conn){
    int ret;
    time_t now;
    datapackage_t data;
    char user_name[DATA_BUF_LEN];
    char password[DATA_BUF_LEN];
    int reg_times = 0;
begin:
    bzero(user_name, sizeof(user_name));
    bzero(password, sizeof(password));
    //接收用户名
    reg_times++;
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(taskfd, &data.datalen, sizeof(int));
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }
    
    ret = recv_cycle(taskfd, data.buf, data.datalen);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }
    
    strcpy(user_name, data.buf);

    //验证用户名是否重复
    MYSQL_RES *res = mysql_select(conn, "user", "user_name", user_name);
    
    if (res){
        bzero(data.buf, sizeof(data.buf));
        if (reg_times == 3){
            strcpy(data.buf, "please register later");
            data.datalen = strlen(data.buf);
            ret = send_cycle(taskfd, &data, 4 + data.datalen);
            if (ret == -1){
                time(&now);
                printf("%s: ", ctime(&now));
                printf("client close\n");
                close(taskfd);
                return -1;
            }
            return 0;
        }
        strcpy(data.buf, "the user name is already in use");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            close(taskfd);
            return -1;
        }
        goto begin;
    }
    bzero(data.buf, sizeof(data.buf));
    strcpy(data.buf, "valid user name");
    data.datalen = strlen(data.buf);
    ret = send_cycle(taskfd, &data, 4 + data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }

    //接收密码
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(taskfd, &data.datalen, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }
    ret = recv_cycle(taskfd, data.buf, data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }
    strcpy(password, data.buf);

    //保存密码密文到数据库
    char *salt = generate_salt();
    ret = mysql_insert_user(conn, user_name, crypt(password, salt), salt);
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql_insert_user error\n");
        bzero(data.buf, sizeof(data.buf));
        strcpy(data.buf, "server error");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        return -1;
    }

    //为用户分配个人文件夹
    res = mysql_select(conn, "user", "user_name", user_name);
    MYSQL_ROW row;
    row = mysql_fetch_row(res);
    int user_id = atoi(row[0]);
    ret = mysql_insert_dir(conn, 0, user_id, user_name);
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("mysql_insert_dir error\n");
        bzero(data.buf, sizeof(data.buf));
        strcpy(data.buf, "server error");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        return -1;
    }

    //注册成功
    bzero(data.buf, sizeof(data.buf));
    strcpy(data.buf, "registered successfully");
    data.datalen = strlen(data.buf);
    ret = send_cycle(taskfd, &data, 4 + data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }
    return 0;
}

int do_login(int taskfd, MYSQL *conn, int *hashtable, int epfd){
    int ret;
    time_t now;
    datapackage_t data;
    char user_name[DATA_BUF_LEN];
    char password[DATA_BUF_LEN];
    int login_user_times = 0;
    int login_passwd_times = 0;

username:
    bzero(user_name, sizeof(user_name));
    //接收用户名
    login_user_times++;
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(taskfd, &data.datalen, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }
    ret = recv_cycle(taskfd, data.buf, data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }
    strcpy(user_name, data.buf);

    //验证用户名
    MYSQL_RES *res = mysql_select(conn, "user", "user_name", user_name);
    if (!res){
        bzero(data.buf, sizeof(data.buf));
        if (login_user_times == 5){
            strcpy(data.buf, "please login later");
            data.datalen = strlen(data.buf);
            ret = send_cycle(taskfd, &data, 4 + data.datalen);
            if (ret == -1){
                time(&now);
                printf("%s: ", ctime(&now));
                printf("client close\n");
                close(taskfd);
                return -1;
            }
            close(taskfd);
            return 0;
        }
        strcpy(data.buf, "invalid user name");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            close(taskfd);
            return -1;
        }
        goto username;
    }
    bzero(data.buf, sizeof(data.buf));
    strcpy(data.buf, "valid user name");
    data.datalen = strlen(data.buf);
    ret = send_cycle(taskfd, &data, 4 + data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }

    //取出用户信息
    MYSQL_ROW row;
    row = mysql_fetch_row(res);
    int user_id = atoi(row[0]);

    //取出盐值并发送
    bzero(data.buf, sizeof(data.buf));
    strcpy(data.buf, row[3]);
    data.datalen = strlen(data.buf);
    ret = send_cycle(taskfd, &data, 4 + data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }

passwd:
    bzero(password, sizeof(password));
    //接收密码
    login_passwd_times++;
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(taskfd, &data.datalen, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }
    ret = recv_cycle(taskfd, data.buf, data.datalen);
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        close(taskfd);
        return -1;
    }
    strcpy(password, data.buf);

    //验证密码
    if (strcmp(password, row[2]) == 0){
        //登录成功
        bzero(data.buf, sizeof(data.buf));
        strcpy(data.buf, "login successful");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            close(taskfd);
            return -1;
        }

        //告知用户home目录id
        int home_id;
        res = mysql_select_dir(conn, "files", "pre_id", 0, "user_id", user_id);
        row = mysql_fetch_row(res);
        home_id = atoi(row[0]);
        ret = send_cycle(taskfd, &home_id, sizeof(int));
        
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            close(taskfd);
            return -1;
        }

        //告知用户id
        ret = send_cycle(taskfd, &user_id, sizeof(int));
        
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            close(taskfd);
            return -1;
        }
        //设置hashtable为已连接状态
        hashtable[taskfd] = 1;
        //加入epoll监听
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = taskfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, taskfd, &event);
        return 0;
    }
    else{
        bzero(data.buf, sizeof(data.buf));
        if (login_passwd_times == 5){
            strcpy(data.buf, "Please log in later");
            data.datalen = strlen(data.buf);
            ret = send_cycle(taskfd, &data, 4 + data.datalen);
            
            if (ret == -1){
                time(&now);
                printf("%s: ", ctime(&now));
                printf("client close\n");
                close(taskfd);
                return -1;
            }
            
            close(taskfd);
            return 0;
        }
        
        strcpy(data.buf, "incorrect password");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            close(taskfd);
            return -1;
        }
        goto passwd;
    }
}
