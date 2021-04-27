#include "../include/client.h"

int connect_with_server(int *socketfd, char *ip, char *port){
    int ret;
    //初始化缓冲区
    *socketfd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(*socketfd, -1, "socket");
    //配置连接
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(atoi(port));
    ret = connect(*socketfd, (struct sockaddr *)&server, sizeof(server));
    ERROR_CHECK(ret, -1, "connect");
    return 0;
}

void system_pause(){ //实现windows下system("pause")的效果
    printf("Press any key to continue...\n");
    system("stty raw");
    getchar();
    system("stty cooked");
}

void start_window(){
    system("clear");
    printf(":***************************************************************:\n");
    printf(":*********************  Welcome   Netdisk  *********************:\n");
    printf(":***************************************************************:\n");
    printf(":                                                               :\n");
    printf(":                    1.register  an  account                    :\n");
    printf(":                    2.login   your  account                    :\n");
    printf(":                                                               :\n");
    printf(":                    0.exit                                     :\n");
    printf(":                                                               :\n");
    printf(":***************************************************************:\n");
    printf("Please input commend number:");
    fflush(stdout);
}

void mainpage_window(){
    system("clear");
    printf(":***************************************************************:\n");
    printf(":*******************  Your Personal Netdisk  *******************:\n");
    printf(":***************************************************************:\n");
    printf(":                                                               :\n");
    printf(":     Usage:                                                    :\n");
    printf(":           download file:             gets                     :\n");
    printf(":           upload file:               puts                     :\n");
    printf(":           make directory:            mkdir                    :\n");
    printf(":           change directory:          cd                       :\n");
    printf(":           list file:                 ls                       :\n");
    printf(":           list local file:           lls                      :\n");
    printf(":           print working directory:   pwd                      :\n");
    printf(":           remove file:               rm                       :\n");
    printf(":           quit                       q                        :\n");
    printf(":                                                               :\n");
    printf(":                                                               :\n");
    printf(":***************************************************************:\n");
    printf("Please input commend:");
    fflush(stdout);
}

int do_register(int socketfd){
    system("clear");
    int ret;
    datapackage_t data;
    data.datalen = -1;
    char *password;

    //发送指令
    send(socketfd, &data.datalen, sizeof(int), 0);

begin:
    //输入用户名
    printf("please input your username:");
    fflush(stdout);
    bzero(data.buf, sizeof(data.buf));
    scanf("%s", data.buf);
    data.datalen = strlen(data.buf);
    send(socketfd, &data, 4 + data.datalen, 0);
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    
    printf("%s\n", data.buf);
    
    if (strcmp(data.buf, "the user name is already in use") == 0){
        sleep(1);
        system("clear");
        goto begin;
    }
    
    if (strcmp(data.buf, "please register later") == 0){
        return 0;
    }

    //输入密码
    bzero(data.buf, sizeof(data.buf));
    password = getpass("please input your password:");
    getchar();
    strcpy(data.buf, password);
    data.datalen = strlen(data.buf);
    send(socketfd, &data, 4 + data.datalen, 0);

    //注册成功
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    printf("%s\n", data.buf);
    if (strcmp(data.buf, "registered successfully") == 0){
        return 0;
    }
    else{
        return -1;
    }
}

