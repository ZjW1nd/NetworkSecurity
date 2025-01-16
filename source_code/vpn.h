#ifndef VPN_H
#define VPN_H

#include "tls.h"
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <pthread.h>

typedef struct thread_data
{
    int tunfd;
    SSL *ssl;
} THDATA, *PTHDATA;

typedef struct pipe_data {
    char *pipe_file;
    SSL *ssl;
} PIPEDATA;

int createTunDevice();
void *listen_tun_server(void *tunfd);

#endif
