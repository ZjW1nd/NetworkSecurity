# 编译器和编译选项
CC = gcc
CFLAGS = -lssl -lcrypto -lpthread -w

# 目标文件
CLIENT_TARGET = my_client
SERVER_TARGET = my_server

# 源文件
CLIENT_SRC = my_client.c tls.c vpn.c
SERVER_SRC = my_server.c tls.c vpn.c

# 头文件
HEADERS = tls.h vpn.h

# 默认目标
all: $(CLIENT_TARGET) $(SERVER_TARGET) copy_files

# 编译客户端
$(CLIENT_TARGET): $(CLIENT_SRC) $(HEADERS)
	$(CC) -o $(CLIENT_TARGET) $(CLIENT_SRC) $(CFLAGS)

# 编译服务器
$(SERVER_TARGET): $(SERVER_SRC) $(HEADERS)
	$(CC) -o $(SERVER_TARGET) $(SERVER_SRC) $(CFLAGS) -lcrypt

# 复制文件到Docker容器
copy_files:
	sudo docker cp ../../my_vpn_all HostU:/
	sudo docker cp ../../my_vpn_all HostU2:/

# 清理生成的文件
clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)

.PHONY: all copy_files clean