int do_login(int socketfd, int *home_id, int *user_id){
    system("clear");
    int ret;
    datapackage_t data;
    data.datalen = -2;
    int getcharflag = 0;

    send(socketfd, &data.datalen, sizeof(int), 0);

username:
    //输入用户名
    printf("please input your username:");
    fflush(stdout);
    bzero(data.buf, sizeof(data.buf));
    scanf("%s", data.buf);
    data.datalen = strlen(data.buf);
    send(socketfd, &data, 4 + data.datalen, 0);

    //验证用户名
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    
    printf("%s\n", data.buf);
    
    if (strcmp(data.buf, "invalid user name") == 0){
        sleep(1);
        system("clear");
        goto username;
    }
    
    if (strcmp(data.buf, "please login later") == 0){
        return -1;
    }

    //接收盐值
    char salt[DATA_BUF_LEN] = {0};
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    
    strcpy(salt, data.buf);

    char *password;

passwd:
    //输入密码
    password = getpass("please input your password:");
    
    if (getcharflag == 0){
        getchar();
    }
    
    bzero(data.buf, sizeof(data.buf));
    strcpy(data.buf, crypt(password, salt));
    data.datalen = strlen(data.buf);
    send(socketfd, &data, 4 + data.datalen, 0);

    //验证密码
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    
    printf("%s\n", data.buf);
    
    if (strcmp(data.buf, "login successful") == 0){
        //接收home目录id
        ret = recv_cycle(socketfd, home_id, sizeof(int));
        
        if (ret == -1){
            printf("server closed\n");
            return -1;
        }
        //接收用户id
        
        ret = recv_cycle(socketfd, user_id, sizeof(int));
        
        if (ret == -1){
            printf("server closed\n");
            return -1;
        }
        
        return 0;
    }
    
    if (strcmp(data.buf, "incorrect password") == 0){
        getcharflag = 1;
        goto passwd;
    }
    else{
        return -1;
    }
}

int do_gets(int socketfd, int dir_id, int user_id){
    int ret;
    datapackage_t data;
    int fd, logfd;
    int repnum = 1;
    char filename[DATA_BUF_LEN] = {0};
    char namebuf[DATA_BUF_LEN] = {0};
    char tempfilename[DATA_BUF_LEN] = {0};
    char logname[DATA_BUF_LEN] = {0};
    off_t filesize, fileslice, lastloadsize = 0, downloadsize = 0, offset = 0;
    int num = 0;
    double now = 0.0;
    char symbuf[52];
    memset(symbuf, '.', sizeof(symbuf));
    char sym[4] = {'-', '\\', '|', '/'};

    //输入文件名，查看本地是否有重名文件
    printf("Please input name of file you want to download:");
    fflush(stdout);
    bzero(data.buf, sizeof(data.buf));
    scanf("%s", data.buf);
    getchar();
    fd = open(data.buf, O_RDWR);
    if (fd != -1)
    {
        close(fd);
        printf("you already have the file with the same name\n");
        return 0;
    }
    //发送指令
    data.datalen = -3;
    ret = send_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    //发送当前文件夹id
    ret = send_cycle(socketfd, &dir_id, sizeof(int));
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    //发送用户id
    ret = send_cycle(socketfd, &user_id, sizeof(int));
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    //发送文件名，申请下载
    data.datalen = strlen(data.buf);
    ret = send_cycle(socketfd, &data, 4 + data.datalen);
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    strcpy(filename, data.buf);
    strcpy(namebuf, data.buf);
    strcpy(tempfilename, data.buf);
    strcat(tempfilename, ".temp");
    //接收申请结果
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1)
    {
        printf("Server Closed\n");
        return -1;
    }
    ret = recv_cycle(socketfd, &data.buf, data.datalen);
    if (ret == -1)
    {
        printf("Server Closed\n");
        return -1;
    }
    printf("%s\n", data.buf);
    //如果没有该文件，则退出
    if (strcmp(data.buf, "the file does not exist") == 0)
    {
        return 0;
    }
    //打开文件,发送偏移量,准备接收
    if ((fd = open(tempfilename, O_RDWR | O_CREAT | O_EXCL, 0666)) == -1)
    { //如果已经存在了临时文件,则打开临时文件进行断点续传
        fd = open(tempfilename, O_RDWR);
        strcpy(logname, filename);
        strcat(logname, ".templog");
        logfd = open(logname, O_RDWR);
        read(logfd, &offset, sizeof(offset));
    }
    else
    { //不存在临时文件则新建文件
        close(fd);
        remove(tempfilename);
        while ((fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0666)) == -1)
        {
            bzero(filename, sizeof(filename));
            sprintf(filename, "%s(%d)", namebuf, repnum++);
        }
        bzero(tempfilename, sizeof(tempfilename));
        strcpy(tempfilename, filename);
        strcat(tempfilename, ".temp");
        rename(filename, tempfilename);
        strcpy(logname, filename);
        strcat(logname, ".templog");
        logfd = open(logname, O_RDWR | O_CREAT | O_EXCL, 0666);
    }
    data.datalen = sizeof(offset);
    bzero(data.buf, sizeof(data.buf));
    memcpy(data.buf, &offset, data.datalen);
    ret = send_cycle(socketfd, &data, 4 + data.datalen);
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    //接文件大小
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1)
    {
        printf("Server Closed\n");
        return -1;
    }
    ret = recv_cycle(socketfd, &filesize, data.datalen);
    if (ret == -1)
    {
        printf("Server Closed\n");
        return -1;
    }
    fileslice = filesize / 10000;
    //接文件内容
    ftruncate(fd, filesize);
    char *pstart = (char *)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ERROR_CHECK(pstart, (char *)-1, "mmap");
    if (offset) //如果是断点续传，先加上原有的下载量
    {
        downloadsize += offset;
    }
    while (downloadsize < filesize)
    {
        ret = recv(socketfd, pstart + downloadsize, filesize - downloadsize, 0);
        if (ret == 0)
        {
            printf("Server Closed\n");
            return -1;
        }
        downloadsize += ret;
        //每传输一次，记录接收到的字符数
        write(logfd, &downloadsize, sizeof(downloadsize));
        lseek(logfd, SEEK_SET, 0);
        //打印进度条
        if (downloadsize - lastloadsize >= fileslice)
        {
            printf("[\033[5;32mloading\033[0m \033[0;31m%-50s\033[0m %5.2lf%% \033[0;33m%c\033[0m ]\r", symbuf, now, sym[num % 4]);
            fflush(stdout);
            now = (double)downloadsize / filesize * 100;
            lastloadsize = downloadsize;
            num = (int)now / 2;
            memset(symbuf, '#', num);
        }
    }
    munmap(pstart, filesize);
    close(fd);
    close(logfd);
    num = 52;
    memset(symbuf, '#', num);
    printf("[\033[1;32mloading\033[0m \033[0;31m%-50s\033[0m 100.00%% \033[1;33m%c\033[0m ]\n", symbuf, sym[num % 4]);
    rename(tempfilename, filename); //将下载完成的临时文件命名为正式文件名
    remove(logname);                //删除log文件
    return 0;
}

