#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "shared-header.h"

static int send_fd_over_socket(int socket_fd, int fd_to_send) {
	struct iovec io;
	char *iobuf = "i'm sending you a file";
	struct msghdr msg = { 0 };
	union {         /* Ancillary data buffer, wrapped in a union
			   in order to ensure it is suitably aligned */
		char buf[CMSG_SPACE(sizeof(int))];
		struct cmsghdr align;
	} u;

	io.iov_base = iobuf;
	io.iov_len = strlen(iobuf);
	printf("sizeof iobuf: %lu\n", strlen(iobuf));
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = u.buf;
	msg.msg_controllen = sizeof(u.buf);

	struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd_to_send));
	*((int *) CMSG_DATA(cmsg)) = fd_to_send;
	msg.msg_controllen = CMSG_SPACE(sizeof(fd_to_send));

	return sendmsg(socket_fd, &msg, 0);
}

int main(int argc, char **argv) {
	int fd;

	printf("In sender pid %i\n", getpid());
	fd = open("/dev/zero", O_RDWR);

	for(;;) {
		if (send_fd_over_socket(3, fd) < 0)
			printf("Failed to send message\n");
		sleep(1);
	}
	return 0;
}
