SRCS:=$(wildcard bin/*.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:=client
CC:=gcc
#mysql编译时加入"mysql_config --cflags --libs"系统会自动查找mysql依赖的内容
CFLAGS:=$(shell mysql_config --cflags --libs) -g -Wall
$(ELF):$(OBJS)
		gcc -o $@ $^ $(CFLAGS) -lcrypt -lpthread
clean:
		rm -rf $(OBJS) $(ELF) 