int do_puts(int socketfd, int dir_id, int user_id)
{
    int ret;
    datapackage_t data;
    int fd;
    char filename[DATA_BUF_LEN] = {0};
    off_t filesize, fileslice, lastloadsize = 0, uploadsize = 0;
    int gap;
    int num = 0;
    double now = 0.0;
    char symbuf[52];
    memset(symbuf, '.', sizeof(symbuf));
    char sym[4] = {'-', '\\', '|', '/'};

    //输入文件名，查看本地是否有该文件，有则打开
    printf("Please input name of file you want to upload:");
    fflush(stdout);
    bzero(data.buf, sizeof(data.buf));
    scanf("%s", data.buf);
    getchar();
    fd = open(data.buf, O_RDWR);
    if (fd == -1)
    {
        printf("the file does not exist\n");
        return 0;
    }
    strcpy(filename, data.buf);
    //发送指令
    data.datalen = -4;
    ret = send_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    //发送当前文件夹id
    ret = send_cycle(socketfd, &dir_id, sizeof(int));
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    //发送用户id
    ret = send_cycle(socketfd, &user_id, sizeof(int));
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    //发送文件名
    data.datalen = strlen(data.buf);
    ret = send_cycle(socketfd, &data, 4 + data.datalen);
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    //接收是否重名提醒
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1)
    {
        printf("Server Closed\n");
        return -1;
    }
    ret = recv_cycle(socketfd, &data.buf, data.datalen);
    if (ret == -1)
    {
        printf("Server Closed\n");
        return -1;
    }
    printf("%s\n", data.buf);
    if (strcmp(data.buf, "you already have the file with the same name") == 0) //如果重名直接退出
    {
        close(fd);
        return 0;
    }
    //发送文件大小
    struct stat statbuf;
    fstat(fd, &statbuf);
    data.datalen = sizeof(statbuf.st_size);
    memcpy(data.buf, &statbuf.st_size, data.datalen);
    ret = send_cycle(socketfd, &data, 4 + data.datalen);
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    filesize=statbuf.st_size;
    fileslice = filesize / 10000;
    //发送MD5码
    bzero(data.buf, sizeof(data.buf));
    Compute_file_md5(filename, data.buf);
    data.datalen = strlen(data.buf);
    ret = send_cycle(socketfd, &data, 4 + data.datalen);
    if (ret == -1)
    {
        printf("server close\n");
        return -1;
    }
    //接收是否秒传
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1)
    {
        printf("Server Closed\n");
        return -1;
    }
    ret = recv_cycle(socketfd, &data.buf, data.datalen);
    if (ret == -1)
    {
        printf("Server Closed\n");
        return -1;
    }
    if (strcmp(data.buf, "quick upload") == 0) //如果秒传直接退出
    {
        close(fd);
        num = 52;
        memset(symbuf, '#', num);
        printf("[\033[1;32mloading\033[0m \033[0;31m%-50s\033[0m 100.00%% \033[1;33m%c\033[0m ]\n", symbuf, sym[num % 4]);
        printf("%s\n", data.buf);
        return 0;
    }
    //发送文件内容 
    char *pstart = (char *)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ERROR_CHECK(pstart, (char *)-1, "mmap");
    while (uploadsize < filesize){
        if(filesize - uploadsize > DATA_BUF_LEN){
            gap = DATA_BUF_LEN;
        }
        else{
            gap = filesize-uploadsize;
        } 
        ret = send(socketfd, pstart + uploadsize, gap, 0);
        if (ret == -1){
            printf("Server Closed\n");
            return -1;
        }
        uploadsize += gap;
        //打印进度条
        if (uploadsize - lastloadsize >= fileslice){
            printf("[\033[5;32mloading\033[0m \033[0;31m%-50s\033[0m %5.2lf%% \033[0;33m%c\033[0m ]\r", symbuf, now, sym[num % 4]);
            fflush(stdout);
            now = (double)uploadsize / filesize * 100;
            lastloadsize = uploadsize;
            num = (int)now / 2;
            memset(symbuf, '#', num);
        }
    }
    munmap(pstart, filesize);
    close(fd);
    num = 52;
    memset(symbuf, '#', num);
    printf("[\033[1;32mloading\033[0m \033[0;31m%-50s\033[0m 100.00%% \033[1;33m%c\033[0m ]\n", symbuf, sym[num % 4]);
    return 0;
}

