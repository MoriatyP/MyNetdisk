#include "../include/server.h"

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
