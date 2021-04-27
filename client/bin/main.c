#include "../include/client.h"

void sigpipefunc(int signum){
    signum = 0;
}

int main(int argc, char *argv[]){
    ARGS_CHECK(argc, 3);

    //忽略SIGPIPE,防止因客户端断开引起进程崩溃
    signal(SIGPIPE, sigpipefunc);

    int ret;
    int socketfd;
    char a;
    char c[DATA_BUF_LEN] = {0};
   // char c;
    char cmd[DATA_BUF_LEN];
    int home_id, user_id, cur_dir_id;
    int endnumber = -10;
    chdir("files");
start :
    start_window();
//    c = getchar();
    scanf("%s", c);
    if(strlen(c) != 1){
        printf("wrong command\n");                                                                                                                     
        fflush(stdin);
        system_pause();
        goto start;
    }
    a = c[0];
    switch (a){
    case '0':
        goto exit;
        break;
    case '1':
        goto sign_up;
        break;
    case '2':
        goto login;
        break;
    default:
        printf("wrong command\n");
        fflush(stdin);
        system_pause();
        goto start;
    }
sign_up:
    ret = connect_with_server(&socketfd, argv[1], argv[2]);
    if(ret == -1){
        goto exit;
    }
    do_register(socketfd);
    close(socketfd);
    system_pause();
    goto start;
login:
    ret = connect_with_server(&socketfd, argv[1], argv[2]);
    if(ret == -1){
        goto exit;
    }
    ret = do_login(socketfd, &home_id, &user_id);
    if(ret == -1){
        close(socketfd);
        system_pause();
        goto start;
    }
    cur_dir_id = home_id;
    system_pause();
    goto mainpage;
mainpage:
    mainpage_window();
    bzero(cmd, sizeof(cmd));
    scanf("%s", cmd);
    getchar();
    if(strcmp(cmd, "gets")==0){
        ret = do_gets(socketfd, cur_dir_id, user_id);
        system_pause();
        if(ret == -1){
            goto end;
        }
    }
    else if(strcmp(cmd, "puts") == 0){
        ret = do_puts(socketfd, cur_dir_id, user_id);
        system_pause();
        if(ret == -1){
            goto end;
        }
    }
    else if(strcmp(cmd, "mkdir") == 0){
        ret = do_mkdir(socketfd, cur_dir_id, user_id);
        system_pause();
        if(ret == -1){
            goto end;
        }
    }
    else if(strcmp(cmd, "cd") == 0){
        ret = do_cd(socketfd, &cur_dir_id, user_id);
        system_pause();
        if(ret == -1){
            goto end;
        }
    }
    else if(strcmp(cmd, "ls") == 0){
        ret = do_ls(socketfd, cur_dir_id, user_id);
        system_pause();
        if(ret == -1){
            goto end;
        }
    }
    else if(strcmp(cmd, "lls") == 0){
        do_lls();
        system_pause();
    }
    else if(strcmp(cmd, "pwd") == 0){
        ret = do_pwd(socketfd, cur_dir_id, user_id);
        system_pause();
        if(ret == -1){
            goto end;
        }
    }
    else if(strcmp(cmd, "rm") == 0){
        ret = do_remove(socketfd, cur_dir_id, user_id);
        system_pause();
        if(ret == -1){
            goto end;
        }
    }
    else if(strcmp(cmd, "q") == 0){
        goto end;
    }
    else{
        printf("wrong command\n");
        system_pause();
    }
    goto mainpage;
end:
    send_cycle(socketfd, &endnumber, sizeof(int));
    close(socketfd);
    goto start;
exit:
    printf("Connect over\n");
    return 0;
}
