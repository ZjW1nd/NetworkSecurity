#include "vpn.h"

// Copy 模块
int createTunDevice()
{
	int tunfd;
	struct ifreq ifr;
	int ret;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	tunfd = open("/dev/net/tun", O_RDWR);
	if (tunfd == -1) {
		printf("Open /dev/net/tun failed! (%d: %s)\n", errno, strerror(errno));
		return -1;
	}
	ret = ioctl(tunfd, TUNSETIFF, &ifr);
	if (ret == -1) {
		printf("Setup TUN interface by ioctl failed! (%d: %s)\n", errno, strerror(errno));
		return -1;
	}

	printf("Setup TUN interface success!\n");
	return tunfd;
}

void *listen_tun_server(void *tunfd) {
    int fd = *((int *)tunfd);
    char buff[2000];
    while (1) {
        int len = read(fd, buff, 2000);
        if (len > 19 && buff[0] == 0x45) {
            printf("TUN Received, length : %d , destination : 192.168.53.%d\n", len, (int)buff[19]);
            char pipe_file[10];
            sprintf(pipe_file, "./pipe/%d", (int)buff[19]);
            int fd = open(pipe_file, O_WRONLY);
            if (fd == -1) {
                printf("[WARNING] File %s does not exist.\n", pipe_file);
            } else {
                write(fd, buff, len);
            }
        }
    }
}