#include "tls.h"
#include "vpn.h"
#include <crypt.h>
#include <shadow.h>
#include <unistd.h>
// 自动分配ipC段尝试

// 用ip_pools.txt实现多进程共享ip池，共享内存和进程通信太麻烦了
// 文件记录的格式为每行"cseg:0/1"，0表示未分配，1表示已分配
// 遍历读取到0的行，修改为1，返回cseg，cseg为2-254
// 并发控制和文件锁
int get_ip_c_seg() {
    int fd = open("ip_pools.txt", O_RDWR);
    if (fd == -1) {
        perror("Error opening ip_pools.txt");
        return -1;
    }
    // 加锁
    if (flock(fd, LOCK_EX) == -1) {
        perror("Error locking file");
        close(fd);
        return -1;
    }
    FILE *fp = fdopen(fd, "r+");
    if (fp == NULL) {
        perror("Error converting file descriptor to FILE pointer");
        close(fd);
        return -1;
    }
    int c_seg;
    char status;
    while (fscanf(fp, "%d:%c\n", &c_seg, &status) != EOF) {
        if (status == '0') {
            fseek(fp, -2, SEEK_CUR); // 回到状态位的位置
            fputc('1', fp);
            fflush(fp); // 确保数据写入文件
            flock(fd, LOCK_UN); // 解锁
            fclose(fp);
            return c_seg;
        }
    }
    // 解锁
    flock(fd, LOCK_UN);
    fclose(fp);
    return -1;
}

int free_ip_c_seg(int c_seg) {
    int fd = open("ip_pools.txt", O_RDWR);
    if (fd == -1) {
        perror("Error opening ip_pools.txt");
        return -1;
    }
    // 加锁
    if (flock(fd, LOCK_EX) == -1) {
        perror("Error locking file");
        close(fd);
        return -1;
    }
    FILE *fp = fdopen(fd, "r+");
    if (fp == NULL) {
        perror("Error converting file descriptor to FILE pointer");
        close(fd);
        return -1;
    }
    int c_seg_tmp;
    char status;
    while (fscanf(fp, "%d:%c\n", &c_seg_tmp, &status) != EOF) {
        if (c_seg_tmp == c_seg) {
            fseek(fp, -2, SEEK_CUR); // 回到状态位的位置
            fputc('0', fp);
            fflush(fp); // 确保数据写入文件
            flock(fd, LOCK_UN); // 解锁
            fclose(fp);
            return 0;
        }
    }
    // 解锁
    flock(fd, LOCK_UN);
    fclose(fp);
    return -1;
}

int login(char *user, char *passwd) {
    struct spwd *pw;
    char *epasswd; 
    pw = getspnam(user);
    if (pw == NULL) {
        printf("Error: Password is NULL.\n");
        return 0;
    }
    printf("USERNAME : %s\n", pw->sp_namp);
    printf("PASSWORD : %s\n", pw->sp_pwdp);
    epasswd = crypt(passwd, pw->sp_pwdp);
    if (strcmp(epasswd, pw->sp_pwdp)) {
        printf("Error: The password is incorrect!\n");
        return 0;
    }
    return 1;
}

void *listen_pipe(void *threadData) {
    PIPEDATA *ptd = (PIPEDATA*)threadData;
    int pipefd = open(ptd->pipe_file, O_RDONLY);

    char buff[2000];
    int len;
    do {
        len = read(pipefd, buff, 2000);
        SSL_write(ptd->ssl, buff, len);
    } while (len > 0);
    printf("%s read 0 byte. Connection closed and file removed.\n", ptd->pipe_file);
    remove(ptd->pipe_file);
}

void send_out(SSL *ssl, int sock, int tunfd) {
    int len;
    do {
        char buf[1024];
        len = SSL_read(ssl, buf, sizeof(buf) - 1);
        write(tunfd, buf, len);
        buf[len] = '\0';
        printf("SSL Received : %d bytes\n", len);
    } while (len > 0);
    printf("SSL shutdown.\n");
}

int main() {
    int err;
    struct sockaddr_in sa_client;
    socklen_t client_len = sizeof(sa_client);
    /*------ TLS Initialization ------*/    
    SSL *ssl = setupTLSServer();
    /*------ TCP Connect ------*/
    int listen_sock = setupTCPServer();
    // tunnel init, redirect and forward
    int tunfd = createTunDevice();
    system("sudo ifconfig tun0 192.168.53.1/24 up && sudo sysctl net.ipv4.ip_forward=1");
    // pipe folder init
    system("rm -rf pipe");
    mkdir("pipe", 0666);
    // creat a listen tunnel
    pthread_t listen_tun_thread;
    pthread_create(&listen_tun_thread, NULL, listen_tun_server, (void *)&tunfd);

    while (1) {
        int sock = accept(listen_sock, (struct sockaddr *)&sa_client, &client_len);  // block, if try connect
        fprintf(stderr, "sock = %d\n", sock);
		if (sock == -1) {
			fprintf(stderr, "Accept TCP connect failed! (%d: %s)\n", errno, strerror(errno));
			continue;
		}
        // The child process
        if (fork() == 0) {
            // Copied from tlsserver.c/main: 97-104
            close(listen_sock);
            SSL_set_fd(ssl, sock);
            int err = SSL_accept(ssl);
            CHK_SSL(err);
            printf(" SSL connection established!\n");

            // Process Request: 
            char user[1024], passwd[1024]/*, ip_c_seg[1024]*/,ip_c_seg2[10];
            user[SSL_read(ssl, user, sizeof(user) - 1)] = '\0';
            passwd[SSL_read(ssl, passwd, sizeof(passwd) - 1)] = '\0';
            int c=get_ip_c_seg();
            if(c==-1){
                printf("Error: IP pool is full.\n");
                return 0;
            }
            sprintf(ip_c_seg2, "%d", c);
            SSL_write(ssl, ip_c_seg2, strlen(ip_c_seg2)); //与客户端协商ip
            //ip_c_seg[SSL_read(ssl, ip_c_seg, sizeof(ip_c_seg) - 1)] = '\0';
            if (login(user, passwd)) {
                printf("[\033[1;32m√\033[0m] Login successfully!\n");
                // check IP and create pipe file
                char pipe_file[10];
                sprintf(pipe_file, "./pipe/%s", ip_c_seg2);
                if (mkfifo(pipe_file, 0666) == -1) {
                    printf("Warning: This IP(%s) has been occupied.", ip_c_seg2);
                } else {
                    pthread_t listen_pipe_thread;
                    PIPEDATA pipeData;
                    pipeData.pipe_file = pipe_file;
                    pipeData.ssl = ssl;
                    pthread_create(&listen_pipe_thread, NULL, listen_pipe, (void *)&pipeData);
                    send_out(ssl, sock, tunfd);
                    pthread_cancel(listen_pipe_thread);
                    remove(pipe_file);
                }
            } else {
                printf("Error: Login failed!\n");
            }
            SSL_shutdown(ssl);
            SSL_free(ssl);
            close(sock);
            free_ip_c_seg(c);
            printf("Socket closed!\n");
            return 0;
        } else {  // The parent process
            close(sock);
        }
    }
}
