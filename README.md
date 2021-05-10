# MyNetdisk
## 1 使用配置

​	在/server/config/中按要求修改server.conf和mysql.conf

## 2 功能实现

- [x] 配置文件读取
- [x] 服务器日志
- [x] 连接mysql
- [x] 退出机制
- [x] 账号注册、登录(使用salt加密)
- [x] `gets` 、`puts` 、`mkdir`、 `cd`、 `ls`、 `lls`、`pwd`、`rm` 、`q`命令
- [x] 虚拟文件系统
- [x] 断点续传
- [x] MD5检验
- [x] 部分命令分离
- [x] 引导界面

## 3 启动

​	通过./server ./config/server.conf 启动服务器端

​	通过./client IP port 方式启动客户端

## 4 使用

​	跟随客户端引导界面使用即可







