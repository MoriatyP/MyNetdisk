#include "../include/server.h"

int do_gets(int taskfd, MYSQL *conn){
    int ret;
    time_t now;
    datapackage_t data;
    int dir_id, user_id;
    char *real_filename;
    off_t offset;

    //接收当前文件夹id
    ret = recv_cycle(taskfd, &dir_id, sizeof(int));
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收用户id
    ret = recv_cycle(taskfd, &user_id, sizeof(int));
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收文件申请
    ret = recv_cycle(taskfd, &data.datalen, sizeof(int));
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(taskfd, data.buf, data.datalen);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    
    //查找是否存在该文件
    MYSQL_RES *res = mysql_select_two(conn, "files", "filename", data.buf, "pre_id", dir_id);
    MYSQL_ROW row;
    
    if (res){
        row = mysql_fetch_row(res);
    }
    
    bzero(data.buf, sizeof(data.buf));
    
    if (!res || strcmp(row[5], "0") == 0){ 
        //如果没找到该文件或者是文件夹，返回文件不存在
        strcpy(data.buf, "the file does not exist");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
    else {
        //找到返回找到并开始下载
        strcpy(data.buf, "find the file and start downloading");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            return -1;
        }
    }
    //接收偏移量
    ret = recv_cycle(taskfd, &data.datalen, sizeof(int));
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    
    ret = recv_cycle(taskfd, &offset, data.datalen);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    //打开服务器源端真实文件
    real_filename = row[4];
    int fd = open(real_filename, O_RDWR);
    ERROR_CHECK(fd, -1, "open");
    //发送文件大小
    struct stat statbuf;
    fstat(fd, &statbuf);
    data.datalen = sizeof(statbuf.st_size);
    memcpy(data.buf, &statbuf.st_size, data.datalen);
    ret = send_cycle(taskfd, &data, 4 + data.datalen);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    //发送文件内容
    char *pstart = (char *)mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ERROR_CHECK(pstart, (char *)-1, "mmap");
    if (offset) {
        //如果是断点续传，偏移到断点再传输
        pstart += offset;
    }
    ret = send_cycle(taskfd, pstart, statbuf.st_size - offset);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("Client close\n");
        return -1;
    }
    
    munmap(pstart, statbuf.st_size);
    return 0;
}

int do_puts(int taskfd, MYSQL *conn){
    int ret;
    time_t now;
    datapackage_t data;
    int dir_id, user_id;
    char filename[DATA_BUF_LEN] = {0};
    char namebuf[DATA_BUF_LEN] = {0};
    char *real_filename;
    char md5sum[DATA_BUF_LEN] = {0};
    int repnum = 1;
    off_t filesize;
    int fd;

    //接收当前文件夹id
    ret = recv_cycle(taskfd, &dir_id, sizeof(int));
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收用户id
    ret = recv_cycle(taskfd, &user_id, sizeof(int));
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收文件名
    ret = recv_cycle(taskfd, &data.datalen, sizeof(int));
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(taskfd, data.buf, data.datalen);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    strcpy(filename, data.buf);
    //查找是否存在该文件
    MYSQL_RES *res = mysql_select_two(conn, "files", "filename", data.buf, "pre_id", dir_id);
    MYSQL_ROW row;
    if (res){
        row = mysql_fetch_row(res);
    }
    bzero(data.buf, sizeof(data.buf));
    if (res) {
        //如果找到重名文件，返回重名，退出
        strcpy(data.buf, "you already have the file with the same name");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
    else {
        //未找到重名文件，返回开始上传
        strcpy(data.buf, "start uploading");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            return -1;
        }
    }
    //接收文件大小
    ret = recv_cycle(taskfd, &data.datalen, sizeof(int));
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    
    ret = recv_cycle(taskfd, &filesize, data.datalen);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    //接收文件MD5码
    
    ret = recv_cycle(taskfd, &data.datalen, sizeof(int));
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    bzero(data.buf, sizeof(data.buf));
    ret = recv_cycle(taskfd, data.buf, data.datalen);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    //查找服务器端是否已经存在源真实文件,并在虚拟文件系统中插入文件
    res = mysql_select_two(conn, "files", "md5sum", data.buf, "filesize", filesize);
    if (res){
        //如果已经存在，返回秒传，退出
        row = mysql_fetch_row(res);
        real_filename = row[4];
        ret = mysql_insert_file(conn, dir_id, user_id, filename, real_filename, filesize, data.buf);
        ERROR_CHECK(ret, -1, "mysql_insert_file");
        bzero(data.buf, sizeof(data.buf));
        strcpy(data.buf, "quick upload");
        data.datalen = strlen(data.buf);
        ret = send_cycle(taskfd, &data, 4 + data.datalen);
        
        if (ret == -1){
            time(&now);
            printf("%s: ", ctime(&now));
            printf("client close\n");
            return -1;
        }
        return 0;
    }
    //否则在服务器端创建新文件
    
    strcpy(md5sum, data.buf);
    bzero(data.buf, sizeof(data.buf));
    strcpy(data.buf, "begin");
    data.datalen = strlen(data.buf);
    ret = send_cycle(taskfd, &data, 4 + data.datalen);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("client close\n");
        return -1;
    }
    
    while ((fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0666)) == -1){
        bzero(filename, sizeof(filename));
        sprintf(filename, "%s(%d)", namebuf, repnum++);
    }
    //接收文件内容
    ftruncate(fd, filesize);
    char *pstart = (char *)mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ERROR_CHECK(pstart, (char *)-1, "mmap");
    ret = recv_cycle(taskfd, pstart, filesize);
    
    if (ret == -1){
        time(&now);
        printf("%s: ", ctime(&now));
        printf("Client close\n");
        close(fd);
        remove(filename);
        return -1;
    }
    
    munmap(pstart, filesize);
    close(fd);
    //虚拟文件系统插入文件
    ret = mysql_insert_file(conn, dir_id, user_id, filename, filename, filesize, md5sum);
    ERROR_CHECK(ret, -1, "mysql_insert_file");
    return 0;
}