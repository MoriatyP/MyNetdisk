SRCS:=$(wildcard bin/*.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:=server
CC:=gcc
#mysql编译时加入"mysql_config --cflags --libs"系统会自动查找mysql依赖的内容
CFLAGS:=$(shell mysql_config --cflags --libs) -g -Wall -gdwarf-2
$(ELF):$(OBJS)
		gcc -o $@ $^ $(CFLAGS) -lcrypt -lpthread
clean:
		rm -rf $(OBJS) $(ELF) 
gdb:
	gdbserver 192.168.50.212:1500 $(ELF) config/server.conf
