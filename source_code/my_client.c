#include "tls.h" //全include进来，给的源码中定义的我们先都copy，小的改动注释写明
#include "vpn.h"

// 标记下删除的地方
char last_ip[5]; 
void *listen_tun_client(void *threadData)
{
    PTHDATA ptd = (PTHDATA)threadData;
    while (1)
    {
        int len;
        char buff[2000];
        bzero(buff, 2000);
        len = read(ptd->tunfd, buff, 2000);
        if (len > 19 && buff[0] == 0x45)
        {
            //if ((int)buff[15] == atoi(last_ip)){
                printf("Receive TUN: %d\n", len);
                SSL_write(ptd->ssl, buff, len);
            //}
            //else{
            //    printf("Error: Incorrect IP: 192.168.53.%s", last_ip);
            //}服务端自动分配ip了
        }
    }
}
int main(int argc, char *argv[])
{
    // 与证书中的Common Name统一
    char *hostname = "Zj_W1nd";
    int port = 4433;
    if (argc > 1)
		hostname = argv[1];
	if (argc > 2)
		port = atoi(argv[2]);

    if (argc < 5)
    {
        printf("Error: Missing arguments.\n");
        exit(1);
    }

    hostname = argv[1];
    port = atoi(argv[2]);
    //last_ip = argv[5];

    /*------ TLS initialization ------*/
    SSL *ssl = setupTLSClient(hostname);
    printf("Set up TLS client successfully!\n");

    /*------ TCP connection ------*/
    int sockfd = setupTCPClient(hostname, port);
    printf("Set up TCP client successfully!\n");

    /*------ TLS handshake ------*/
    SSL_set_fd(ssl, sockfd);
    int err = SSL_connect(ssl); //?刚刚好好地调一下就失效了？
    CHK_SSL(err);
    printf("SSL connected! \n");
    printf("SSL connection using %s\n", SSL_get_cipher(ssl));

    /*------ Authenticating Client by user/passwd ------*/
    SSL_write(ssl, argv[3], strlen(argv[3])); // username
    SSL_write(ssl, argv[4], strlen(argv[4])); // password
    //SSL_write(ssl, argv[5], strlen(argv[5])); // local the last byte of IP
    last_ip[SSL_read(ssl, last_ip, sizeof(last_ip) - 1)]='\0';
    /*------ Send/Receive data ------*/
    int tunfd = createTunDevice();
    pthread_t listen_tun_thread;
    THDATA threadData;
    threadData.tunfd = tunfd;
    threadData.ssl = ssl;
    pthread_create(&listen_tun_thread, NULL, listen_tun_client, (void *)&threadData);

    // redirect and routing ###########自动分配ip并配置路由
    char cmd[100];
    sprintf(cmd, "sudo ifconfig tun0 192.168.53.%s/24 up && sudo route add -net 192.168.60.0/24 tun0",
            last_ip);
    system(cmd);

    int len;
    do
    {
        char buf[9000];
        len = SSL_read(ssl, buf, sizeof(buf) - 1);
        write(tunfd, buf, len);
        printf("SSL received: %d\n", len);
    } while (len > 0);
    pthread_cancel(listen_tun_thread);
    printf("Connection closed!\n");
    return 0;
}