int do_mkdir(int socketfd, int dir_id, int user_id){
    datapackage_t data;
    int ret;
    //发送指令
    data.datalen = -5;
    ret = send_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送当前文件夹id
    ret = send_cycle(socketfd, &dir_id, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送用户id
    ret = send_cycle(socketfd, &user_id, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送文件夹名字
    printf("please input name of directory:");
    fflush(stdout);
    bzero(data.buf, sizeof(data.buf));
    scanf("%s", data.buf);
    getchar();
    data.datalen = strlen(data.buf);
    ret = send_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    ret = send_cycle(socketfd, data.buf, data.datalen);
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //接收新建结果
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    printf("%s\n", data.buf);
    return 0;
}

int do_cd(int socketfd, int *dir_id, int user_id){
    datapackage_t data;
    int ret;
    //发送指令
    data.datalen = -6;
    ret = send_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送用户id
    ret = send_cycle(socketfd, &user_id, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送当前文件夹id
    ret = send_cycle(socketfd, dir_id, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送路径
    printf("you can input:\n");
    printf("1.\"..\"(means parent Directory)\n");
    printf("2.<directory name>\n");
    printf("3.<absolute path>\n");
    printf("please input path:");
    fflush(stdout);
    bzero(data.buf, sizeof(data.buf));
    scanf("%s", data.buf);
    getchar();
    data.datalen = strlen(data.buf);
    ret = send_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    ret = send_cycle(socketfd, data.buf, data.datalen);
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //接收cd结果
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    printf("%s\n", data.buf);
    if (strcmp(data.buf, "success") == 0){ //成功则接收新的当前目录id
        ret = recv_cycle(socketfd, dir_id, sizeof(int));
        if (ret == -1){
            printf("server close\n");
            return -1;
        }
    }
    return 0;
}

int do_ls(int socketfd, int dir_id, int user_id){
    datapackage_t data;
    int ret;
    //发送指令
    data.datalen = -7;
    ret = send_cycle(socketfd, &data.datalen, sizeof(int));
    
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送当前文件夹id
    ret = send_cycle(socketfd, &dir_id, sizeof(int));
    
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送用户id
    ret = send_cycle(socketfd, &user_id, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //接收文件数量，并打印
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    
    printf("[ totle: %d ]\n", data.datalen);
    //接收文件名，并打印
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    printf("%s", data.buf);
    return 0;
}

void do_lls(){
    char localdir[DATA_BUF_LEN] = {0};
    char ans[DATA_BUF_LEN] = {0};
    getcwd(localdir, sizeof(localdir));
    DIR *pdir = opendir(localdir);
    struct dirent *pbuf;
    int totle = 0;
    while ((pbuf = readdir(pdir)) != NULL){
        if (strcmp(pbuf->d_name, ".") && strcmp(pbuf->d_name, "..")){
            strcat(ans, pbuf->d_name);
            strcat(ans, "\n");
            totle++;
        }
    }
    printf("[ totle: %d ]\n%s", totle, ans);
}

int do_pwd(int socketfd, int dir_id, int user_id){
    datapackage_t data;
    int ret;
    //发送指令
    data.datalen = -8;
    ret = send_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送当前文件夹id
    ret = send_cycle(socketfd, &dir_id, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送用户id
    ret = send_cycle(socketfd, &user_id, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //接收路径并打印
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    printf("%s\n", data.buf);
    return 0;
}

int do_remove(int socketfd, int dir_id, int user_id){
    datapackage_t data;
    int ret;
    //发送指令
    data.datalen = -9;
    ret = send_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送当前文件夹id
    ret = send_cycle(socketfd, &dir_id, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //发送用户id
    ret = send_cycle(socketfd, &user_id, sizeof(int));
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //输入并发送要删除的文件的名称
    printf("please enter the file name to delete:");
    fflush(stdout);
    bzero(data.buf, sizeof(data.buf));
    scanf("%s", data.buf);
    getchar();
    data.datalen = strlen(data.buf);
    ret = send_cycle(socketfd, &data, 4 + data.datalen);
    if (ret == -1){
        printf("server close\n");
        return -1;
    }
    //接收处理结果并打印
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(socketfd, &data.datalen, sizeof(int));
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    ret = recv_cycle(socketfd, data.buf, data.datalen);
    if (ret == -1){
        printf("server closed\n");
        return -1;
    }
    printf("%s\n", data.buf);
    return 0;
}

//按规定字节数发送信息
int send_cycle(int newfd, void *pbuf, int len){
    int sendtotle = 0;
    int sendnum;
    while (sendtotle < len){
        sendnum = send(newfd, (char *)pbuf + sendtotle, len - sendtotle, 0);
        if (sendnum == -1){
            return -1;
        }
        sendtotle += sendnum;
    }
    return 0;
}

//按规定字节数接收信息
int recv_cycle(int newfd, void *pbuf, int len){
    int recvtotle = 0;
    int recvnum;
    while (recvtotle < len){
        recvnum = recv(newfd, (char *)pbuf + recvtotle, len - recvtotle, 0);
        if (recvnum == 0){
            return -1;
        }
        recvtotle += recvnum;
    }
    return 0;
}
